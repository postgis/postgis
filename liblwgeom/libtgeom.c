/**********************************************************************
 * $Id:$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>

#include "liblwgeom_internal.h"
#include "libtgeom.h"


/*
 * Create a new empty tgeom struct
 * Return a pointer on the newly allocated struct
 */
TGEOM*
tgeom_new(uchar type, int hasz, int hasm)
{
	TGEOM *tgeom;

	tgeom = lwalloc(sizeof(TGEOM));
	tgeom->type = type;
	FLAGS_SET_Z(tgeom->flags, hasz);
	FLAGS_SET_M(tgeom->flags, hasm);
	tgeom->bbox=NULL;
	tgeom->srid=0;
	tgeom->nedges=0;
	tgeom->maxedges=0;
	tgeom->edges=NULL;
	tgeom->maxfaces=0;
	tgeom->nfaces=0;
	tgeom->faces=NULL;

	return tgeom;
}


/*
 * Check if a edge (start and end points) are or not already
 * in a given tgeom.
 * Return 0 if not in.
 * Return positive index edge number if the edge is well oriented
 * Return negative index edge number if the edge is reversed
 */
static int
tgeom_is_edge(const TGEOM *tgeom, const POINT4D *s, const POINT4D *e)
{
	int i, hasz, hasm;
	POINT4D *p1, *p2;

	assert(tgeom);
	assert(s);
	assert(e);

	hasz = FLAGS_GET_Z(tgeom->flags);
	hasm = FLAGS_GET_M(tgeom->flags);

	LWDEBUGF(3, "To check [%lf,%lf,%lf,%lf] -> [%lf,%lf,%lf,%lf]\n",
	         s->x, s->y, s->z, s->m, e->x, e->y, e->z, e->m);

	for (i=1 ; i <= tgeom->nedges ; i++)  /* edges are 1 based */
	{
		p1 = tgeom->edges[i]->s;
		p2 = tgeom->edges[i]->e;

		LWDEBUGF(3, "[%i]/[%i]  (%lf,%lf,%lf,%lf) -> (%lf,%lf,%lf,%lf)\n",
		         i, tgeom->nedges,
		         p1->x, p1->y, p1->z, p1->m,
		         p2->x, p2->y, p2->z, p2->m);

		/* X,Y,Z,M */
		if (hasz && hasm)
		{
			if (	p1->x == e->x && p1->y == e->y && p1->z == e->z && p1->m == e->m &&
			        p2->x == s->x && p2->y == s->y && p2->z == s->z && p2->m == s->m)
				return -i;

			if (    p1->x == s->x && p1->y == s->y && p1->z == s->z && p1->m == s->m &&
			        p2->x == e->x && p2->y == e->y && p2->z == e->z && p2->m == e->m)
				return i;
		}
		/* X,Y,Z */
		else if (hasz && !hasm)
		{
			if (	p1->x == e->x && p1->y == e->y && p1->z == e->z &&
			        p2->x == s->x && p2->y == s->y && p2->z == s->z )
				return -i;

			if (    p1->x == s->x && p1->y == s->y && p1->z == s->z &&
			        p2->x == e->x && p2->y == e->y && p2->z == e->z )
				return i;
		}
		/* X,Y,M */
		else if (!hasz && hasm)
		{
			if (	p1->x == e->x && p1->y == e->y && p1->m == e->m &&
			        p2->x == s->x && p2->y == s->y && p2->m == s->m )
				return -i;

			if (    p1->x == s->x && p1->y == s->y && p1->m == s->m &&
			        p2->x == e->x && p2->y == e->y && p2->m == e->m )
				return i;
		}
		else  /*X,Y */
		{
			if (	p1->x == e->x && p1->y == e->y &&
			        p2->x == s->x && p2->y == s->y)
				return -i;

			if (    p1->x == s->x && p1->y == s->y &&
			        p2->x == e->x && p2->y == e->y)
				return i;
		}
	}

	LWDEBUG(3, "Edge not found in array");

	return 0;
}


/*
 * Add an edge to a face in a tgeom
 * Edge is describded as a starting and an ending point
 * Points are really copied
 * Return the new tgeom pointer
 */
