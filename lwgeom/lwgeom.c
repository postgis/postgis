#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "wktparse.h"

//#define PGIS_DEBUG_CALLS 1

LWGEOM *
lwgeom_deserialize(uchar *srl)
{
	int type = lwgeom_getType(srl[0]);

	//lwnotice("lwgeom_deserialize got %s", lwgeom_typename(type));

	switch (type)
	{
		case POINTTYPE:
			return (LWGEOM *)lwpoint_deserialize(srl);
		case LINETYPE:
			return (LWGEOM *)lwline_deserialize(srl);
		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_deserialize(srl);
		case MULTIPOINTTYPE:
			return (LWGEOM *)lwmpoint_deserialize(srl);
		case MULTILINETYPE:
			return (LWGEOM *)lwmline_deserialize(srl);
		case MULTIPOLYGONTYPE:
			return (LWGEOM *)lwmpoly_deserialize(srl);
		case COLLECTIONTYPE:
			return (LWGEOM *)lwcollection_deserialize(srl);
		default:
			lwerror("Unknown geometry type: %d", type);
			return NULL;
	}

}

size_t
lwgeom_serialize_size(LWGEOM *lwgeom)
{
	int type = TYPE_GETTYPE(lwgeom->type);

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwgeom_serialize_size(%s) called", lwgeom_typename(type));
#endif

	switch (type)
	{
		case POINTTYPE:
			return lwpoint_serialize_size((LWPOINT *)lwgeom);
		case LINETYPE:
			return lwline_serialize_size((LWLINE *)lwgeom);
		case POLYGONTYPE:
			return lwpoly_serialize_size((LWPOLY *)lwgeom);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_serialize_size((LWCOLLECTION *)lwgeom);
		default:
			lwerror("Unknown geometry type: %d", type);
			return 0;
	}
}

void
lwgeom_serialize_buf(LWGEOM *lwgeom, uchar *buf, size_t *retsize)
{
	int type = TYPE_GETTYPE(lwgeom->type);

#ifdef PGIS_DEBUG_CALLS 
	lwnotice("lwgeom_serialize_buf called with a %s",
			lwgeom_typename(type));
#endif
	switch (type)
	{
		case POINTTYPE:
			lwpoint_serialize_buf((LWPOINT *)lwgeom, buf, retsize);
			break;
		case LINETYPE:
			lwline_serialize_buf((LWLINE *)lwgeom, buf, retsize);
			break;
		case POLYGONTYPE:
			lwpoly_serialize_buf((LWPOLY *)lwgeom, buf, retsize);
			break;
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			lwcollection_serialize_buf((LWCOLLECTION *)lwgeom, buf,
				retsize);
			break;
		default:
			lwerror("Unknown geometry type: %d", type);
			return;
	}
	return;
}

uchar *
lwgeom_serialize(LWGEOM *lwgeom)
{
	size_t size = lwgeom_serialize_size(lwgeom);
	size_t retsize;
	uchar *serialized = lwalloc(size);

	lwgeom_serialize_buf(lwgeom, serialized, &retsize);

#ifdef PGIS_DEBUG
	if ( retsize != size )
	{
		lwerror("lwgeom_serialize: computed size %d, returned size %d",
			size, retsize);
	}
#endif

	return serialized;
}

// Force Right-hand-rule on LWGEOM polygons
void
lwgeom_forceRHR(LWGEOM *lwgeom)
{
	LWCOLLECTION *coll;
	int i;

	switch (TYPE_GETTYPE(lwgeom->type))
	{
		case POLYGONTYPE:
			lwpoly_reverse((LWPOLY *)lwgeom);
			return;

		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			coll = (LWCOLLECTION *)lwgeom;
			for (i=0; i<coll->ngeoms; i++)
				lwgeom_forceRHR(coll->geoms[i]);
			return;
	}
}

// Reverse vertex order of LWGEOM
void
lwgeom_reverse(LWGEOM *lwgeom)
{
	int i;
	LWCOLLECTION *col;

	switch (TYPE_GETTYPE(lwgeom->type))
	{
		case LINETYPE:
			lwline_reverse((LWLINE *)lwgeom);
			return;
		case POLYGONTYPE:
			lwpoly_reverse((LWPOLY *)lwgeom);
			return;
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			col = (LWCOLLECTION *)lwgeom;
			for (i=0; i<col->ngeoms; i++)
				lwgeom_reverse(col->geoms[i]);
			return;
	}
}

