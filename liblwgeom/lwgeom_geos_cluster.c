/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <string.h>
#include "liblwgeom.h"
#include "lwgeom_log.h"
#include "lwgeom_geos.h"
#include "lwunionfind.h"

struct UnionContext
{
	UNIONFIND* uf;
	char error;
	size_t* p;
	const GEOSPreparedGeometry* prep;
	GEOSGeometry** geoms;
};

struct STRTree
{
	GEOSSTRtree* tree;
	GEOSGeometry** envelopes;
	int* geom_ids;
	int num_geoms;
};

static struct STRTree make_strtree(GEOSGeometry** geoms, int num_geoms);
static void destroy_strtree(struct STRTree tree);
static void union_if_intersecting(void* item, void* userdata);
static int union_intersecting_pairs(GEOSGeometry** geoms, int num_geoms, UNIONFIND* uf);

static struct STRTree
make_strtree(GEOSGeometry** geoms, int num_geoms)
{
	struct STRTree tree;
	tree.tree = GEOSSTRtree_create(num_geoms);
	if (tree.tree == NULL)
	{
		return tree;
	}
	tree.envelopes = lwalloc(num_geoms * sizeof(GEOSGeometry*));
	tree.geom_ids  = lwalloc(num_geoms * sizeof(int));
	tree.num_geoms = num_geoms;

	size_t i;
	for (i = 0; i < num_geoms; i++)
	{
		tree.geom_ids[i] = i;
		tree.envelopes[i] = GEOSEnvelope(geoms[i]);
		GEOSSTRtree_insert(tree.tree, tree.envelopes[i], &(tree.geom_ids[i]));
	}
	return tree;
}

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
}

static void
union_if_intersecting(void* item, void* userdata)
{
	struct UnionContext *cxt = userdata;
	if (cxt->error)
	{
		return;
	}
	size_t q = *((size_t*) item);
	size_t p = *(cxt->p);

	if (p != q && UF_find(cxt->uf, p) != UF_find(cxt->uf, q))
	{
		/* Lazy initialize prepared geometry */
		if (cxt->prep == NULL)
		{
			cxt->prep = GEOSPrepare(cxt->geoms[p]);
		}
		int geos_result = GEOSPreparedIntersects(cxt->prep, cxt->geoms[q]);
		if (geos_result > 1)
		{
			cxt->error = geos_result;
			return;
		}
		if (geos_result)
		{
			UF_union(cxt->uf, p, q);
		}
	}
}

static int
union_intersecting_pairs(GEOSGeometry** geoms, int num_geoms, UNIONFIND* uf)
{
	size_t i;

	/* Identify intersecting geometries and mark them as being in the
	 * same set */
	struct STRTree tree = make_strtree(geoms, num_geoms);
	if (tree.tree == NULL)
	{
		destroy_strtree(tree);
		return LW_FAILURE;
	}
	for (i = 0; i < num_geoms; i++)
	{
		struct UnionContext cxt =
		{
			.uf = uf,
			.error = 0,
			.p = &i,
			.prep = NULL,
			.geoms = geoms
		};
		GEOSGeometry* query_envelope = GEOSEnvelope(geoms[i]);
		GEOSSTRtree_query(tree.tree, query_envelope, &union_if_intersecting, &cxt);

		GEOSGeom_destroy(query_envelope);
		GEOSPreparedGeom_destroy(cxt.prep);
		if (cxt.error)
		{
			return LW_FAILURE;
		}
	}
	destroy_strtree(tree);

	return LW_SUCCESS;
}

int
cluster_intersecting(GEOSGeometry** geoms, uint32_t num_geoms, GEOSGeometry*** clusterGeoms, uint32_t* num_clusters)
{
	size_t i, j, k;
	UNIONFIND* uf = UF_create(num_geoms);

	if (union_intersecting_pairs(geoms, num_geoms, uf) == LW_FAILURE)
	{
		UF_destroy(uf);
		return LW_FAILURE;
	}

	/* Combine components of each cluster into their own GeometryCollection */
	*num_clusters = uf->num_clusters;
	*clusterGeoms = lwalloc(*num_clusters * sizeof(GEOSGeometry*));

	GEOSGeometry **geoms_in_cluster = lwalloc(num_geoms * sizeof(GEOSGeometry*));
	int* ordered_components = UF_ordered_by_cluster(uf);
	for (i = 0, j = 0, k = 0; i < num_geoms; i++)
	{
		geoms_in_cluster[j++] = geoms[ordered_components[i]];

		/* Is this the last geometry in the component? */
		if ((i == num_geoms - 1) ||
		        (UF_find(uf, ordered_components[i]) != UF_find(uf, ordered_components[i+1])))
		{
			(*clusterGeoms)[k++] = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, geoms_in_cluster, j);
			j = 0;
		}
	}

	lwfree(geoms_in_cluster);
	lwfree(ordered_components);
	UF_destroy(uf);

	return LW_SUCCESS;
}