static TGEOM*
tgeom_add_face_edge(TGEOM *tgeom, int face_id, POINT4D *s, POINT4D *e)
{
	int nedges, edge_id;

	assert(tgeom);
	assert(s);
	assert(e);

	edge_id = tgeom_is_edge(tgeom, s, e);

	if (edge_id)
	{
		tgeom->edges[abs(edge_id)]->count++;
		LWDEBUGF(3, "face [%i] Founded Edge: %i\n",
		         face_id, edge_id);
	}
	else
	{
		if ((tgeom->nedges + 1) == INT_MAX)
			lwerror("tgeom_add_face_edge: Unable to alloc more than %i edges", INT_MAX);

		/* alloc edges array */
		if (tgeom->maxedges == 0)
		{
			tgeom->edges = (TEDGE**) lwalloc(sizeof(TEDGE*) * 4);
			tgeom->maxedges = 4;
		}
		if (tgeom->maxedges >= (tgeom->nedges + 1))
		{
			tgeom->edges = (TEDGE **) lwrealloc(tgeom->edges,
			                         sizeof(TEDGE*) * tgeom->maxedges * 2);
			tgeom->maxedges *= 2;
		}

		edge_id = ++tgeom->nedges; /* edge_id is 1 based */
		tgeom->edges[edge_id] = (TEDGE*) lwalloc(sizeof(TEDGE));
		tgeom->edges[edge_id]->s = lwalloc(sizeof(POINT4D));
		tgeom->edges[edge_id]->e = lwalloc(sizeof(POINT4D));
		memcpy(tgeom->edges[edge_id]->s, s, sizeof(POINT4D));
		memcpy(tgeom->edges[edge_id]->e, e, sizeof(POINT4D));
		tgeom->edges[edge_id]->count = 1;

		LWDEBUGF(3, "face [%i] adding edge [%i] (%lf, %lf, %lf, %lf) -> (%lf, %lf, %lf, %lf)\n",
		         face_id, edge_id, s->x, s->y, s->z, s->m, e->x, e->y, e->z, e->m);
	}

	nedges = tgeom->faces[face_id]->nedges;
	if (tgeom->faces[face_id]->maxedges == 0)
	{
		tgeom->faces[face_id]->edges = (int *) lwalloc(sizeof(int) * 4);
		tgeom->faces[face_id]->maxedges = 4;
	}
	if (tgeom->faces[face_id]->maxedges == nedges)
	{
		tgeom->faces[face_id]->edges = (int *) lwrealloc(tgeom->faces[face_id]->edges,
		                               sizeof(int) * tgeom->faces[face_id]->maxedges * 2);
		tgeom->faces[face_id]->maxedges *= 2;
	}

	LWDEBUGF(3, "face [%i] add %i in edge array in %i pos\n",
	         face_id, edge_id, tgeom->faces[face_id]->nedges);

	tgeom->faces[face_id]->edges[nedges]= edge_id;
	tgeom->faces[face_id]->nedges++;

	return tgeom;
}


/*
 * Add a LWPOLY inside a tgeom
 * Copy geometries from LWPOLY
 */
static TGEOM*
tgeom_add_polygon(TGEOM *tgeom, LWPOLY *poly)
{
	int i;

	assert(tgeom);
	assert(poly);

	if ((tgeom->nfaces + 1) == INT_MAX)
		lwerror("tgeom_add_polygon: Unable to alloc more than %i faces", INT_MAX);

	/* Integrity checks on subgeom, dims and srid */
	if (tgeom->type != POLYHEDRALSURFACETYPE)
		lwerror("tgeom_add_polygon: Unable to handle %s - %s type",
		        tgeom->type, lwtype_name(tgeom->type));

	if (FLAGS_NDIMS(tgeom->flags) != FLAGS_NDIMS(poly->flags))
		lwerror("tgeom_add_polygon: Mixed dimension");

	if (tgeom->srid != poly->srid && (tgeom->srid != 0 && poly->srid != SRID_UNKNOWN))
		lwerror("tgeom_add_polygon: Mixed srid. Tgeom: %i / Polygon: %i",
		        tgeom->srid, poly->srid);

	/* handle face array allocation */
	if (tgeom->maxfaces == 0)
	{
		tgeom->faces = lwalloc(sizeof(TFACE*) * 2);
		tgeom->maxfaces = 2;
	}
	if ((tgeom->maxfaces - 1) == tgeom->nfaces)
	{
		tgeom->faces = lwrealloc(tgeom->faces,
		                         sizeof(TFACE*) * tgeom->maxfaces * 2);
		tgeom->maxfaces *= 2;
	}

	/* add an empty face */
	tgeom->faces[tgeom->nfaces] = lwalloc(sizeof(TFACE));
	tgeom->faces[tgeom->nfaces]->rings = NULL;
	tgeom->faces[tgeom->nfaces]->nrings = 0;
	tgeom->faces[tgeom->nfaces]->nedges = 0;
	tgeom->faces[tgeom->nfaces]->maxedges = 0;

	/* Compute edge on poly external ring */
	for (i=1 ; i < poly->rings[0]->npoints ; i++)
	{
		POINT4D p1, p2;

		getPoint4d_p(poly->rings[0], i-1, &p1);
		getPoint4d_p(poly->rings[0], i,   &p2);
		tgeom_add_face_edge(tgeom, tgeom->nfaces, &p1, &p2);
	}

	/* External ring is already handled by edges */
	tgeom->faces[tgeom->nfaces]->nrings = poly->nrings - 1;

	/* handle rings array allocation */
	if (tgeom->faces[tgeom->nfaces]->nrings >= 1)
		tgeom->faces[tgeom->nfaces]->rings = lwalloc(sizeof(POINTARRAY*)
		                                     * tgeom->faces[tgeom->nfaces]->nrings);

	/* clone internal rings */
	for (i=0 ; i < tgeom->faces[tgeom->nfaces]->nrings ; i++)
		tgeom->faces[tgeom->nfaces]->rings[i]
		= ptarray_clone_deep(poly->rings[i+1]);

	tgeom->nfaces++;

	return tgeom;
}