int
lwgeom_compute_box2d_p(LWGEOM *lwgeom, BOX2DFLOAT4 *buf)
{
	switch(TYPE_GETTYPE(lwgeom->type))
	{
		case POINTTYPE:
			return lwpoint_compute_box2d_p((LWPOINT *)lwgeom, buf);
		case LINETYPE:
			return lwline_compute_box2d_p((LWLINE *)lwgeom, buf);
		case POLYGONTYPE:
			return lwpoly_compute_box2d_p((LWPOLY *)lwgeom, buf);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_compute_box2d_p((LWCOLLECTION *)lwgeom, buf);
	}
	return 0;
}

//dont forget to lwfree() result

BOX2DFLOAT4 *
lwgeom_compute_box2d(LWGEOM *lwgeom)
{
	BOX2DFLOAT4 *result = lwalloc(sizeof(BOX2DFLOAT4));
	if ( lwgeom_compute_box2d_p(lwgeom, result) ) return result;
	else  {
		lwfree(result);
		return NULL;
	}
}

LWPOINT *
lwgeom_as_lwpoint(LWGEOM *lwgeom)
{
	if ( lwgeom->type == POINTTYPE ) return (LWPOINT *)lwgeom;
	else return NULL;
}

LWLINE *
lwgeom_as_lwline(LWGEOM *lwgeom)
{
	if ( lwgeom->type == LINETYPE ) return (LWLINE *)lwgeom;
	else return NULL;
}

LWPOLY *
lwgeom_as_lwpoly(LWGEOM *lwgeom)
{
	if ( lwgeom->type == POLYGONTYPE ) return (LWPOLY *)lwgeom;
	else return NULL;
}

LWCOLLECTION *
lwgeom_as_lwcollection(LWGEOM *lwgeom)
{
	if ( TYPE_GETTYPE(lwgeom->type) >= MULTIPOINTTYPE )
		return (LWCOLLECTION *)lwgeom;
	else return NULL;
}

LWMPOINT *
lwgeom_as_lwmpoint(LWGEOM *lwgeom)
{
	if ( lwgeom->type == MULTIPOINTTYPE ) return (LWMPOINT *)lwgeom;
	else return NULL;
}

LWMLINE *
lwgeom_as_lwmline(LWGEOM *lwgeom)
{
	if ( lwgeom->type == MULTILINETYPE ) return (LWMLINE *)lwgeom;
	else return NULL;
}

LWMPOLY *
lwgeom_as_lwmpoly(LWGEOM *lwgeom)
{
	if ( lwgeom->type == MULTIPOLYGONTYPE ) return (LWMPOLY *)lwgeom;
	else return NULL;
}

LWGEOM *lwmpoly_as_lwgeom(LWMPOLY *obj) { return (LWGEOM *)obj; }
LWGEOM *lwmline_as_lwgeom(LWMLINE *obj) { return (LWGEOM *)obj; }
LWGEOM *lwmpoint_as_lwgeom(LWMPOINT *obj) { return (LWGEOM *)obj; }
LWGEOM *lwcollection_as_lwgeom(LWCOLLECTION *obj) { return (LWGEOM *)obj; }
LWGEOM *lwpoly_as_lwgeom(LWPOLY *obj) { return (LWGEOM *)obj; }
LWGEOM *lwline_as_lwgeom(LWLINE *obj) { return (LWGEOM *)obj; }
LWGEOM *lwpoint_as_lwgeom(LWPOINT *obj) { return (LWGEOM *)obj; }

void
lwgeom_release(LWGEOM *lwgeom)
{
	uint32 i;
	LWCOLLECTION *col;

#ifdef INTEGRITY_CHECKS
	if ( ! lwgeom )
		lwerror("lwgeom_release: someone called on 0x0");
#endif

	// Drop bounding box (always a copy)
	if ( lwgeom->bbox ) lwfree(lwgeom->bbox);

	// Collection
	if ( (col=lwgeom_as_lwcollection(lwgeom)) )
	{
		for (i=0; i<col->ngeoms; i++)
		{
			lwgeom_release(col->geoms[i]);
		}
		lwfree(lwgeom);
	}

	// Single element
	else lwfree(lwgeom);

}

