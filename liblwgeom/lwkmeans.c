#include <float.h>
#include <math.h>

#include "liblwgeom_internal.h"

/*
 * When clustering lists with NULL elements, they will get this as
 * their cluster number. (All the other clusters will be non-negative)
 */
#define KMEANS_NULL_CLUSTER -1

/*
 * If the algorithm doesn't converge within this number of iterations,
 * it will return with a failure error code.
 */
#define KMEANS_MAX_ITERATIONS 1000

typedef enum { KMEANS_OK, KMEANS_EXCEEDED_MAX_ITERATIONS, KMEANS_ERROR } kmeans_result;

typedef struct kmeans_config
{
	/* An array of objects to be analyzed. User allocates this array */
	/* and is responsible for freeing it. */
	/* For objects that are not capable of participating in the distance */
	/* calculations, but for which you still want included in the process */
	/* (for examples, database nulls, or geometry empties) use a NULL */
	/* value in this list. All NULL values will be returned in the */
	/* KMEANS_NULL_CLUSTER. */
	POINT2D** objs;

	/* Number of objects in the preceding array */
	size_t num_objs;

	/* An array of initial centers for the algorithm */
	/* Can be randomly assigned, or using proportions, */
	/* unfortunately the algorithm is sensitive to starting */
	/* points, so using a "better" set of starting points */
	/* might be wise. User allocates and is responsible for freeing. */
	POINT2D** centers;

	/* Number of means we are calculating, length of preceding array */
	unsigned int k;

	/* Iteration counter */
	unsigned int total_iterations;

	/* Array to fill in with cluster numbers. User allocates and frees. */
	int* clusters;

} kmeans_config;

kmeans_result kmeans(kmeans_config* config);