/*
 * Add a LWTRIANGLE inside a tgeom
 * Copy geometries from LWTRIANGLE
 */
static TGEOM*
tgeom_add_triangle(TGEOM *tgeom, LWTRIANGLE *triangle)
{
	int i;

	assert(tgeom);
	assert(triangle);

	if ((tgeom->nfaces + 1) == INT_MAX)
		lwerror("tgeom_add_triangle: Unable to alloc more than %i faces", INT_MAX);

	/* Integrity checks on subgeom, dims and srid */
	if (tgeom->type != TINTYPE)
		lwerror("tgeom_add_triangle: Unable to handle %s - %s type",
		        tgeom->type, lwtype_name(tgeom->type));

	if (FLAGS_NDIMS(tgeom->flags) != FLAGS_NDIMS(triangle->flags))
		lwerror("tgeom_add_triangle: Mixed dimension");

	if (tgeom->srid != triangle->srid
	        && (tgeom->srid != 0 && triangle->srid != SRID_UNKNOWN))
		lwerror("tgeom_add_triangle: Mixed srid. Tgeom: %i / Triangle: %i",
		        tgeom->srid, triangle->srid);

	/* handle face array allocation */
	if (tgeom->maxfaces == 0)
	{
		tgeom->faces = lwalloc(sizeof(TFACE*) * 2);
		tgeom->maxfaces = 2;
	}
	if ((tgeom->maxfaces - 1) == tgeom->nfaces)
	{
		tgeom->faces = lwrealloc(tgeom->faces,
		                         sizeof(TFACE*) * tgeom->maxfaces * 2);
		tgeom->maxfaces *= 2;
	}

	/* add an empty face */
	tgeom->faces[tgeom->nfaces] = lwalloc(sizeof(TFACE));
	tgeom->faces[tgeom->nfaces]->rings = NULL;
	tgeom->faces[tgeom->nfaces]->nrings = 0;
	tgeom->faces[tgeom->nfaces]->nedges = 0;
	tgeom->faces[tgeom->nfaces]->maxedges = 0;

	/* Compute edge on triangle */
	for (i=1 ; i < triangle->points->npoints ; i++)
	{
		POINT4D p1, p2;

		getPoint4d_p(triangle->points, i-1, &p1);
		getPoint4d_p(triangle->points, i,   &p2);

		tgeom_add_face_edge(tgeom, tgeom->nfaces, &p1, &p2);
	}

	tgeom->nfaces++;

	return tgeom;
}


/*
 * Free a tgeom struct
 * and even the geometries inside
 */
void
tgeom_free(TGEOM *tgeom)
{
	int i, j;

	assert(tgeom);

	/* bbox */
	if (tgeom->bbox) lwfree(tgeom->bbox);

	/* edges */
	for (i=1 ; i <= tgeom->nedges ; i++)
	{
		lwfree(tgeom->edges[i]->e);
		lwfree(tgeom->edges[i]->s);
		lwfree(tgeom->edges[i]);
	}
	if (tgeom->edges) lwfree(tgeom->edges);

	/* faces */
	for (i=0 ; i < tgeom->nfaces ; i++)
	{
		/* edges array */
		if (tgeom->faces[i]->edges)
			lwfree(tgeom->faces[i]->edges);

		/* rings */
		for (j=0 ; j < tgeom->faces[i]->nrings ; j++)
			ptarray_free(tgeom->faces[i]->rings[j]);
		if (tgeom->faces[i]->rings)
			lwfree(tgeom->faces[i]->rings);

		lwfree(tgeom->faces[i]);
	}
	if (tgeom->faces) lwfree(tgeom->faces);

	lwfree(tgeom);
}


/*
 * Return a TSERIALIZED pointer from an LWGEOM
 */
TSERIALIZED*
tserialized_from_lwgeom(LWGEOM *lwgeom)
{
	assert(lwgeom);

	return tgeom_serialize(tgeom_from_lwgeom(lwgeom));
}


/*
 * Return a TGEOM pointer from an LWGEOM
 * Caution: Geometries from LWGEOM are copied
 */
