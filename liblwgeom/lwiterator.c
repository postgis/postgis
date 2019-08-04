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


#include "liblwgeom.h"
#include "lwgeom_log.h"

struct LISTNODE
{
	struct LISTNODE* next;
	void* item;
};
typedef struct LISTNODE LISTNODE;

/* The LWPOINTITERATOR consists of two stacks of items to process: a stack
 * of geometries, and a stack of POINTARRAYs extracted from those geometries.
 * The index "i" refers to the "next" point, which is found at the top of the
 * pointarrays stack.
 *
 * When the pointarrays stack is depleted, we pull a geometry from the geometry
 * stack to replenish it.
 */
struct LWPOINTITERATOR
{
	LISTNODE* geoms;
	LISTNODE* pointarrays;
	uint32_t i;
	char allow_modification;
};

static LISTNODE*
prepend_node(void* g, LISTNODE* front)
{
	LISTNODE* n = lwalloc(sizeof(LISTNODE));
	n->item = g;
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

/** Return a pointer to the first of one or more LISTNODEs holding the POINTARRAYs
 *  of a geometry.  Will not handle GeometryCollections.
 */
static LISTNODE*
extract_pointarrays_from_lwgeom(LWGEOM* g)
{
	switch(lwgeom_get_type(g))
	{
	case POINTTYPE:
		return prepend_node(lwgeom_as_lwpoint(g)->point, NULL);
	case LINETYPE:
		return prepend_node(lwgeom_as_lwline(g)->points, NULL);
	case TRIANGLETYPE:
		return prepend_node(lwgeom_as_lwtriangle(g)->points, NULL);
	case CIRCSTRINGTYPE:
		return prepend_node(lwgeom_as_lwcircstring(g)->points, NULL);
	case POLYGONTYPE:
	{
		LISTNODE* n = NULL;

		LWPOLY* p = lwgeom_as_lwpoly(g);
		int i;
		for (i = p->nrings - 1; i >= 0; i--)
			n = prepend_node(p->rings[i], n);

		return n;
	}
	default:
		lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(g->type));
	}

	return NULL;
}

/** Remove an LWCOLLECTION from the iterator stack, and add the components of the
 *  LWCOLLECTIONs to the stack.
 */
static void
unroll_collection(LWPOINTITERATOR* s)
{
	int i;
	LWCOLLECTION* c;

	if (!s->geoms)
	{
		return;
	}

	c = (LWCOLLECTION*) s->geoms->item;
	s->geoms = pop_node(s->geoms);

	for (i = c->ngeoms - 1; i >= 0; i--)
	{
		LWGEOM* g = lwcollection_getsubgeom(c, i);

		add_lwgeom_to_stack(s, g);
	}
}

/** Unroll LWCOLLECTIONs from the top of the stack, as necessary, until the element at the
 *  top of the stack is not a LWCOLLECTION.
 */
static void
unroll_collections(LWPOINTITERATOR* s)
{
	while(s->geoms && lwgeom_is_collection(s->geoms->item))
	{
		unroll_collection(s);
	}
}

static int
lwpointiterator_advance(LWPOINTITERATOR* s)
{
	s->i += 1;

	/* We've reached the end of our current POINTARRAY.  Try to see if there
	 * are any more POINTARRAYS on the stack. */
	if (s->pointarrays && s->i >= ((POINTARRAY*) s->pointarrays->item)->npoints)
	{
		s->pointarrays = pop_node(s->pointarrays);
		s->i = 0;
	}

	/* We don't have a current POINTARRAY.  Pull a geometry from the stack, and
	 * decompose it into its POINTARRARYs. */
	if (!s->pointarrays)
	{
		LWGEOM* g;
		unroll_collections(s);

		if (!s->geoms)
		{
			return LW_FAILURE;
		}

		s->i = 0;
		g = s->geoms->item;
		s->pointarrays = extract_pointarrays_from_lwgeom(g);

		s->geoms = pop_node(s->geoms);
	}

	if (!s->pointarrays)
	{
		return LW_FAILURE;
	}
	return LW_SUCCESS;
}

/* Public API implementation */

int
lwpointiterator_peek(LWPOINTITERATOR* s, POINT4D* p)
{
	if (!lwpointiterator_has_next(s))
		return LW_FAILURE;

	return getPoint4d_p(s->pointarrays->item, s->i, p);
}

int
lwpointiterator_has_next(LWPOINTITERATOR* s)
{
	if (s->pointarrays && s->i < ((POINTARRAY*) s->pointarrays->item)->npoints)
		return LW_TRUE;
	return LW_FALSE;
}

int
lwpointiterator_next(LWPOINTITERATOR* s, POINT4D* p)
{
	if (!lwpointiterator_has_next(s))
		return LW_FAILURE;

	/* If p is NULL, just advance without reading */
	if (p && !lwpointiterator_peek(s, p))
		return LW_FAILURE;

	lwpointiterator_advance(s);
	return LW_SUCCESS;
}

int
lwpointiterator_modify_next(LWPOINTITERATOR* s, const POINT4D* p)
{
	if (!lwpointiterator_has_next(s))
		return LW_FAILURE;

	if (!s->allow_modification)
	{
		lwerror("Cannot write to read-only iterator");
		return LW_FAILURE;
	}

	ptarray_set_point4d(s->pointarrays->item, s->i, p);

	lwpointiterator_advance(s);
	return LW_SUCCESS;
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
	it->pointarrays = NULL;
	it->i = 0;
	it->allow_modification = LW_TRUE;

	add_lwgeom_to_stack(it, g);
	lwpointiterator_advance(it);

	return it;
}

void
lwpointiterator_destroy(LWPOINTITERATOR* s)
{
	while (s->geoms != NULL)
	{
		s->geoms = pop_node(s->geoms);
	}

	while (s->pointarrays != NULL)
	{
		s->pointarrays = pop_node(s->pointarrays);
	}

	lwfree(s);
}
