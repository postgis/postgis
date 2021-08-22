/*-------------------------------------------------------------------------
 *
 * Copyright (c) 2018-2020, Darafei Praliaskouski <me@komzpa.net>
 * Copyright (c) 2016, Paul Ramsey <pramsey@cleverelephant.ca>
 *
 *------------------------------------------------------------------------*/

#include "liblwgeom_internal.h"

/*
 * When clustering lists with NULL or EMPTY elements, they will get this as
 * their cluster number. (All the other clusters will be non-negative)
 */
#define KMEANS_NULL_CLUSTER -1

/*
 * If the algorithm doesn't converge within this number of iterations,
 * it will return with a failure error code.
 */
#define KMEANS_MAX_ITERATIONS 1000

inline static double
distance3d_sqr_pt4d_pt4d(const POINT4D *p1, const POINT4D *p2)
{
	double hside = p2->x - p1->x;
	double vside = p2->y - p1->y;
	double zside = p2->z - p1->z;

	return hside * hside + vside * vside + zside * zside;
}

static uint8_t
update_r(POINT4D *objs, int *clusters, uint32_t n, POINT4D *centers, uint32_t k)
{
	uint8_t converged = LW_TRUE;

	for (uint32_t i = 0; i < n; i++)
	{
		POINT4D obj = objs[i];

		/* Initialize with distance to first cluster */
		double curr_distance = distance3d_sqr_pt4d_pt4d(&obj, &centers[0]);
		int curr_cluster = 0;

		/* Check all other cluster centers and find the nearest */
		for (int cluster = 1; cluster < (int)k; cluster++)
		{
			double distance = distance3d_sqr_pt4d_pt4d(&obj, &centers[cluster]);
			if (distance < curr_distance)
			{
				curr_distance = distance;
				curr_cluster = cluster;
			}
		}

		/* Store the nearest cluster this object is in */
		if (clusters[i] != curr_cluster)
		{
			converged = LW_FALSE;
			clusters[i] = curr_cluster;
		}
	}
	return converged;
}

static void
update_means(POINT4D *objs, int *clusters, uint32_t n, POINT4D *centers, uint32_t k)
{
	memset(centers, 0, sizeof(POINT4D) * k);
	for (uint32_t i = 0; i < n; i++)
	{
		int cluster = clusters[i];
		centers[cluster].x += objs[i].x * objs[i].m;
		centers[cluster].y += objs[i].y * objs[i].m;
		centers[cluster].z += objs[i].z * objs[i].m;
		centers[cluster].m += objs[i].m;
	}
	for (uint32_t i = 0; i < k; i++)
	{
		if (centers[i].m)
		{
			centers[i].x /= centers[i].m;
			centers[i].y /= centers[i].m;
			centers[i].z /= centers[i].m;
		}
	}
}

static uint8_t
kmeans(POINT4D *objs, int *clusters, uint32_t n, POINT4D *centers, uint32_t k)
{
	uint8_t converged = LW_FALSE;

	for (uint32_t i = 0; i < KMEANS_MAX_ITERATIONS; i++)
	{
		LW_ON_INTERRUPT(break);
		converged = update_r(objs, clusters, n, centers, k);
		if (converged)
			break;
		update_means(objs, clusters, n, centers, k);
	}
	if (!converged)
		lwerror("%s did not converge after %d iterations", __func__, KMEANS_MAX_ITERATIONS);
	return converged;
}

static void
kmeans_init(POINT4D *objs, uint32_t n, POINT4D *centers, uint32_t k)
{
	double *distances;
	uint32_t p1 = 0, p2 = 0;
	uint32_t i, j;
	uint32_t duplicate_count = 1; /* a point is a duplicate of itself */
	double max_dst = -1, current_distance;
	double dst_p1, dst_p2;

	/* k=0, k=1: "clustering" is just input validation */
	assert(k > 1);

	/* k >= 2: find two distant points greedily */
	for (i = 1; i < n; i++)
	{
		/* if we found a larger distance, replace our choice */
		dst_p1 = distance3d_sqr_pt4d_pt4d(&objs[i], &objs[p1]);
		dst_p2 = distance3d_sqr_pt4d_pt4d(&objs[i], &objs[p2]);
		if ((dst_p1 > max_dst) || (dst_p2 > max_dst))
		{
			if (dst_p1 > dst_p2)
			{
				max_dst = dst_p1;
				p2 = i;
			}
			else
			{
				max_dst = dst_p2;
				p1 = i;
			}
		}
		if ((dst_p1 == 0) || (dst_p2 == 0))
			duplicate_count++;
	}
	if (duplicate_count > 1)
		lwnotice(
		    "%s: there are at least %u duplicate inputs, number of output clusters may be less than you requested",
		    __func__,
		    duplicate_count);

	/* by now two points should be found and non-same */
	assert(p1 != p2 && max_dst >= 0);

	/* accept these two points */
	centers[0] = objs[p1];
	centers[1] = objs[p2];

	if (k > 2)
	{
		/* array of minimum distance to a point from accepted cluster centers */
		distances = lwalloc(sizeof(double) * n);

		/* initialize array with distance to first object */
		for (j = 0; j < n; j++)
			distances[j] = distance3d_sqr_pt4d_pt4d(&objs[j], &centers[0]);
		distances[p1] = -1;
		distances[p2] = -1;

		/* loop i on clusters, skip 0 and 1 as found already */
		for (i = 2; i < k; i++)
		{
			uint32_t candidate_center = 0;
			double max_distance = -DBL_MAX;

			/* loop j on objs */
			for (j = 0; j < n; j++)
			{
				/* empty objs and accepted clusters are already marked with distance = -1 */
				if (distances[j] < 0)
					continue;

				/* update minimal distance with previosuly accepted cluster */
				current_distance = distance3d_sqr_pt4d_pt4d(&objs[j], &centers[i - 1]);
				if (current_distance < distances[j])
					distances[j] = current_distance;

				/* greedily take a point that's farthest from any of accepted clusters */
				if (distances[j] > max_distance)
				{
					candidate_center = j;
					max_distance = distances[j];
				}
			}

			/* Checked earlier by counting entries on input, just in case */
			assert(max_distance >= 0);

			/* accept candidate to centers */
			distances[candidate_center] = -1;
			/* Copy the point coordinates into the initial centers array
			 * Centers array is an array of pointers to points, not an array of points */
			centers[i] = objs[candidate_center];
		}
		lwfree(distances);
	}
}

