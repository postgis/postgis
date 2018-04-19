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
update_r(POINT2D** objs, int* clusters, uint32_t n, POINT2D** centers, uint32_t k)
{
	POINT2D* obj;
	unsigned int i;
	double distance, curr_distance;
	uint32_t cluster, curr_cluster;

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
		clusters[i] = (int) curr_cluster;
	}
}

static void
update_means(POINT2D** objs, int* clusters, uint32_t n, POINT2D** centers, uint32_t* weights, uint32_t k)
{
	uint32_t i;

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
kmeans(POINT2D** objs, int* clusters, uint32_t n, POINT2D** centers, uint32_t k)
{
	uint32_t i = 0;
	int* clusters_last;
	int converged = LW_FALSE;
	size_t clusters_sz = sizeof(int) * n;
	uint32_t* weights;

	weights = lwalloc(sizeof(int) * k);

	/* Previous cluster state array. At this time, r doesn't mean anything but it's ok */
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

static void
kmeans_init(POINT2D** objs, int* clusters, uint32_t n, POINT2D** centers, POINT2D* centers_raw, uint32_t k)
{
	double* distances;
	uint32_t p1 = 0, p2 = 0;
	uint32_t i;
	uint32_t j;
	double max_dst = -1;
	double dst_p1, dst_p2;

	/* k = 1: first non-null is ok */
	if (k == 1)
	{
		for (i = 0; i < n; i++)
		{
			if (!objs[i]) continue;
			centers_raw[0] = *((POINT2D *)objs[i]);
			centers[0] = &(centers_raw[0]);
			return;
		}
		assert(0);
	}

	/* k >= 2: find two distant points greedily */
	for (i = 0; i < n; i++)
	{
		/* skip null */
		if (!objs[i]) continue;

		/* reinit if first element happened to be null */
		if (!objs[p1] && !objs[p2])
		{
			p1 = i;
			p2 = i;
			continue;
		}

		/* if we found a larger distance, replace our choice */
		dst_p1 = distance2d_sqr_pt_pt(objs[i], objs[p1]);
		dst_p2 = distance2d_sqr_pt_pt(objs[i], objs[p2]);
		if ((dst_p1 > max_dst) || (dst_p2 > max_dst))
		{
			max_dst = fmax(dst_p1, dst_p2);
			if (dst_p1 > dst_p2)
				p2 = i;
			else
				p1 = i;
		}
	}

	/* by now two points should be found and non-same */
	assert(p1 != p2 && objs[p1] && objs[p2] && max_dst >= 0);

	/* accept first two points */
	centers_raw[0] = *((POINT2D *)objs[p1]);
	centers[0] = &(centers_raw[0]);
	centers_raw[1] = *((POINT2D *)objs[p2]);
	centers[1] = &(centers_raw[1]);

	if (k > 2)
	{
		/* array of minimum distance to a point from accepted cluster centers */
		distances = lwalloc(sizeof(double) * n);
		distances[p1] = -1;
		distances[p2] = -1;
		for (j = 0; j < n; j++)
		{
			if (distances[j] < 0)
				continue;
			if (!objs[j])
			{
				distances[j] = -1;
				continue;
			}
			distances[j] = distance2d_sqr_pt_pt(objs[j], centers[0]);
		}

		/* loop i on clusters, skip 0 and 1 as it's found already */
		for (i = 2; i < k; i++)
		{

			uint32_t candidate_center = 0;
			double max_distance = -DBL_MAX;
			double curr_distance;

			/* loop j on objs */
			for (j = 0; j < n; j++)
			{
				/* empty objs and accepted clusters are already marked with distance = -1 */
				if (distances[j] < 0) continue;

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

			/* Something is wrong with data, cannot find a candidate.
			 * Checked earlier by counting entries on input */
			assert(max_distance >= 0);

			/* accept candidate to centers */
			distances[candidate_center] = -1;
			/* Copy the point coordinates into the initial centers array
			 * This is ugly, but the centers array is an array of pointers to points, not an array of points */
			centers_raw[i] = *((POINT2D *)objs[candidate_center]);
			centers[i] = &(centers_raw[i]);
		}
		lwfree(distances);
	}
}

int*
lwgeom_cluster_2d_kmeans(const LWGEOM** geoms, uint32_t n, uint32_t k)
{
	uint32_t i;
	uint32_t num_centroids = 0;
	uint32_t num_non_empty = 0;
	LWGEOM** centroids;
	POINT2D* centers_raw;
	const POINT2D* cp;
	int result = LW_FALSE;

	/* An array of objects to be analyzed.
	 * All NULL values will be returned in the KMEANS_NULL_CLUSTER. */
	POINT2D** objs;

	/* An array of centers for the algorithm. */
	POINT2D** centers;

	/* Array to fill in with cluster numbers. */
	int* clusters;

	assert(k > 0);
	assert(n > 0);
	assert(geoms);

	if (n < k)
	{
		lwerror("%s: number of geometries is less than the number of clusters requested", __func__);
		return NULL;
	}

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

	/* Prepare the list of object pointers for K-means */
	for (i = 0; i < n; i++)
	{
		const LWGEOM* geom = geoms[i];
		LWPOINT* lwpoint;

		/* Null/empty geometries get a NULL pointer */
		if ((!geom) || lwgeom_is_empty(geom))
		{
			objs[i] = NULL;
			continue;
		}

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
			lwpoint = lwgeom_as_lwpoint(geom);

		/* Store a pointer to the POINT2D we are interested in */
		cp = getPoint2d_cp(lwpoint->point, 0);
		objs[i] = (POINT2D*)cp;
		num_non_empty++;
	}

	if (num_non_empty < k)
	{
		lwerror("%s: number of clusterable geometries is less than the number of clusters requested", __func__);
		return NULL;
	}

	kmeans_init(objs, clusters, n, centers, centers_raw, k);

	result = kmeans(objs, clusters, n, centers, k);

	/* Before error handling, might as well clean up all the inputs */
	lwfree(objs);
	lwfree(centers);
	lwfree(centers_raw);
	lwfree(centroids);

	/* Good result */
	if (result)
		return clusters;

	/* Bad result, not going to need the answer */
	lwfree(clusters);
	return NULL;
}
