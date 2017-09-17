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

static int lwkmeans_pt_closest(const Pointer * objs, size_t num_objs, const Pointer a)
{
	int i;
	double d;
	double d_closest = FLT_MAX;
	int closest = -1;

	assert(num_objs > 0);

	for (i = 0; i < num_objs; i++)
	{
		/* Skip nulls/empties */
		if (!objs[i])
			continue;

		d = lwkmeans_pt_distance(objs[i], a);
		if (d < d_closest)
		{
			d_closest = d;
			closest = i;
		}
	}

	return closest;
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
	const POINT2D *cp;
	POINT2D min = { DBL_MAX,   DBL_MAX };
	POINT2D max = { -DBL_MAX, -DBL_MAX };
	double dx, dy;
	kmeans_config config;
	kmeans_result result;
	int *seen;
	int sidx = 0;

	assert(k>0);
	assert(ngeoms>0);
	assert(geoms);

    /* Initialize our static structs */
    memset(&config, 0, sizeof(kmeans_config));
    memset(&result, 0, sizeof(kmeans_result));

	if (ngeoms<k)
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

	/* Prepare the list of object pointers for K-means */
	for (i = 0; i < ngeoms; i++)
	{
		const LWGEOM *geom = geoms[i];
		LWPOINT *lwpoint;

		/* Null/empty geometries get a NULL pointer */
		if ((!geom) || lwgeom_is_empty(geom))
		{
			config.objs[i] = NULL;
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

		/* Since we're already here, let's calculate the extrema of the set */
		if (cp->x < min.x) min.x = cp->x;
		if (cp->y < min.y) min.y = cp->y;
		if (cp->x > max.x) max.x = cp->x;
		if (cp->y > max.y) max.y = cp->y;
	}

	/*
	* We map a uniform assignment of points in the area covered by the set
	* onto actual points in the set
	*/
	dx = (max.x - min.x)/k;
	dy = (max.y - min.y)/k;
	seen = lwalloc(sizeof(int)*config.k);
	memset(seen, 0, sizeof(int)*config.k);
	for (i = 0; i < k; i++)
	{
		int closest;
		POINT2D p;
		int j;

		/* Calculate a point in the range */
		p.x = min.x + dx * (i + 0.5);
		p.y = min.y + dy * (i + 0.5);

		/* Find the data point closest to the calculated point */
		closest = lwkmeans_pt_closest(config.objs, config.num_objs, &p);

		/* If something is terrible wrong w/ data, cannot find a closest */
		if (closest < 0)
			lwerror("unable to calculate cluster seed points, too many NULLs or empties?");

		/* Ensure we aren't already using that point as a seed */
		j = 0;
		while(j < sidx)
		{
			if (seen[j] == closest)
			{
				closest = (closest + 1) % config.num_objs;
			}
			else
			{
				j++;
			}
		}
		seen[sidx++] = closest;

		/* Copy the point coordinates into the initial centers array */
		/* This is ugly, but the centers array is an array of */
		/* pointers to points, not an array of points */
		centers_raw[i] = *((POINT2D*)config.objs[closest]);
		config.centers[i] = &(centers_raw[i]);
	}

	result = kmeans(&config);

	/* Before error handling, might as well clean up all the inputs */
	lwfree(config.objs);
	lwfree(config.centers);
	lwfree(centers_raw);
	lwfree(centroids);
	lwfree(seen);

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