TGEOM*
tgeom_from_lwgeom(const LWGEOM *lwgeom)
{
	int i, solid;
	LWTIN *tin;
	LWPSURFACE *psurf;
	TGEOM *tgeom = NULL;

	tgeom = tgeom_new(0, FLAGS_GET_Z(lwgeom->flags), FLAGS_GET_M(lwgeom->flags));

	if (lwgeom->srid < 1) tgeom->srid = SRID_UNKNOWN;
	else tgeom->srid = lwgeom->srid;

	if (lwgeom_is_empty(lwgeom)) return tgeom;

	switch (lwgeom->type)
	{
	case TINTYPE:
		tgeom->type = TINTYPE;
		tin = (LWTIN *) lwgeom;

		for (i=0 ; i < tin->ngeoms ; i++)
			tgeom = tgeom_add_triangle(tgeom,
			                           (LWTRIANGLE *) tin->geoms[i]);

		break;

	case POLYHEDRALSURFACETYPE:
		tgeom->type = POLYHEDRALSURFACETYPE;
		psurf = (LWPSURFACE *) lwgeom;
		for (i=0 ; i < psurf->ngeoms ; i++)
			tgeom = tgeom_add_polygon(tgeom,
			                          (LWPOLY *) psurf->geoms[i]);

		break;

		/* TODO handle COLLECTIONTYPE */

	default:
		lwerror("tgeom_from_lwgeom: unknown geometry type %i - %s",
		        tgeom->type, lwtype_name(tgeom->type));
	}



	for (solid=1, i=1 ; i <= tgeom->nedges ; i++)
	{
		if (tgeom->edges[i]->count != 2)
		{
			LWDEBUGF(3, "no solid, edges: [%i], count: [%i] (%lf,%lf,%lf,%lf)->(%lf,%lf,%lf,%lf)\n",
			         i, tgeom->edges[i]->count,
			         tgeom->edges[i]->s->x, tgeom->edges[i]->s->y,
			         tgeom->edges[i]->s->z, tgeom->edges[i]->s->m,
			         tgeom->edges[i]->e->x, tgeom->edges[i]->e->y,
			         tgeom->edges[i]->e->z, tgeom->edges[i]->e->m);

			solid = 0;
			break;
		}
	}
	FLAGS_SET_SOLID(tgeom->flags, solid);

	return tgeom;
}


/*
 * Return a LWGEOM pointer from an TSERIALIZED struct
 */
LWGEOM*
lwgeom_from_tserialized(TSERIALIZED *t)
{
	assert(t);

	return lwgeom_from_tgeom(tgeom_deserialize(t));
}


/*
 * Return a LWGEOM pointer from an TGEOM struct
 * Geometries are NOT copied
 */
LWGEOM*
lwgeom_from_tgeom(TGEOM *tgeom)
{
	int i, j, k;
	LWGEOM *geom;
	POINTARRAY *dpa;
	POINTARRAY **ppa;
	int hasz, hasm, edge_id;
	int dims=0;

	assert(tgeom);

	hasz=FLAGS_GET_Z(tgeom->flags);
	hasm=FLAGS_GET_M(tgeom->flags);

	geom = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, tgeom->srid, hasz, hasm);

	switch (tgeom->type)
	{
	case TINTYPE:
		geom->type = TINTYPE;
		for (i=0 ; i < tgeom->nfaces ; i++)
		{
			FLAGS_SET_Z(dims, hasz?1:0);
			FLAGS_SET_M(dims, hasm?1:0);
			dpa = ptarray_construct_empty(hasz, hasm, 4);

			for (j=0 ; j < tgeom->faces[i]->nedges ; j++)
			{
				edge_id = tgeom->faces[i]->edges[j];
				LWDEBUGF(3, "TIN edge_id: %i\n", edge_id);

				assert(edge_id);
				if (edge_id > 0)
					ptarray_append_point(dpa, tgeom->edges[edge_id]->s, LW_TRUE);
				else
					ptarray_append_point(dpa, tgeom->edges[-edge_id]->e, LW_TRUE);
			}

			edge_id = tgeom->faces[i]->edges[0];
			LWDEBUGF(3, "TIN edge_id: %i\n", edge_id);
			if (edge_id > 0)
				ptarray_append_point(dpa, tgeom->edges[edge_id]->s, LW_TRUE); 
			else
				ptarray_append_point(dpa, tgeom->edges[-edge_id]->e, LW_TRUE); 

			geom = (LWGEOM *) lwtin_add_lwtriangle((LWTIN *) geom,
			                                       lwtriangle_construct(tgeom->srid, NULL, dpa));
		}
		break;

	case POLYHEDRALSURFACETYPE:
		geom->type = POLYHEDRALSURFACETYPE;
		for (i=0 ; i < tgeom->nfaces ; i++)
		{
			FLAGS_SET_Z(dims, hasz?1:0);
			FLAGS_SET_M(dims, hasm?1:0);;
			dpa = ptarray_construct_empty(hasz, hasm, 4);

			for (j=0 ; j < tgeom->faces[i]->nedges ; j++)
			{
				edge_id = tgeom->faces[i]->edges[j];
				assert(edge_id);
				LWDEBUGF(3, "POLYHEDRALSURFACE edge_id: %i\n", edge_id);
				if (edge_id > 0)
					ptarray_append_point(dpa, tgeom->edges[edge_id]->s, LW_TRUE);
				else
					ptarray_append_point(dpa, tgeom->edges[-edge_id]->e, LW_TRUE);
			}

			edge_id = tgeom->faces[i]->edges[0];
			LWDEBUGF(3, "POLYHEDRALSURFACE edge_id: %i\n", edge_id);
			if (edge_id > 0)
				ptarray_append_point(dpa, tgeom->edges[edge_id]->s, LW_TRUE);
			else
				ptarray_append_point(dpa, tgeom->edges[-edge_id]->e, LW_TRUE);

			ppa = lwalloc(sizeof(POINTARRAY*)
			              * (tgeom->faces[i]->nrings + 1));
			ppa[0] = dpa;
			for (k=0; k < tgeom->faces[i]->nrings ; k++)
				ppa[k+1] = tgeom->faces[i]->rings[k];

			geom = (LWGEOM *) lwpsurface_add_lwpoly((LWPSURFACE *) geom,
			                                        lwpoly_construct(tgeom->srid, NULL, k + 1, ppa));
		}
		break;

	default:
		lwerror("lwgeom_from_tgeom: Unkwnown type %i - %s\n",
		        tgeom->type, lwtype_name(tgeom->type));
	}

	if (geom->srid == 0) geom->srid = SRID_UNKNOWN;

	return geom;
}