// Clone an LWGEOM object. POINTARRAY are not copied.
LWGEOM *
lwgeom_clone(const LWGEOM *lwgeom)
{
	switch(TYPE_GETTYPE(lwgeom->type))
	{
		case POINTTYPE:
			return (LWGEOM *)lwpoint_clone((LWPOINT *)lwgeom);
		case LINETYPE:
			return (LWGEOM *)lwline_clone((LWLINE *)lwgeom);
		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_clone((LWPOLY *)lwgeom);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return (LWGEOM *)lwcollection_clone((LWCOLLECTION *)lwgeom);
		default:
			return NULL;
	}
}

// Add 'what' to 'to' at position 'where'.
// where=0 == prepend
// where=-1 == append
// Appended-to LWGEOM gets a new type based on new condition.
// Mix of dimensions is not allowed (TODO: allow it?).
LWGEOM *
lwgeom_add(const LWGEOM *to, uint32 where, const LWGEOM *what)
{
	if ( TYPE_NDIMS(what->type) != TYPE_NDIMS(to->type) )
	{
		lwerror("lwgeom_add: mixed dimensions not supported");
		return NULL;
	}

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwgeom_add(%s, %d, %s) called",
		lwgeom_typename(TYPE_GETTYPE(to->type)),
		where,
		lwgeom_typename(TYPE_GETTYPE(what->type)));
#endif

	switch(TYPE_GETTYPE(to->type))
	{
		case POINTTYPE:
			return (LWGEOM *)lwpoint_add((const LWPOINT *)to, where, what);
		case LINETYPE:
			return (LWGEOM *)lwline_add((const LWLINE *)to, where, what);
		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_add((const LWPOLY *)to, where, what);

		case MULTIPOINTTYPE:
			return (LWGEOM *)lwmpoint_add((const LWMPOINT *)to,
				where, what);

		case MULTILINETYPE:
			return (LWGEOM *)lwmline_add((const LWMLINE *)to,
				where, what);

		case MULTIPOLYGONTYPE:
			return (LWGEOM *)lwmpoly_add((const LWMPOLY *)to,
				where, what);

		case COLLECTIONTYPE:
			return (LWGEOM *)lwcollection_add(
				(const LWCOLLECTION *)to, where, what);

		default:
			lwerror("lwgeom_add: unknown geometry type: %d",
				TYPE_GETTYPE(to->type));
			return NULL;
	}
}

/*
 * Return an alloced string
 */
char *
lwgeom_to_ewkt(LWGEOM *lwgeom)
{
	uchar *serialized = lwgeom_serialize(lwgeom);
	char *ret;
	if ( ! serialized ) {
		lwerror("Error serializing geom %p", lwgeom);
	}
	ret = unparse_WKT(serialized, lwalloc, lwfree);
	lwfree(serialized);
	return ret;
}

/*
 * Return an alloced string
 */
char *
lwgeom_to_hexwkb(LWGEOM *lwgeom, unsigned int byteorder)
{
	uchar *serialized = lwgeom_serialize(lwgeom);
	char *hexwkb = unparse_WKB(serialized, lwalloc, lwfree, byteorder,NULL,1);
	lwfree(serialized);
	return hexwkb;
}

/*
 * Return an alloced string
 */
uchar *
lwgeom_to_ewkb(LWGEOM *lwgeom, char byteorder, size_t *outsize)
{
	uchar *serialized = lwgeom_serialize(lwgeom);
	char *hexwkb = unparse_WKB(serialized, lwalloc, lwfree,
		byteorder, outsize, 0);
	lwfree(serialized);
	return hexwkb;
}

LWGEOM *
lwgeom_from_ewkb(uchar *ewkb, size_t size)
{
	uchar *pglwgeom = (uchar *)pglwgeom_from_ewkb(ewkb, size);
	LWGEOM *ret = lwgeom_deserialize(pglwgeom+4);
	return ret;
}

