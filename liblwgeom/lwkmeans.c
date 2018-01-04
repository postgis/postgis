#include <float.h>
#include <math.h>

#include "kmeans.h"
#include "liblwgeom_internal.h"

static double lwkmeans_pt_distance(const Pointer a, const Pointer b)
{
	POINT2D *pa = (POINT2D*)a;
	POINT2D *pb = (POINT2D*)b;

	double dx = (pa->x - pb->x);
	double dy = (pa->y - pb->y);

	return dx*dx + dy*dy;
}

static void lwkmeans_pt_centroid(const Pointer * objs, const int * clusters, size_t num_objs, int cluster, Pointer centroid)
{
	int i;
	int num_cluster = 0;
	POINT2D sum;
	POINT2D **pts = (POINT2D**)objs;
	POINT2D *center = (POINT2D*)centroid;

	sum.x = sum.y = 0.0;

	if (num_objs <= 0) return;

	for (i = 0; i < num_objs; i++)
	{
		/* Skip points that are not of interest */
		if (clusters[i] != cluster) continue;

		sum.x += pts[i]->x;
		sum.y += pts[i]->y;
		num_cluster++;
	}
	if (num_cluster)
	{
		sum.x /= num_cluster;
		sum.y /= num_cluster;
		*center = sum;
	}
	return;
}


int *
lwgeom_cluster_2d_kmeans(const LWGEOM **geoms, int ngeoms, int k)
{
	int i;
	int num_centroids = 0;
	LWGEOM **centroids;
	POINT2D *centers_raw;
	double *distances;
	const POINT2D *cp;
	kmeans_config config;
	kmeans_result result;
	int boundary_point_idx = 0;
	double xmin = DBL_MAX;

	assert(k>0);
	assert(ngeoms>0);
	assert(geoms);

	/* Initialize our static structs */
	memset(&config, 0, sizeof(kmeans_config));
	memset(&result, 0, sizeof(kmeans_result));

	if (ngeoms < k)
	{
		lwerror("%s: number of geometries is less than the number of clusters requested", __func__);
	}

	/* We'll hold the temporary centroid objects here */
	centroids = lwalloc(sizeof(LWGEOM*) * ngeoms);
	memset(centroids, 0, sizeof(LWGEOM*) * ngeoms);

	/* The vector of cluster means. We have to allocate a */
	/* chunk of memory for these because we'll be mutating them */
	/* in the kmeans algorithm */
	centers_raw = lwalloc(sizeof(POINT2D) * k);
	memset(centers_raw, 0, sizeof(POINT2D) * k);

	/* K-means configuration setup */
	config.objs = lwalloc(sizeof(Pointer) * ngeoms);
	config.num_objs = ngeoms;
	config.clusters = lwalloc(sizeof(int) * ngeoms);
	config.centers = lwalloc(sizeof(Pointer) * k);
	config.k = k;
	config.max_iterations = 0;
	config.distance_method = lwkmeans_pt_distance;
	config.centroid_method = lwkmeans_pt_centroid;

	/* Clean the memory */
	memset(config.objs, 0, sizeof(Pointer) * ngeoms);
	memset(config.clusters, 0, sizeof(int) * ngeoms);
	memset(config.centers, 0, sizeof(Pointer) * k);

	/* Array of sums of distances to a point from accepted cluster centers */
	distances = lwalloc(sizeof(double)*config.num_objs);
	memset(distances, 0, sizeof(double)*config.num_objs);

	/* Prepare the list of object pointers for K-means */
	for (i = 0; i < ngeoms; i++)
	{
		const LWGEOM *geom = geoms[i];
		LWPOINT *lwpoint;

		/* Null/empty geometries get a NULL pointer */
		if ((!geom) || lwgeom_is_empty(geom))
		{
			config.objs[i] = NULL;
			/* mark as unreachable */
			distances[i] = -1;
			continue;
		}

		/* If the input is a point, use its coordinates */
		/* If its not a point, convert it to one via centroid */
		if (lwgeom_get_type(geom) != POINTTYPE)
		{
			LWGEOM *centroid = lwgeom_centroid(geom);
			if ((!centroid) || lwgeom_is_empty(centroid))
			{
				config.objs[i] = NULL;
				continue;
			}
			centroids[num_centroids++] = centroid;
			lwpoint = lwgeom_as_lwpoint(centroid);
		}
		else
		{
			lwpoint = lwgeom_as_lwpoint(geom);
		}

		/* Store a pointer to the POINT2D we are interested in */
		cp = getPoint2d_cp(lwpoint->point, 0);
		config.objs[i] = (Pointer)cp;

		/* Find the point that's on the boundary to use as seed */
		if (xmin < cp->x)
		{
			boundary_point_idx = i;
			xmin = cp->x;
		}
	}

	if (xmin == DBL_MAX)
	{
		lwerror("unable to calculate any cluster seed point, too many NULLs or empties?");
	}

	/* start with point on boundary */
	centers_raw[0] = *((POINT2D*)config.objs[boundary_point_idx]);
	config.centers[0] = &(centers_raw[0]);
	/* loop i on clusters, skip 0 as it's found already */
	for (i = 1; i < k; i++)
	{
		int j;
		double max_distance = -DBL_MAX;
		int candidate_center = 0;

		/* loop j on objs */
		for (j = 0; j < config.num_objs; j++)
		{
			/* empty objs and accepted clusters are already marked with distance = -1 */
			if (distances[j] < 0) continue;

			/* greedily take a point that's farthest from all accepted clusters */
			distances[j] += distance2d_sqr_pt_pt((POINT2D)&config.objs[j], (POINT2D)&centers_raw[i-1]);
			if (distances[j] > max_distance)
			{
				candidate_center = j;
				max_distance = distances[j];
			}
		}

		/* something is wrong with data, cannot find a candidate */
		if (max_distance < 0)
			lwerror("unable to calculate cluster seed points, too many NULLs or empties?");

		/* accept candidtate to centers */
		distances[candidate_center] = -1;
		/* Copy the point coordinates into the initial centers array */
		/* This is ugly, but the centers array is an array of */
		/* pointers to points, not an array of points */
		centers_raw[i] = *((POINT2D*)config.objs[candidate_center]);
		config.centers[i] = &(centers_raw[i]);
	}

	result = kmeans(&config);

	/* Before error handling, might as well clean up all the inputs */
	lwfree(config.objs);
	lwfree(config.centers);
	lwfree(centers_raw);
	lwfree(centroids);
	lwfree(distances);

	/* Good result */
	if (result == KMEANS_OK)
		return config.clusters;

	/* Bad result, not going to need the answer */
	lwfree(config.clusters);
	if (result == KMEANS_EXCEEDED_MAX_ITERATIONS)
	{
		lwerror("%s did not converge after %d iterations", __func__, config.max_iterations);
		return NULL;
	}

	/* Unknown error */
	return NULL;
}