/*
 * Compute the memory size needed to serialize
 * a TGEOM struct
 */
static size_t
tgeom_serialize_size(const TGEOM *tgeom)
{
	int i,j;
	size_t size;
	int dims = FLAGS_NDIMS(tgeom->flags);

	size = sizeof(uchar);					/* type */
	size += sizeof(uchar);					/* flags */
	size += sizeof(uint32);					/* srid */
	if (tgeom->bbox) size += sizeof(BOX3D);			/* bbox */

	size += sizeof(int);					/* nedges */
	size += (sizeof(double) * dims * 2 + 4) * tgeom->nedges;/* edges */

	size += sizeof(int);					/* nfaces */
	for (i=0 ; i < tgeom->nfaces ; i++)
	{
		size += sizeof(int);				/* nedges */
		size += sizeof(int) * tgeom->faces[i]->nedges;	/* edges */

		size += sizeof(int);				/* nrings */
		for (j=0 ; j < tgeom->faces[i]->nrings ; j++)
		{
			size += sizeof(int);			/* npoints */
			size += sizeof(double) * dims 	 	/* rings */
			        * tgeom->faces[i]->rings[j]->npoints;
		}
	}

	return size;
}


/*
 * Serialize a TGEOM to a buf
 * retsize return by reference the allocated buf size
 */
static size_t
tgeom_serialize_buf(const TGEOM *tgeom, uchar *buf, size_t *retsize)
{
	int i,j;
	size_t size=0;
	uchar *loc=buf;
	int dims = FLAGS_NDIMS(tgeom->flags);

	assert(tgeom);
	assert(buf);

	/* Write in the type. */
	memcpy(loc, &tgeom->type, sizeof(uchar));
	loc  += 1;
	size += 1;

	/* Write in the flags. */
	memcpy(loc, &tgeom->flags, sizeof(uchar));
	loc  += 1;
	size += 1;

	/* Write in the srid. */
	memcpy(loc, &tgeom->srid, sizeof(uint32));
	loc  += 4;
	size += 4;

	/* Write in the bbox. */
	if (tgeom->bbox)
	{
		memcpy(loc, tgeom->bbox, sizeof(BOX3D));
		loc  += sizeof(BOX3D);
		size += sizeof(BOX3D);
	}

	/* Write in the number of edges (0=> EMPTY) */
	memcpy(loc, &tgeom->nedges, sizeof(int));
	loc  += 4;
	size +=4;

	/* Edges */
	for (i=1 ; i <= tgeom->nedges ; i++)
	{
		/* 3DM specific handle */
		if (!FLAGS_GET_Z(tgeom->flags) && FLAGS_GET_M(tgeom->flags))
		{
			memcpy(loc, tgeom->edges[i]->s, 2 * sizeof(double));
			loc  += sizeof(double) * 2;
			memcpy(loc, &(tgeom->edges[i]->s->m), sizeof(double));
			loc  += sizeof(double);

			memcpy(loc, tgeom->edges[i]->e, 2 * sizeof(double));
			loc  += sizeof(double) * 2;
			memcpy(loc, &(tgeom->edges[i]->e->m), sizeof(double));
			loc  += sizeof(double);
		}
		else /* 2D, 3DZ && 4D */
		{
			memcpy(loc, tgeom->edges[i]->s, dims * sizeof(double));
			loc  += sizeof(double) * dims;
			memcpy(loc, tgeom->edges[i]->e, dims * sizeof(double));
			loc  += sizeof(double) * dims;
		}
		memcpy(loc, &tgeom->edges[i]->count, sizeof(int));
		loc  += 4;

		size += sizeof(double) * dims * 2 + 4;
	}

	/* Write in the number of faces */
	memcpy(loc, &tgeom->nfaces, sizeof(int));
	loc  += 4;
	size += 4;

	/* Faces */
	for (i=0 ; i < tgeom->nfaces ; i++)
	{
		/* Write in the number of edges */
		memcpy(loc, &tgeom->faces[i]->nedges, sizeof(int));
		loc  += 4;
		size += 4;

		/* Write in the edges array */
		memcpy(loc, tgeom->faces[i]->edges,
		       sizeof(int) * tgeom->faces[i]->nedges);
		loc  += 4 * tgeom->faces[i]->nedges;
		size += 4 * tgeom->faces[i]->nedges;

		/* Write the number of rings */
		memcpy(loc, &tgeom->faces[i]->nrings, sizeof(int));
		loc  += 4;
		size += 4;

		for (j=0 ; j < tgeom->faces[i]->nrings ; j++)
		{
			/* Write the number of points */
			memcpy(loc, &tgeom->faces[i]->rings[j]->npoints, sizeof(int));
			loc  += 4;
			size += 4;

			/* Write the points */
			memcpy(loc, getPoint_internal(tgeom->faces[i]->rings[j], 0),
			       sizeof(double) * dims * tgeom->faces[i]->rings[j]->npoints);
			loc  += sizeof(double) * dims * tgeom->faces[i]->rings[j]->npoints;
			size += sizeof(double) * dims * tgeom->faces[i]->rings[j]->npoints;
		}
	}

	if (retsize) *retsize = size;

	return (size_t) (loc - buf);
}


