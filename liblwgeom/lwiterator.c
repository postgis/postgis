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

#include "liblwgeom.h"
#include "lwiterator.h"
#include "lwgeom_log.h"

static LISTNODE* prepend_node(const LWGEOM* g, LISTNODE* front)
{
	LISTNODE* n = malloc(sizeof(LISTNODE));
	n->geom = g;
	n->next = front;

	return n;
}

static LISTNODE* pop_node(LISTNODE* i)
{
	LISTNODE* next = i->next;
	lwfree(i);
	return next;
}

static int add_lwgeom_to_stack(LWITERATOR*s, const LWGEOM* g)
{
	if (lwgeom_is_empty(g))
		return LW_FAILURE;

	s->geoms = prepend_node(g, s->geoms);
	return LW_SUCCESS;
}

/* Attempts to retrieve the next point from active point geometry */
static int next_from_point(LWITERATOR* s, POINT4D* p)
{
	LWPOINT* point = (LWPOINT*) s->geoms->geom;

	return getPoint4d_p(point->point, 0, p);
}

/* Attempts to retrieve the next point from active line geometry */
static int next_from_line(LWITERATOR* s, POINT4D* p)
{
	LWLINE* line = (LWLINE*) s->geoms->geom;

	return getPoint4d_p(line->points, s->i, p);
}

/* Attempts to retrieve the next point from active polygon geometry */
static int next_from_poly(LWITERATOR* s, POINT4D* p)
{
	LWPOLY* poly = (LWPOLY*) s->geoms->geom;

	return getPoint4d_p(poly->rings[s->ring], s->i, p);
}

/* Attempts to advance to the next vertex in active line geometry
 * Returns LW_TRUE if it exists, LW_FALSE otherwise
 * */
static int advance_line(LWITERATOR* s)
{
	LWLINE* line = (LWLINE*) s->geoms->geom;
	s->i += 1;

	if (s->i < line->points->npoints)
		return LW_TRUE;
	else
		return LW_FALSE;
}

/* Attempts to advance to the next vertex in active polygon geometry
 * Returns LW_TRUE if it exists, LW_FALSE otherwise
 * */
static int advance_poly(LWITERATOR* s)
{
	LWPOLY* poly = (LWPOLY*) s->geoms->geom;
	s->i += 1;

	/* Are there any more points left in this ring? */
	if (s->i < poly->rings[s->ring]->npoints)
		return LW_TRUE;

	/* We've finished with the current ring.  Reset our
	 * counter, and advance to the next ring. */
	s->i = 0;
	s->ring += 1;

	/* Does this ring exist? */
	if (s->ring < poly->nrings)
	{
		return LW_TRUE;
	}

	return LW_FALSE;
}

/* Removes a GeometryCollection from the iterator stack, and adds
 * the components of the GeometryCollection to the stack.
 * */
static void lwiterator_unroll_collection(LWITERATOR* s)
{
	uint32_t i;
	LWCOLLECTION* c = (LWCOLLECTION*) s->geoms->geom;
	s->geoms = pop_node(s->geoms);

	for (i = 0; i < c->ngeoms; i++)
	{
		LWGEOM* g = lwcollection_getsubgeom(c, i);

		add_lwgeom_to_stack(s, g);
	}
}


/* Attempts to assigns the next point in the iterator to p.  Does not advance.
 * Returns LW_TRUE if the assignment was successful, LW_FALSE otherwise.
 * */
int lwiterator_peek(LWITERATOR* s, POINT4D* p)
{
	if (s->geoms == NULL)
		return LW_FAILURE;
	if (s->geoms->geom == NULL)
		return LW_FAILURE;

	while(lwgeom_is_collection(s->geoms->geom))
	{
		lwiterator_unroll_collection(s);
	}

	switch(s->geoms->geom->type)
	{
	case POINTTYPE:
		return next_from_point(s, p);
	case LINETYPE:
	case TRIANGLETYPE:
		return next_from_line(s, p);
	case POLYGONTYPE:
		return next_from_poly(s, p);
	default:
		lwerror("Unsupported geometry type for lwiterator_next");
		return LW_FAILURE;
	}

	return LW_SUCCESS;
}

/* Returns true if there is another point available in the iterator. */
int lwiterator_has_next(LWITERATOR* s)
{
	POINT4D p;
	if (lwiterator_peek(s, &p))
		return LW_TRUE;
	return LW_FALSE;
}

/* Attempts to assign the next point int the iterator to p, and advances
 * the iterator to the next point. */
int lwiterator_next(LWITERATOR* s, POINT4D* p)
{
	char points_remain;

	if (!lwiterator_peek(s, p))
		return LW_FALSE;

	switch(s->geoms->geom->type)
	{
	case POINTTYPE:
		points_remain = LW_FALSE;
		break;
	case LINETYPE:
	case TRIANGLETYPE:
		points_remain = advance_line(s);
		break;
	case POLYGONTYPE:
		points_remain = advance_poly(s);
		break;
	default:
		lwerror("Unsupported geometry type for lwiterator_next");
		return LW_FAILURE;
	}

	if (!points_remain)
	{
		s->i = 0;
		s->ring = 0;
		s->geoms = pop_node(s->geoms);
	}

	return LW_SUCCESS;
}

/* Create a new LWITERATOR over supplied LWGEOM */
void lwiterator_create(const LWGEOM* g, LWITERATOR* s)
{
	s->geoms = NULL;
	add_lwgeom_to_stack(s, g);
	s->ring = 0;
	s->i = 0;
}

/* Destroy the LWITERATOR */
void lwiterator_destroy(LWITERATOR* s)
{
	while (s->geoms != NULL)
	{
		s->geoms = pop_node(s->geoms);
	}
}
