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
#include "lwgeom_log.h"

struct LISTNODE
{
	struct LISTNODE* next;
	LWGEOM* geom;
};
typedef struct LISTNODE LISTNODE;

struct LWPOINTITERATOR
{
	LISTNODE* geoms;
	uint32_t ring;
	uint32_t i;
	char allow_modification;
};

typedef struct LWPOINTITERATOR LWPOINTITERATOR;

static LISTNODE*
prepend_node(LWGEOM* g, LISTNODE* front)
{
	LISTNODE* n = lwalloc(sizeof(LISTNODE));
	n->geom = g;
	n->next = front;

	return n;
}

static LISTNODE*
pop_node(LISTNODE* i)
{
	LISTNODE* next = i->next;
	lwfree(i);
	return next;
}

static int
add_lwgeom_to_stack(LWPOINTITERATOR* s, LWGEOM* g)
{
	if (lwgeom_is_empty(g))
		return LW_FAILURE;

	s->geoms = prepend_node(g, s->geoms);
	return LW_SUCCESS;
}

/* Attempts to retrieve the next point from active point geometry */
static int
next_from_point(LWPOINTITERATOR* s, POINT4D* p)
{
	LWPOINT* point = (LWPOINT*) s->geoms->geom;

	return getPoint4d_p(point->point, 0, p);
}

/* Attempts to retrieve the next point from active line geometry */
static int
next_from_line(LWPOINTITERATOR* s, POINT4D* p)
{
	LWLINE* line = (LWLINE*) s->geoms->geom;

	return getPoint4d_p(line->points, s->i, p);
}

/* Attempts to retrieve the next point from active polygon geometry */
static int
next_from_poly(LWPOINTITERATOR* s, POINT4D* p)
{
	LWPOLY* poly = (LWPOLY*) s->geoms->geom;

	return getPoint4d_p(poly->rings[s->ring], s->i, p);
}

/* Attempts to advance to the next vertex in active line geometry
 * Returns LW_TRUE if it exists, LW_FALSE otherwise
 * */
static int
advance_line(LWPOINTITERATOR* s)
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
static int
advance_poly(LWPOINTITERATOR* s)
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
static void
lwpointiterator_unroll_collection(LWPOINTITERATOR* s)
{
	int i;
	LWCOLLECTION* c = (LWCOLLECTION*) s->geoms->geom;
	s->geoms = pop_node(s->geoms);

	for (i = c->ngeoms - 1; i >= 0; i--)
	{
		LWGEOM* g = lwcollection_getsubgeom(c, i);

		add_lwgeom_to_stack(s, g);
	}
}

int
lwpointiterator_peek(LWPOINTITERATOR* s, POINT4D* p)
{
	if (s->geoms == NULL)
		return LW_FAILURE;
	if (s->geoms->geom == NULL)
		return LW_FAILURE;

	while(lwgeom_is_collection(s->geoms->geom))
	{
		lwpointiterator_unroll_collection(s);
	}

	switch(s->geoms->geom->type)
	{
	case POINTTYPE:
		return next_from_point(s, p);
	case LINETYPE:
	case TRIANGLETYPE:
	case CIRCSTRINGTYPE:
		return next_from_line(s, p);
	case POLYGONTYPE:
		return next_from_poly(s, p);
	default:
		lwerror("Unsupported geometry type for lwpointiterator_next");
		return LW_FAILURE;
	}

	return LW_SUCCESS;
}

int
lwpointiterator_has_next(LWPOINTITERATOR* s)
{
	POINT4D p;
	if (lwpointiterator_peek(s, &p))
		return LW_TRUE;
	return LW_FALSE;
}

int
lwpointiterator_next(LWPOINTITERATOR* s, POINT4D* p)
{
	char points_remain;

	if (!lwpointiterator_peek(s, p))
		return LW_FALSE;

	switch(s->geoms->geom->type)
	{
	case POINTTYPE:
		points_remain = LW_FALSE;
		break;
	case LINETYPE:
	case TRIANGLETYPE:
	case CIRCSTRINGTYPE:
		points_remain = advance_line(s);
		break;
	case POLYGONTYPE:
		points_remain = advance_poly(s);
		break;
	default:
		lwerror("Unsupported geometry type for lwpointiterator_next");
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

int
lwpointiterator_modify_next(LWPOINTITERATOR* s, const POINT4D* p)
{
	POINTARRAY* pa;
	LWGEOM* g;
	POINT4D throwaway;

	if (!s->allow_modification)
	{
		lwerror("Cannot write to read-only iterator");
		return LW_FAILURE;
	}

	if (!lwpointiterator_has_next(s))
	{
		return LW_FAILURE;
	}

	while(lwgeom_is_collection(s->geoms->geom))
	{
		lwpointiterator_unroll_collection(s);
	}

	g = s->geoms->geom;
	switch(lwgeom_get_type(g))
	{
	case POINTTYPE:
		pa = lwgeom_as_lwpoint(g)->point;
		break;
	case LINETYPE:
		pa = lwgeom_as_lwline(g)->points;
		break;
	case TRIANGLETYPE:
		pa = lwgeom_as_lwtriangle(g)->points;
		break;
	case CIRCSTRINGTYPE:
		pa = lwgeom_as_lwcircstring(g)->points;
		break;
	case POLYGONTYPE:
		pa = lwgeom_as_lwpoly(g)->rings[s->ring];
		break;
	default:
		lwerror("Unsupported geometry type for lwpointiterator_next");
		return LW_FAILURE;
	}

	ptarray_set_point4d(pa, s->i, p);

	return lwpointiterator_next(s, &throwaway);
}

LWPOINTITERATOR*
lwpointiterator_create(const LWGEOM* g)
{
	LWPOINTITERATOR* it = lwpointiterator_create_rw((LWGEOM*) g);
	it->allow_modification = LW_FALSE;

	return it;
}

LWPOINTITERATOR*
lwpointiterator_create_rw(LWGEOM* g)
{
	LWPOINTITERATOR* it = lwalloc(sizeof(LWPOINTITERATOR));

	it->geoms = NULL;
	add_lwgeom_to_stack(it, g);
	it->ring = 0;
	it->i = 0;
	it->allow_modification = LW_TRUE;

	return it;
}

/* Destroy the LWPOINTITERATOR */
void
lwpointiterator_destroy(LWPOINTITERATOR* s)
{
	while (s->geoms != NULL)
	{
		s->geoms = pop_node(s->geoms);
	}
	lwfree(s);
}