/*
 * Serialize a tgeom struct and return a
 * TSERIALIZED pointer
 */
TSERIALIZED *
tgeom_serialize(const TGEOM *tgeom)
{
	size_t size, retsize;
	TSERIALIZED * t;
	uchar *data;

	size = tgeom_serialize_size(tgeom);
	data = lwalloc(size);
	tgeom_serialize_buf(tgeom, data, &retsize);

	if ( retsize != size )
	{
		lwerror("tgeom_serialize_size returned %d, ..serialize_buf returned %d",
		        size, retsize);
	}

	t = lwalloc(sizeof(TSERIALIZED));
	t->flags = tgeom->flags;
	t->srid = tgeom->srid;
	t->data = data;

	/*
	     * We are aping PgSQL code here, PostGIS code should use
	     * VARSIZE to set this for real.
	     */
	t->size = retsize << 2;

	return t;
}


/*
 * Deserialize a TSERIALIZED struct and
 * return a TGEOM pointer
 */
TGEOM *
tgeom_deserialize(TSERIALIZED *serialized_form)
{
	uchar type, flags;
	TGEOM *result;
	uchar *loc, *data;
	int i, j;

	assert(serialized_form);
	assert(serialized_form->data);

	data = serialized_form->data;

	/* type and flags */
	type  = data[0];
	flags = data[1];
	result = tgeom_new(type, FLAGS_GET_Z(flags), FLAGS_GET_M(flags));
	loc = data + 2;

	/* srid */
	result->srid = lw_get_int32(loc);
	loc += 4;

	/* bbox */
	if (FLAGS_GET_BBOX(flags))
	{
		result->bbox = lwalloc(sizeof(BOX3D));
		memcpy(result->bbox, loc, sizeof(BOX3D));
		loc += sizeof(BOX3D);
	}
	else result->bbox = NULL;

	/* edges number (0=> EMPTY) */
	result->nedges = lw_get_int32(loc);
	loc  += 4;

	/* edges */
	result->edges = lwalloc(sizeof(TEDGE*) * (result->nedges + 1));
	for (i=1 ; i <= result->nedges ; i++)
	{
		result->edges[i] = lwalloc(sizeof(TEDGE));
		result->edges[i]->s = lwalloc(sizeof(POINT4D));
		result->edges[i]->e = lwalloc(sizeof(POINT4D));

		/* 3DM specific handle */
		if (!FLAGS_GET_Z(result->flags) && FLAGS_GET_M(result->flags))
		{
			memcpy(result->edges[i]->s, loc, sizeof(double) * 2);
			loc  += sizeof(double) * 2;
			memcpy(&(result->edges[i]->s->m), loc, sizeof(double));
			loc  += sizeof(double);

			memcpy(result->edges[i]->e, loc, sizeof(double) * 2);
			loc  += sizeof(double) * 2;
			memcpy(&(result->edges[i]->e->m), loc, sizeof(double));
			loc  += sizeof(double);
		}
		else /* 2D,3DZ && 4D */
		{
			memcpy(result->edges[i]->s, loc,
			       sizeof(double) * FLAGS_NDIMS(flags));
			loc  += sizeof(double) * FLAGS_NDIMS(flags);

			result->edges[i]->e = lwalloc(sizeof(POINT4D));
			memcpy(result->edges[i]->e, loc,
			       sizeof(double) * FLAGS_NDIMS(flags));
			loc  += sizeof(double) * FLAGS_NDIMS(flags);
		}

		result->edges[i]->count = lw_get_int32(loc);
		loc  += 4;
	}

	/* faces number */
	result->nfaces = lw_get_int32(loc);
	loc  += 4;

	/* faces */
	result->faces = lwalloc(sizeof(TFACE*) * result->nfaces);
	for (i=0 ; i < result->nfaces ; i++)
	{
		result->faces[i] = lwalloc(sizeof(TFACE));

		/* number of edges */
		result->faces[i]->nedges = lw_get_int32(loc);
		loc  += 4;

		/* edges array */
		result->faces[i]->edges = lwalloc(sizeof(TEDGE*)
		                                  * result->faces[i]->nedges);
		memcpy(result->faces[i]->edges, loc, sizeof(TEDGE*)
		       * result->faces[i]->nedges);
		loc  += 4 * result->faces[i]->nedges;

		/* number of rings */
		result->faces[i]->nrings = lw_get_int32(loc);
		loc  += 4;

		if (result->faces[i]->nrings)
			result->faces[i]->rings = lwalloc(sizeof(POINTARRAY*)
			                                  * result->faces[i]->nrings);

		for (j=0 ; j < result->faces[i]->nrings ; j++)
		{
			int npoints;

			/* number of points */
			npoints = lw_get_int32(loc);
			loc  += 4;

			/* pointarray */
			result->faces[i]->rings[j] = ptarray_construct_reference_data(FLAGS_GET_Z(flags), FLAGS_GET_M(flags), npoints, loc);
			
			loc += sizeof(double)* FLAGS_NDIMS(flags) * npoints;
		}
	}

	return result;
}