// geom1 same as geom2
//  iff
//      + have same type                                                        //      + have same # objects
//      + have same bvol
//      + each object in geom1 has a corresponding object in geom2 (see above)
//
char
lwgeom_same(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2)
{
	if ( TYPE_GETTYPE(lwgeom1->type) != TYPE_GETTYPE(lwgeom2->type) )
		return 0;

	if ( TYPE_GETZM(lwgeom1->type) != TYPE_GETZM(lwgeom2->type) )
		return 0;

	// Check boxes if both already computed 
	if ( lwgeom1->bbox && lwgeom2->bbox )
	{
		//lwnotice("bbox1:%p, bbox2:%p", lwgeom1->bbox, lwgeom2->bbox);
		if ( ! box2d_same(lwgeom1->bbox, lwgeom2->bbox) ) return 0;
	}

	// geoms have same type, invoke type-specific function
	switch(TYPE_GETTYPE(lwgeom1->type))
	{
		case POINTTYPE:
			return lwpoint_same((LWPOINT *)lwgeom1,
				(LWPOINT *)lwgeom2);
		case LINETYPE:
			return lwline_same((LWLINE *)lwgeom1,
				(LWLINE *)lwgeom2);
		case POLYGONTYPE:
			return lwpoly_same((LWPOLY *)lwgeom1,
				(LWPOLY *)lwgeom2);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_same((LWCOLLECTION *)lwgeom1,
				(LWCOLLECTION *)lwgeom2);
		default:
			lwerror("lwgeom_same: unknown geometry type: %d",
				TYPE_GETTYPE(lwgeom1->type));
			return 0;
	}

}

void
lwgeom_changed(LWGEOM *lwgeom)
{
	if ( lwgeom->bbox ) lwfree(lwgeom->bbox);
	lwgeom->bbox = NULL;
	TYPE_SETHASBBOX(lwgeom->type, 0);
}

void
lwgeom_dropBBOX(LWGEOM *lwgeom)
{
	if ( lwgeom->bbox ) lwfree(lwgeom->bbox);
	lwgeom->bbox = NULL;
	TYPE_SETHASBBOX(lwgeom->type, 0);
}

/*
 * Ensure there's a box in the LWGEOM.
 * If the box is already there just return,
 * else compute it.
 */
void
lwgeom_addBBOX(LWGEOM *lwgeom)
{
	if ( lwgeom->bbox ) return;
	lwgeom->bbox = lwgeom_compute_box2d(lwgeom);
	TYPE_SETHASBBOX(lwgeom->type, 1);
}

void
lwgeom_dropSRID(LWGEOM *lwgeom)
{
	TYPE_SETHASSRID(lwgeom->type, 0);
	lwgeom->SRID = -1;
}

LWGEOM *
lwgeom_segmentize2d(LWGEOM *lwgeom, double dist)
{
	switch(TYPE_GETTYPE(lwgeom->type))
	{
		case LINETYPE:
			return (LWGEOM *)lwline_segmentize2d((LWLINE *)lwgeom,
				dist);
		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_segmentize2d((LWPOLY *)lwgeom,
				dist);
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return (LWGEOM *)lwcollection_segmentize2d(
				(LWCOLLECTION *)lwgeom, dist);

		default:
			return lwgeom_clone(lwgeom);
	}
}

void
lwgeom_longitude_shift(LWGEOM *lwgeom)
{
	int i;
	switch(TYPE_GETTYPE(lwgeom->type))
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWCOLLECTION *coll;

		case POINTTYPE:
			point = (LWPOINT *)lwgeom;
			ptarray_longitude_shift(point->point);
			return;
		case LINETYPE:
			line = (LWLINE *)lwgeom;
			ptarray_longitude_shift(line->points);
			return;
		case POLYGONTYPE:
			poly = (LWPOLY *)lwgeom;
			for (i=0; i<poly->nrings; i++)
				ptarray_longitude_shift(poly->rings[i]);
			return;
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			coll = (LWCOLLECTION *)lwgeom;
			for (i=0; i<coll->ngeoms; i++)
				lwgeom_longitude_shift(coll->geoms[i]);
			return;
		default:
			lwerror("%s:%d: unknown geom type: %d",
				__FILE__, __LINE__,
				TYPE_GETTYPE(lwgeom->type));
	}
}
