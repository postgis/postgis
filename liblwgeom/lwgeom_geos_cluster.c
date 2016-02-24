/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
 *
 **********************************************************************/

#include <string.h>
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "lwgeom_geos.h"
#include "lwunionfind.h"

static const int STRTREE_NODE_CAPACITY = 10;

/* Utility struct used to accumulate geometries in GEOSSTRtree_query callback */
struct QueryContext
{
	void** items_found;
	uint32_t items_found_size;
	uint32_t num_items_found;
};

/* Utility struct to keep GEOSSTRtree and associated structures to be freed after use */
struct STRTree
{
	GEOSSTRtree* tree;
	GEOSGeometry** envelopes;
	uint32_t* geom_ids;
	uint32_t num_geoms;
};

static struct STRTree make_strtree(void** geoms, uint32_t num_geoms, char is_lwgeom);
static void destroy_strtree(struct STRTree tree);
static int union_intersecting_pairs(GEOSGeometry** geoms, uint32_t num_geoms, UNIONFIND* uf);
static int combine_geometries(UNIONFIND* uf, void** geoms, uint32_t num_geoms, void*** clustersGeoms, uint32_t* num_clusters, char is_lwgeom);

/** Make a GEOSSTRtree of either GEOSGeometry* or LWGEOM* pointers */
static struct STRTree
make_strtree(void** geoms, uint32_t num_geoms, char is_lwgeom)
{
	struct STRTree tree;
	tree.tree = GEOSSTRtree_create(STRTREE_NODE_CAPACITY);
	if (tree.tree == NULL)
	{
		return tree;
	}
	tree.envelopes = lwalloc(num_geoms * sizeof(GEOSGeometry*));
	tree.geom_ids  = lwalloc(num_geoms * sizeof(uint32_t));
	tree.num_geoms = num_geoms;

	size_t i;
	for (i = 0; i < num_geoms; i++)
	{
		tree.geom_ids[i] = i;
		if (!is_lwgeom)
		{
			tree.envelopes[i] = GEOSEnvelope(geoms[i]);
		}
		else
		{
			const GBOX* box = lwgeom_get_bbox(geoms[i]);
			if (box)
			{
				tree.envelopes[i] = GBOX2GEOS(box);
			} 
			else
			{
				/* Empty geometry */
				tree.envelopes[i] = GEOSGeom_createEmptyPolygon();
			}
		}
		GEOSSTRtree_insert(tree.tree, tree.envelopes[i], &(tree.geom_ids[i]));
	}
	return tree;
}

/** Clean up STRTree after use */
static void
destroy_strtree(struct STRTree tree)
{
	size_t i;
	GEOSSTRtree_destroy(tree.tree);

	for (i = 0; i < tree.num_geoms; i++)
	{
		GEOSGeom_destroy(tree.envelopes[i]);
	}
	lwfree(tree.geom_ids);
	lwfree(tree.envelopes);
}

static void
query_accumulate(void* item, void* userdata)
{
	struct QueryContext *cxt = userdata;
	if (!cxt->items_found)
	{
		cxt->items_found_size = 8;
		cxt->items_found = lwalloc(cxt->items_found_size * sizeof(void*));
	}

	if (cxt->num_items_found >= cxt->items_found_size)
	{
		cxt->items_found_size = 2 * cxt->items_found_size;
		cxt->items_found = lwrealloc(cxt->items_found, cxt->items_found_size * sizeof(void*));
	}
	cxt->items_found[cxt->num_items_found++] = item;
}