/*
 * Indicate if an given LWGEOM is or not a solid
 */
int
lwgeom_is_solid(LWGEOM *lwgeom)
{
	int solid=0;
	TGEOM *tgeom;

	assert(lwgeom);

	/* Obvious case who could'nt be solid */
	if (lwgeom->type != POLYHEDRALSURFACETYPE && lwgeom->type != TINTYPE) return 0;
	if (!FLAGS_GET_Z(lwgeom->flags)) return 0;

	/* Use TGEOM convert to know */
	tgeom = tgeom_from_lwgeom(lwgeom);
	solid = FLAGS_GET_SOLID(tgeom->flags);
	tgeom_free(tgeom);

	return solid;
}


/*
 *  Compute 2D perimeter of a TGEOM
 */
double
tgeom_perimeter2d(TGEOM* tgeom)
{
	int i;
	double hz, vt, bdy = 0.0;

	assert(tgeom);

	if (tgeom->type != POLYHEDRALSURFACETYPE && tgeom->type != TINTYPE)
		lwerror("tgeom_perimeter2d called with wrong type: %i - %s",
			tgeom->type, lwtype_name(tgeom->type)); 

	/* Solid have a 0.0 length perimeter */
	if (FLAGS_GET_SOLID(tgeom->flags)) return bdy;

	for (i=1 ; i <= tgeom->nedges ; i++)
	{
		if (tgeom->edges[i]->count == 1)
		{
       			hz = tgeom->edges[i]->s->x - tgeom->edges[i]->e->x;
       			vt = tgeom->edges[i]->s->y - tgeom->edges[i]->e->y;
       			bdy += sqrt(hz*hz + vt*vt);
		}
	}
	
	return bdy;
}


/*
 *  Compute 2D/3D perimeter of a TGEOM
 */
double
tgeom_perimeter(TGEOM* tgeom)
{
	int i;
	double hz, vt, ht, bdy = 0.0;

	assert(tgeom);

	if (tgeom->type != POLYHEDRALSURFACETYPE && tgeom->type != TINTYPE)
		lwerror("tgeom_perimeter called with wrong type: %i - %s",
			tgeom->type, lwtype_name(tgeom->type)); 

	/* Solid have a 0.0 length perimeter */
	if (FLAGS_GET_SOLID(tgeom->flags)) return bdy;

	/* If no Z use 2d function instead */
	if (!FLAGS_GET_Z(tgeom->flags))     return tgeom_perimeter2d(tgeom);

	for (i=1 ; i <= tgeom->nedges ; i++)
	{
		if (tgeom->edges[i]->count == 1)
		{
       			hz = tgeom->edges[i]->s->x - tgeom->edges[i]->e->x;
       			vt = tgeom->edges[i]->s->y - tgeom->edges[i]->e->y;
       			ht = tgeom->edges[i]->s->z - tgeom->edges[i]->e->z;
       			bdy += sqrt(hz*hz + vt*vt + ht*ht);
		}
	}
	
	return bdy;
}


/*
 * Print a TGEOM struct
 * Debug purpose only
 */
