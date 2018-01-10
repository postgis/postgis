/*-------------------------------------------------------------------------
 *
 * Copyright (c) 2018, Darafei Praliaskouski <me@komzpa.net>
 * Copyright (c) 2016, Paul Ramsey <pramsey@cleverelephant.ca>
 *
 *------------------------------------------------------------------------*/

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

static void
update_r(POINT2D** objs, int* clusters, uint32_t n, POINT2D** centers, unsigned int k)
{
	POINT2D* obj;
	unsigned int i;
	double distance, curr_distance;
	int cluster, curr_cluster;

	for (i = 0; i < n; i++)
	{
		obj = objs[i];

		/* Don't try to cluster NULL objects, just add them to the "unclusterable cluster" */
		if (!obj)
		{
			clusters[i] = KMEANS_NULL_CLUSTER;
			continue;
		}

		/* Initialize with distance to first cluster */
		curr_distance = distance2d_sqr_pt_pt(obj, centers[0]);
		curr_cluster = 0;

		/* Check all other cluster centers and find the nearest */
		for (cluster = 1; cluster < k; cluster++)
		{
			distance = distance2d_sqr_pt_pt(obj, centers[cluster]);
			if (distance < curr_distance)
			{
				curr_distance = distance;
				curr_cluster = cluster;
			}
		}

		/* Store the nearest cluster this object is in */
		clusters[i] = curr_cluster;
	}
}

static void
update_means(POINT2D** objs, int* clusters, uint32_t n, POINT2D** centers, unsigned int* weights, unsigned int k)
{
	unsigned int i;

	memset(weights, 0, sizeof(int) * k);
	for (i = 0; i < k; i++)
	{
		centers[i]->x = 0.0;
		centers[i]->y = 0.0;
	}
	for (i = 0; i < n; i++)
	{
		centers[clusters[i]]->x += objs[i]->x;
		centers[clusters[i]]->y += objs[i]->y;
		weights[clusters[i]] += 1;
	}
	for (i = 0; i < k; i++)
	{
		centers[i]->x /= weights[i];
		centers[i]->y /= weights[i];
	}
}

static int
kmeans(POINT2D** objs, int* clusters, uint32_t n, POINT2D** centers, unsigned int k)
{
	unsigned int i = 0;
	int* clusters_last;
	int converged = LW_FALSE;
	uint32_t clusters_sz = sizeof(int) * n;
	unsigned int* weights;

	weights = lwalloc(sizeof(int) * k);

	/*
	 * Previous cluster state array. At this time, r doesn't mean anything
	 * but it's ok
	 */
	clusters_last = lwalloc(clusters_sz);

	for (i = 0; i < KMEANS_MAX_ITERATIONS && !converged; i++)
	{
		LW_ON_INTERRUPT(break);

		/* Store the previous state of the clustering */
		memcpy(clusters_last, clusters, clusters_sz);

		update_r(objs, clusters, n, centers, k);
		update_means(objs, clusters, n, centers, weights, k);

		/* if all the cluster numbers are unchanged, we are at a stable solution */
		converged = memcmp(clusters_last, clusters, clusters_sz) == 0;
	}

	lwfree(clusters_last);
	lwfree(weights);
	if (!converged)
		lwerror("%s did not converge after %d iterations", __func__, i);
	return converged;
}

int*
lwgeom_cluster_2d_kmeans(const LWGEOM** geoms, int n, int k)
{
	unsigned int i;
	unsigned int num_centroids = 0;
	LWGEOM** centroids;
	POINT2D* centers_raw;
	double* distances;
	const POINT2D* cp;
	int result = LW_FALSE;
	unsigned int boundary_point_idx = 0;
	double max_norm = -DBL_MAX;
	double curr_norm;

	/* An array of objects to be analyzed.
	 * All NULL values will be returned in the  KMEANS_NULL_CLUSTER. */
	POINT2D** objs;

	/* An array of centers for the algorithm. */
	POINT2D** centers;

	/* Array to fill in with cluster numbers. */
	int* clusters;

	assert(k > 0);
	assert(n > 0);
	assert(geoms);

	if (n < k)
		lwerror("%s: number of geometries is less than the number of clusters requested", __func__);

	/* We'll hold the temporary centroid objects here */
	centroids = lwalloc(sizeof(LWGEOM*) * n);
	memset(centroids, 0, sizeof(LWGEOM*) * n);

	/* The vector of cluster means. We have to allocate a chunk of memory for these because we'll be mutating them
	 * in the kmeans algorithm */
	centers_raw = lwalloc(sizeof(POINT2D) * k);
	memset(centers_raw, 0, sizeof(POINT2D) * k);

	/* K-means configuration setup */
	objs = lwalloc(sizeof(POINT2D*) * n);
	clusters = lwalloc(sizeof(int) * n);
	centers = lwalloc(sizeof(POINT2D*) * k);

	/* Clean the memory */
	memset(objs, 0, sizeof(POINT2D*) * n);
	memset(clusters, 0, sizeof(int) * n);
	memset(centers, 0, sizeof(POINT2D*) * k);

	/* Array of sums of distances to a point from accepted cluster centers */
	distances = lwalloc(sizeof(double) * n);

	/* Prepare the list of object pointers for K-means */
	for (i = 0; i < n; i++)
	{
		const LWGEOM* geom = geoms[i];
		LWPOINT* lwpoint;

		/* Null/empty geometries get a NULL pointer */
		if ((!geom) || lwgeom_is_empty(geom))
		{
			objs[i] = NULL;
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
				objs[i] = NULL;
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
		objs[i] = (POINT2D*)cp;

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
	centers_raw[0] = *((POINT2D*)objs[boundary_point_idx]);
	centers[0] = &(centers_raw[0]);
	/* loop i on clusters, skip 0 as it's found already */
	for (i = 1; i < k; i++)
	{
		unsigned int j;
		double max_distance = -DBL_MAX;
		double curr_distance;
		unsigned int candidate_center = 0;

		/* loop j on objs */
		for (j = 0; j < n; j++)
		{
			/* empty objs and accepted clusters are already marked with distance = -1 */
			if (distances[j] < 0)
				continue;

			/* update distance to closest cluster */
			curr_distance = distance2d_sqr_pt_pt(objs[j], centers[i - 1]);
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
		/* Copy the point coordinates into the initial centers array
		 * This is ugly, but the centers array is an array of pointers to points, not an array of points */
		centers_raw[i] = *((POINT2D*)objs[candidate_center]);
		centers[i] = &(centers_raw[i]);
	}

	result = kmeans(objs, clusters, n, centers, k);

	/* Before error handling, might as well clean up all the inputs */
	lwfree(objs);
	lwfree(centers);
	lwfree(centers_raw);
	lwfree(centroids);
	lwfree(distances);

	/* Good result */
	if (result)
		return clusters;

	/* Bad result, not going to need the answer */
	lwfree(clusters);
	return NULL;
}