int *
lwgeom_cluster_kmeans(const LWGEOM **geoms, uint32_t n, uint32_t k)
{
	uint32_t num_non_empty = 0;
	uint8_t converged = LW_FALSE;

	assert(k > 0);
	assert(n > 0);
	assert(geoms);

	if (n < k)
	{
		lwerror(
		    "%s: number of geometries is less than the number of clusters requested, not all clusters will get data",
		    __func__);
		k = n;
	}

	/* An array of objects to be analyzed. */
	POINT4D *objs = lwalloc(sizeof(POINT4D) * n);

	/* Array to mark unclusterable objects. Will be returned as KMEANS_NULL_CLUSTER. */
	uint8_t *geom_valid = lwalloc(sizeof(uint8_t) * n);
	memset(geom_valid, 0, sizeof(uint8_t) * n);

	/* Array to fill in with cluster numbers. */
	int *clusters = lwalloc(sizeof(int) * n);
	memset(clusters, 0, sizeof(int) * n);

	/* An array of clusters centers for the algorithm. */
	POINT4D *centers = lwalloc(sizeof(POINT4D) * k);
	memset(centers, 0, sizeof(POINT4D) * k);

	/* Prepare the list of object pointers for K-means */
	for (uint32_t i = 0; i < n; i++)
	{
		const LWGEOM *geom = geoms[i];
		/* Unset M values will be 1 */
		POINT4D out = {0, 0, 0, 1};

		/* Null/empty geometries get geom_valid=LW_FALSE set earlier with memset */
		if ((!geom) || lwgeom_is_empty(geom))
			continue;

		/* If the input is a point, use its coordinates */
		if (lwgeom_get_type(geom) == POINTTYPE)
		{
			out.x = lwpoint_get_x(lwgeom_as_lwpoint(geom));
			out.y = lwpoint_get_y(lwgeom_as_lwpoint(geom));
			if (lwgeom_has_z(geom))
				out.z = lwpoint_get_z(lwgeom_as_lwpoint(geom));
			if (lwgeom_has_m(geom))
			{
				out.m = lwpoint_get_m(lwgeom_as_lwpoint(geom));
				if (out.m <= 0)
					lwerror("%s has an input point geometry with weight in M less or equal to 0",
						__func__);
			}
		}
		else if (!lwgeom_has_z(geom))
		{
			/* For 2D, we can take a centroid*/
			LWGEOM *centroid = lwgeom_centroid(geom);
			if (!centroid)
				continue;
			if (lwgeom_is_empty(centroid))
			{
				lwgeom_free(centroid);
				continue;
			}
			out.x = lwpoint_get_x(lwgeom_as_lwpoint(centroid));
			out.y = lwpoint_get_y(lwgeom_as_lwpoint(centroid));
			lwgeom_free(centroid);
		}
		else
		{
			/* For 3D non-point, we can have a box center */
			const GBOX *box = lwgeom_get_bbox(geom);
			if (!gbox_is_valid(box))
				continue;
			out.x = (box->xmax + box->xmin) / 2;
			out.y = (box->ymax + box->ymin) / 2;
			out.z = (box->zmax + box->zmin) / 2;
		}
		geom_valid[i] = LW_TRUE;
		objs[num_non_empty++] = out;
	}

	if (num_non_empty < k)
	{
		lwnotice(
		    "%s: number of non-empty geometries (%d) is less than the number of clusters (%d) requested, not all clusters will get data",
		    __func__,
		    num_non_empty,
		    k);
		k = num_non_empty;
	}

	if (k > 1)
	{
		int *clusters_dense = lwalloc(sizeof(int) * num_non_empty);
		memset(clusters_dense, 0, sizeof(int) * num_non_empty);

		kmeans_init(objs, num_non_empty, centers, k);
		converged = kmeans(objs, clusters_dense, num_non_empty, centers, k);

		if (converged)
		{
			uint32_t d = 0;
			for (uint32_t i = 0; i < n; i++)
				if (geom_valid[i])
					clusters[i] = clusters_dense[d++];
				else
					clusters[i] = KMEANS_NULL_CLUSTER;
		}
		lwfree(clusters_dense);
	}
	else if (k == 0)
	{
		/* k=0: everything is unclusterable */
		for (uint32_t i = 0; i < n; i++)
			clusters[i] = KMEANS_NULL_CLUSTER;
		converged = LW_TRUE;
	}
	else
	{
		/* k=1: mark up NULL and non-NULL */
		for (uint32_t i = 0; i < n; i++)
		{
			if (!geom_valid[i])
				clusters[i] = KMEANS_NULL_CLUSTER;
			else
				clusters[i] = 0;
		}
		converged = LW_TRUE;
	}

	/* Before error handling, might as well clean up all the inputs */
	lwfree(objs);
	lwfree(centers);
	lwfree(geom_valid);

	/* Good result */
	if (converged)
		return clusters;

	/* Bad result, not going to need the answer */
	lwfree(clusters);
	return NULL;
}