/* Identify intersecting geometries and mark them as being in the same set */
static int
union_intersecting_pairs(GEOSGeometry** geoms, uint32_t num_geoms, UNIONFIND* uf)
{
	uint32_t p, i;
	struct STRTree tree;
	struct QueryContext cxt =
	{
		.items_found = NULL,
		.num_items_found = 0,
		.items_found_size = 0
	};
	int success = LW_SUCCESS;

	if (num_geoms <= 1)
		return LW_SUCCESS;

	tree = make_strtree((void**) geoms, num_geoms, 0);
	if (tree.tree == NULL)
	{
		destroy_strtree(tree);
		return LW_FAILURE;
	}

	for (p = 0; p < num_geoms; p++)
	{
		const GEOSPreparedGeometry* prep = NULL;
		GEOSGeometry* query_envelope;

		if (GEOSisEmpty(geoms[p]))
			continue;

		cxt.num_items_found = 0;
		query_envelope = GEOSEnvelope(geoms[p]);
		GEOSSTRtree_query(tree.tree, query_envelope, &query_accumulate, &cxt);

		for (i = 0; i < cxt.num_items_found; i++)
		{
			uint32_t q = *((uint32_t*) cxt.items_found[i]);

			if (p != q && UF_find(uf, p) != UF_find(uf, q))
			{
				int geos_type = GEOSGeomTypeId(geoms[p]);
				int geos_result;

				/* Don't build prepared a geometry around a Point or MultiPoint -
				 * there are some problems in the implementation, and it's not clear
				 * there would be a performance benefit in any case.  (See #3433)
				 */
				if (geos_type != GEOS_POINT && geos_type != GEOS_MULTIPOINT)
				{
					/* Lazy initialize prepared geometry */
					if (prep == NULL)
					{
						prep = GEOSPrepare(geoms[p]);
					}
					geos_result = GEOSPreparedIntersects(prep, geoms[q]);
				}
				else
				{
					geos_result = GEOSIntersects(geoms[p], geoms[q]);
				}
				if (geos_result > 1)
				{
					success = LW_FAILURE;
					break;
				}
				else if (geos_result)
				{
					UF_union(uf, p, q);
				}
			}
		}

		if (prep)
			GEOSPreparedGeom_destroy(prep);
		GEOSGeom_destroy(query_envelope);

		if (!success)
			break;
	}

	if (cxt.items_found)
		lwfree(cxt.items_found);

	destroy_strtree(tree);
	return success;
}

/** Takes an array of GEOSGeometry* and constructs an array of GEOSGeometry*, where each element in the constructed
 *  array is a GeometryCollection representing a set of interconnected geometries. Caller is responsible for
 *  freeing the input array, but not for destroying the GEOSGeometry* items inside it.  */
int
cluster_intersecting(GEOSGeometry** geoms, uint32_t num_geoms, GEOSGeometry*** clusterGeoms, uint32_t* num_clusters)
{
	int cluster_success;
	UNIONFIND* uf = UF_create(num_geoms);

	if (union_intersecting_pairs(geoms, num_geoms, uf) == LW_FAILURE)
	{
		UF_destroy(uf);
		return LW_FAILURE;
	}

	cluster_success = combine_geometries(uf, (void**) geoms, num_geoms, (void***) clusterGeoms, num_clusters, 0);
	UF_destroy(uf);
	return cluster_success;
}

/** Takes an array of LWGEOM* and constructs an array of LWGEOM*, where each element in the constructed array is a
 *  GeometryCollection representing a set of geometries separated by no more than the specified tolerance. Caller is
 *  responsible for freeing the input array, but not the LWGEOM* items inside it. */
int
cluster_within_distance(LWGEOM** geoms, uint32_t num_geoms, double tolerance, LWGEOM*** clusterGeoms, uint32_t* num_clusters)
{
	int min_points = 1;
	return cluster_dbscan(geoms, num_geoms, tolerance, min_points, clusterGeoms, num_clusters);
}


static int
dbscan_update_context(GEOSSTRtree* tree, struct QueryContext* cxt, LWGEOM** geoms, uint32_t p, double eps)
{
	cxt->num_items_found = 0;

	const GBOX* geom_extent = lwgeom_get_bbox(geoms[p]);
	GBOX* query_extent = gbox_clone(geom_extent);
	gbox_expand(query_extent, eps);
	GEOSGeometry* query_envelope = GBOX2GEOS(query_extent);

	if (!query_envelope)
		return LW_FAILURE;

	GEOSSTRtree_query(tree, query_envelope, &query_accumulate, cxt);

	lwfree(query_extent);
	GEOSGeom_destroy(query_envelope);

	return LW_SUCCESS;
}