int*
lwgeom_cluster_2d_kmeans(const LWGEOM** geoms, int ngeoms, int k)
{
	unsigned int i;
	unsigned int num_centroids = 0;
	LWGEOM** centroids;
	POINT2D* centers_raw;
	double* distances;
	const POINT2D* cp;
	kmeans_config config;
	kmeans_result result;
	unsigned int boundary_point_idx = 0;
	double max_norm = -DBL_MAX;
	double curr_norm;

	assert(k > 0);
	assert(ngeoms > 0);
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
	config.objs = lwalloc(sizeof(POINT2D*) * ngeoms);
	config.num_objs = ngeoms;
	config.clusters = lwalloc(sizeof(int) * ngeoms);
	config.centers = lwalloc(sizeof(POINT2D*) * k);
	config.k = k;

	/* Clean the memory */
	memset(config.objs, 0, sizeof(POINT2D*) * ngeoms);
	memset(config.clusters, 0, sizeof(int) * ngeoms);
	memset(config.centers, 0, sizeof(POINT2D*) * k);

	/* Array of sums of distances to a point from accepted cluster centers */
	distances = lwalloc(sizeof(double) * config.num_objs);

	/* Prepare the list of object pointers for K-means */
	for (i = 0; i < ngeoms; i++)
	{
		const LWGEOM* geom = geoms[i];
		LWPOINT* lwpoint;

		/* Null/empty geometries get a NULL pointer */
		if ((!geom) || lwgeom_is_empty(geom))
		{
			config.objs[i] = NULL;
			/* mark as unreachable */
			distances[i] = -1;
			continue;
		}

		distances[i] = DBL_MAX;

		/* If the input is a point, use its coordinates */
		/* If its not a point, convert it to one via centroid */
		if (lwgeom_get_type(geom) != POINTTYPE)
		{
			LWGEOM* centroid = lwgeom_centroid(geom);
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
		config.objs[i] = (POINT2D*)cp;

		/* Find the point with largest Euclidean norm to use as seed */
		curr_norm = (cp->x) * (cp->x) + (cp->y) * (cp->y);
		if (curr_norm > max_norm)
		{
			boundary_point_idx = i;
			max_norm = curr_norm;
		}
	}

	if (max_norm == -DBL_MAX)
	{
		lwerror("unable to calculate any cluster seed point, too many NULLs or empties?");
	}

	/* start with point on boundary */
	distances[boundary_point_idx] = -1;
	centers_raw[0] = *((POINT2D*)config.objs[boundary_point_idx]);
	config.centers[0] = &(centers_raw[0]);
	/* loop i on clusters, skip 0 as it's found already */
	for (i = 1; i < k; i++)
	{
		unsigned int j;
		double max_distance = -DBL_MAX;
		double curr_distance;
		unsigned int candidate_center = 0;

		/* loop j on objs */
		for (j = 0; j < config.num_objs; j++)
		{
			/* empty objs and accepted clusters are already marked with distance = -1 */
			if (distances[j] < 0)
				continue;

			/* update distance to closest cluster */
			curr_distance = distance2d_sqr_pt_pt(config.objs[j], config.centers[i - 1]);
			distances[j] = fmin(curr_distance, distances[j]);

			/* greedily take a point that's farthest from any of accepted clusters */
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
		lwerror("%s did not converge after %d iterations", __func__, KMEANS_MAX_ITERATIONS);
		return NULL;
	}

	/* Unknown error */
	return NULL;
}

static void
update_r(kmeans_config* config)
{
	unsigned int i;
	assert(config->objs != NULL);
	assert(config->num_objs > 0);
	assert(config->centers);
	assert(config->clusters);
	for (i = 0; i < config->num_objs; i++)
	{
		double distance, curr_distance;
		int cluster, curr_cluster;
		POINT2D* obj;

		obj = config->objs[i];

		/*
		 * Don't try to cluster NULL objects, just add them
		 * to the "unclusterable cluster"
		 */
		if (!obj)
		{
			config->clusters[i] = KMEANS_NULL_CLUSTER;
			continue;
		}

		/* Initialize with distance to first cluster */
		curr_distance = distance2d_sqr_pt_pt(obj, config->centers[0]);
		curr_cluster = 0;

		/* Check all other cluster centers and find the nearest */
		for (cluster = 1; cluster < config->k; cluster++)
		{
			distance = distance2d_sqr_pt_pt(obj, config->centers[cluster]);
			if (distance < curr_distance)
			{
				curr_distance = distance;
				curr_cluster = cluster;
			}
		}

		/* Store the nearest cluster this object is in */
		config->clusters[i] = curr_cluster;
	}
}

static void
update_means(kmeans_config* config)
{
	unsigned int i;
	unsigned int* weights;

	weights = lwalloc(sizeof(int) * config->k);
	memset(weights, 0, sizeof(int) * config->k);

	for (i = 0; i < config->k; i++)
	{
		config->centers[i]->x = 0.0;
		config->centers[i]->y = 0.0;
	}
	for (i = 0; i < config->num_objs; i++)
	{
		config->centers[config->clusters[i]]->x += config->objs[i]->x;
		config->centers[config->clusters[i]]->y += config->objs[i]->y;
		weights[config->clusters[i]] += 1;
	}
	for (i = 0; i < config->k; i++)
	{
		config->centers[i]->x /= weights[i];
		config->centers[i]->y /= weights[i];
	}
	lwfree(weights);
}

kmeans_result
kmeans(kmeans_config* config)
{
	unsigned int iterations = 0;
	int* clusters_last;
	size_t clusters_sz = sizeof(int) * config->num_objs;

	assert(config);
	assert(config->objs);
	assert(config->num_objs);
	assert(config->centers);
	assert(config->k);
	assert(config->clusters);
	assert(config->k <= config->num_objs);

	/* Zero out cluster numbers, just in case user forgets */
	memset(config->clusters, 0, clusters_sz);

	/*
	 * Previous cluster state array. At this time, r doesn't mean anything
	 * but it's ok
	 */
	clusters_last = lwalloc(clusters_sz);

	while (1)
	{
		LW_ON_INTERRUPT(lwfree(clusters_last); return KMEANS_ERROR);

		/* Store the previous state of the clustering */
		memcpy(clusters_last, config->clusters, clusters_sz);

		update_r(config);
		update_means(config);

		/*
		 * if all the cluster numbers are unchanged since last time,
		 * we are at a stable solution, so we can stop here
		 */
		if (memcmp(clusters_last, config->clusters, clusters_sz) == 0)
		{
			lwfree(clusters_last);
			config->total_iterations = iterations;
			return KMEANS_OK;
		}

		if (iterations++ > KMEANS_MAX_ITERATIONS)
		{
			lwfree(clusters_last);
			config->total_iterations = iterations;
			return KMEANS_EXCEEDED_MAX_ITERATIONS;
		}
	}

	lwfree(clusters_last);
	config->total_iterations = iterations;
	return KMEANS_ERROR;
}