void
printTGEOM(TGEOM *tgeom)
{
	int i,j;

	assert(tgeom);

	printf("TGEOM:\n");
	printf(" - type %i, %s\n", tgeom->type, lwtype_name(tgeom->type));
	printf(" - srid %i\n", tgeom->srid);
	printf(" - nedges %i\n", tgeom->nedges);
	printf(" - nfaces %i\n", tgeom->nfaces);
	printf("  => EDGES:\n");

	for (i=1 ; i <= tgeom->nedges ; i++)
	{
		if      (FLAGS_NDIMS(tgeom->flags) == 2)
			printf("   [%i] (%lf,%lf) -> (%lf,%lf)\n", i,
			       tgeom->edges[i]->s->x,
			       tgeom->edges[i]->s->y,
			       tgeom->edges[i]->e->x,
			       tgeom->edges[i]->e->y);
		else if (FLAGS_NDIMS(tgeom->flags) == 3)
			printf("   [%i] (%lf,%lf,%lf) -> (%lf,%lf,%lf)\n", i,
			       tgeom->edges[i]->s->x,
			       tgeom->edges[i]->s->y,
			       tgeom->edges[i]->s->z,
			       tgeom->edges[i]->e->x,
			       tgeom->edges[i]->e->y,
			       tgeom->edges[i]->e->z);
		else
			printf("   [%i] (%lf,%lf,%lf,%lf) -> (%lf,%lf,%lf,%lf)\n", i,
			       tgeom->edges[i]->s->x,
			       tgeom->edges[i]->s->y,
			       tgeom->edges[i]->s->z,
			       tgeom->edges[i]->s->m,
			       tgeom->edges[i]->e->x,
			       tgeom->edges[i]->e->y,
			       tgeom->edges[i]->e->z,
			       tgeom->edges[i]->e->m);
	}

	for (i=0 ; i < tgeom->nfaces ; i++)
	{
		printf("  => FACE [%i] nedges:%i nrings:%i\n", i,
		       tgeom->faces[i]->nedges, tgeom->faces[i]->nrings);

		for (j=0 ; j < tgeom->faces[i]->nedges ; j++)
		{
			int edge = tgeom->faces[i]->edges[j];
			printf("    -> EDGES [%i]{%i} ", j, edge);

			if (FLAGS_NDIMS(tgeom->flags) == 2)
			{
				if (tgeom->faces[i]->edges[j] > 0)
					printf("(%lf,%lf) -> (%lf,%lf)\n",
					       tgeom->edges[edge]->s->x,
					       tgeom->edges[edge]->s->y,
					       tgeom->edges[edge]->e->x,
					       tgeom->edges[edge]->e->y);
				else
					printf("(%lf,%lf) -> (%lf,%lf)\n",
					       tgeom->edges[-edge]->e->x,
					       tgeom->edges[-edge]->e->y,
					       tgeom->edges[-edge]->s->x,
					       tgeom->edges[-edge]->s->y);
			}
			else if (FLAGS_NDIMS(tgeom->flags) == 3)
			{
				if (tgeom->faces[i]->edges[j] > 0)
					printf("(%lf,%lf,%lf -> %lf,%lf,%lf)\n",
					       tgeom->edges[edge]->s->x,
					       tgeom->edges[edge]->s->y,
					       tgeom->edges[edge]->s->z,
					       tgeom->edges[edge]->e->x,
					       tgeom->edges[edge]->e->y,
					       tgeom->edges[edge]->e->z);
				else
					printf("(%lf,%lf,%lf -> %lf,%lf,%lf)\n",
					       tgeom->edges[-edge]->e->x,
					       tgeom->edges[-edge]->e->y,
					       tgeom->edges[-edge]->e->z,
					       tgeom->edges[-edge]->s->x,
					       tgeom->edges[-edge]->s->y,
					       tgeom->edges[-edge]->s->z);
			}
			else if (FLAGS_NDIMS(tgeom->flags) == 4)
			{
				if (tgeom->faces[i]->edges[j] > 0)
					printf("(%lf,%lf,%lf,%lf -> %lf,%lf,%lf,%lf)\n",
					       tgeom->edges[edge]->s->x,
					       tgeom->edges[edge]->s->y,
					       tgeom->edges[edge]->s->z,
					       tgeom->edges[edge]->s->m,
					       tgeom->edges[edge]->e->x,
					       tgeom->edges[edge]->e->y,
					       tgeom->edges[edge]->e->z,
					       tgeom->edges[edge]->e->m);
				else
					printf("(%lf,%lf,%lf,%lf -> %lf,%lf,%lf,%lf)\n",
					       tgeom->edges[-edge]->e->x,
					       tgeom->edges[-edge]->e->y,
					       tgeom->edges[-edge]->e->z,
					       tgeom->edges[-edge]->e->m,
					       tgeom->edges[-edge]->s->x,
					       tgeom->edges[-edge]->s->y,
					       tgeom->edges[-edge]->s->z,
					       tgeom->edges[-edge]->s->m);
			}
		}

		for (j=0 ; j < tgeom->faces[i]->nrings ; j++)
		{
			printf("    - Ring[%i/%i]", j, tgeom->faces[i]->nrings);
			printPA(tgeom->faces[i]->rings[j]);
		}
	}
}