int
union_dbscan(LWGEOM** geoms, uint32_t num_geoms, UNIONFIND* uf, double eps, uint32_t min_points)
{
	uint32_t p, i;
	struct STRTree tree;
	struct QueryContext cxt =
	{
		.items_found = NULL,
		.num_items_found = 0,
		.items_found_size = 0
	};
	int success = LW_SUCCESS;

	if (num_geoms <= 1)
		return LW_SUCCESS;

	tree = make_strtree((void**) geoms, num_geoms, 1);
	if (tree.tree == NULL)
	{
		destroy_strtree(tree);
		return LW_FAILURE;
	}

	for (p = 0; p < num_geoms; p++)
	{
		uint32_t num_neighbors = 0;
		uint32_t* neighbors;

		if (lwgeom_is_empty(geoms[p]))
			continue;

		dbscan_update_context(tree.tree, &cxt, geoms, p, eps);
		neighbors = lwalloc(cxt.num_items_found * sizeof(uint32_t*));

		for (i = 0; i < cxt.num_items_found; i++)
		{
			uint32_t q = *((uint32_t*) cxt.items_found[i]);

			double mindist = lwgeom_mindistance2d_tolerance(geoms[p], geoms[q], eps);
			if (mindist == FLT_MAX)
			{
				success = LW_FAILURE;
				break;
			}

			if (mindist <= eps)
				neighbors[num_neighbors++] = q;
		}

		if (num_neighbors >= min_points)
		{
			for (i = 0; i < num_neighbors; i++)
			{
				UF_union(uf, p, neighbors[i]);
			}
		}

		lwfree(neighbors);

		if (!success)
			break;
	}

	if (cxt.items_found)
		lwfree(cxt.items_found);

	destroy_strtree(tree);
	return success;
}

int
cluster_dbscan(LWGEOM** geoms, uint32_t num_geoms, double eps, uint32_t min_points, LWGEOM*** clusterGeoms, uint32_t* num_clusters)
{
	int cluster_success;
	UNIONFIND* uf = UF_create(num_geoms);

	if (union_dbscan(geoms, num_geoms, uf, eps, min_points) == LW_FAILURE)
	{
		UF_destroy(uf);
		return LW_FAILURE;
	}

	cluster_success = combine_geometries(uf, (void**) geoms, num_geoms, (void***) clusterGeoms, num_clusters, 1);
	UF_destroy(uf);
	return cluster_success;
}

/** Uses a UNIONFIND to identify the set with which each input geometry is associated, and groups the geometries into
 *  GeometryCollections.  Supplied geometry array may be of either LWGEOM* or GEOSGeometry*; is_lwgeom is used to
 *  identify which. Caller is responsible for freeing input geometry array but not the items contained within it. */
static int
combine_geometries(UNIONFIND* uf, void** geoms, uint32_t num_geoms, void*** clusterGeoms, uint32_t* num_clusters, char is_lwgeom)
{
	size_t i, j, k;

	/* Combine components of each cluster into their own GeometryCollection */
	*num_clusters = uf->num_clusters;
	*clusterGeoms = lwalloc(*num_clusters * sizeof(void*));

	void** geoms_in_cluster = lwalloc(num_geoms * sizeof(void*));
	uint32_t* ordered_components = UF_ordered_by_cluster(uf);
	for (i = 0, j = 0, k = 0; i < num_geoms; i++)
	{
		geoms_in_cluster[j++] = geoms[ordered_components[i]];

		/* Is this the last geometry in the component? */
		if ((i == num_geoms - 1) ||
		        (UF_find(uf, ordered_components[i]) != UF_find(uf, ordered_components[i+1])))
		{
			if (k >= uf->num_clusters) {
				/* Should not get here - it means that we have more clusters than uf->num_clusters thinks we should. */
				return LW_FAILURE;
			}

			if (is_lwgeom)
			{
				LWGEOM** components = lwalloc(j * sizeof(LWGEOM*));
				memcpy(components, geoms_in_cluster, j * sizeof(LWGEOM*));
				(*clusterGeoms)[k++] = lwcollection_construct(COLLECTIONTYPE, components[0]->srid, NULL, j, (LWGEOM**) components);
			}
			else
			{
				(*clusterGeoms)[k++] = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, (GEOSGeometry**) geoms_in_cluster, j);
			}
			j = 0;
		}
	}

	lwfree(geoms_in_cluster);
	lwfree(ordered_components);

	return LW_SUCCESS;
}
