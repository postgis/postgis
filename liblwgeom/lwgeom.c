/**********************************************************************
 * $Id: lwgeom.c 2797 2008-05-31 09:56:44Z mcayland $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/*#include "lwgeom_pg.h"*/
#include "liblwgeom.h"
#include "wktparse.h"


LWGEOM *
lwgeom_deserialize(uchar *srl)
{
	int type = lwgeom_getType(srl[0]);

	LWDEBUGF(2, "lwgeom_deserialize got %d - %s", type, lwgeom_typename(type));

	switch (type)
	{
		case POINTTYPE:
			return (LWGEOM *)lwpoint_deserialize(srl);
		case LINETYPE:
			return (LWGEOM *)lwline_deserialize(srl);
                case CURVETYPE:
                        return (LWGEOM *)lwcurve_deserialize(srl);
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
                case COMPOUNDTYPE:
                        return (LWGEOM *)lwcompound_deserialize(srl);
                case CURVEPOLYTYPE:
                        return (LWGEOM *)lwcurvepoly_deserialize(srl);
                case MULTICURVETYPE:
                        return (LWGEOM *)lwmcurve_deserialize(srl);
                case MULTISURFACETYPE:
                        return (LWGEOM *)lwmsurface_deserialize(srl);
		default:
			lwerror("Unknown geometry type: %d", type);

			return NULL;
	}

}

size_t
lwgeom_serialize_size(LWGEOM *lwgeom)
{
	int type = TYPE_GETTYPE(lwgeom->type);

	LWDEBUGF(2, "lwgeom_serialize_size(%s) called", lwgeom_typename(type));

	switch (type)
	{
		case POINTTYPE:
			return lwpoint_serialize_size((LWPOINT *)lwgeom);
		case LINETYPE:
			return lwline_serialize_size((LWLINE *)lwgeom);
		case POLYGONTYPE:
			return lwpoly_serialize_size((LWPOLY *)lwgeom);
                case CURVETYPE:
                        return lwcurve_serialize_size((LWCURVE *)lwgeom);
                case CURVEPOLYTYPE:
                case COMPOUNDTYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
                case MULTICURVETYPE:
		case MULTIPOLYGONTYPE:
                case MULTISURFACETYPE:
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

	LWDEBUGF(2, "lwgeom_serialize_buf called with a %s",
			lwgeom_typename(type));

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
                case CURVETYPE:
                        lwcurve_serialize_buf((LWCURVE *)lwgeom, buf, retsize);
                        break;
                case CURVEPOLYTYPE:
                case COMPOUNDTYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
                case MULTICURVETYPE:
		case MULTIPOLYGONTYPE:
                case MULTISURFACETYPE:
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

#if POSTGIS_DEBUG_LEVEL > 0
	if ( retsize != size )
	{
		lwerror("lwgeom_serialize: computed size %d, returned size %d",
			size, retsize);
	}
#endif

	return serialized;
}

/* Force Right-hand-rule on LWGEOM polygons */
void
lwgeom_forceRHR(LWGEOM *lwgeom)
{
	LWCOLLECTION *coll;
	int i;

	switch (TYPE_GETTYPE(lwgeom->type))
	{
		case POLYGONTYPE:
			lwpoly_forceRHR((LWPOLY *)lwgeom);
			return;

		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			coll = (LWCOLLECTION *)lwgeom;
			for (i=0; i<coll->ngeoms; i++)
				lwgeom_forceRHR(coll->geoms[i]);
			return;
	}
}

/* Reverse vertex order of LWGEOM */
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
        LWDEBUGF(2, "lwgeom_compute_box2d_p called of %p of type %d.", lwgeom, TYPE_GETTYPE(lwgeom->type));

	switch(TYPE_GETTYPE(lwgeom->type))
	{
		case POINTTYPE:
			return lwpoint_compute_box2d_p((LWPOINT *)lwgeom, buf);
		case LINETYPE:
			return lwline_compute_box2d_p((LWLINE *)lwgeom, buf);
                case CURVETYPE:
                        return lwcurve_compute_box2d_p((LWCURVE *)lwgeom, buf);
		case POLYGONTYPE:
			return lwpoly_compute_box2d_p((LWPOLY *)lwgeom, buf);
                case COMPOUNDTYPE:
                case CURVEPOLYTYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
                case MULTICURVETYPE:
		case MULTIPOLYGONTYPE:
                case MULTISURFACETYPE:
		case COLLECTIONTYPE:
			return lwcollection_compute_box2d_p((LWCOLLECTION *)lwgeom, buf);
	}
	return 0;
}

/*
 * dont forget to lwfree() result
 */
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
	if ( TYPE_GETTYPE(lwgeom->type) == POINTTYPE )
		return (LWPOINT *)lwgeom;
	else return NULL;
}

LWLINE *
lwgeom_as_lwline(LWGEOM *lwgeom)
{
	if ( TYPE_GETTYPE(lwgeom->type) == LINETYPE )
		return (LWLINE *)lwgeom;
	else return NULL;
}

LWCURVE *
lwgeom_as_lwcurve(LWGEOM *lwgeom)
{
        if( TYPE_GETTYPE(lwgeom->type) == CURVETYPE )
                return (LWCURVE *)lwgeom;
        else return NULL;
}

LWPOLY *
lwgeom_as_lwpoly(LWGEOM *lwgeom)
{
	if ( TYPE_GETTYPE(lwgeom->type) == POLYGONTYPE )
		return (LWPOLY *)lwgeom;
	else return NULL;
}

LWCOLLECTION *
lwgeom_as_lwcollection(LWGEOM *lwgeom)
{
	if ( TYPE_GETTYPE(lwgeom->type) >= MULTIPOINTTYPE 
            && TYPE_GETTYPE(lwgeom->type) <= COLLECTIONTYPE)
		return (LWCOLLECTION *)lwgeom;
	else return NULL;
}

LWMPOINT *
lwgeom_as_lwmpoint(LWGEOM *lwgeom)
{
	if ( TYPE_GETTYPE(lwgeom->type) == MULTIPOINTTYPE )
		return (LWMPOINT *)lwgeom;
	else return NULL;
}

LWMLINE *
lwgeom_as_lwmline(LWGEOM *lwgeom)
{
	if ( TYPE_GETTYPE(lwgeom->type) == MULTILINETYPE )
		return (LWMLINE *)lwgeom;
	else return NULL;
}

LWMPOLY *
lwgeom_as_lwmpoly(LWGEOM *lwgeom)
{
	if ( TYPE_GETTYPE(lwgeom->type) == MULTIPOLYGONTYPE )
		return (LWMPOLY *)lwgeom;
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

	/* Drop bounding box (always a copy) */
	if ( lwgeom->bbox ) {
                LWDEBUG(3, "lwgeom_release: releasing bbox.");

                lwfree(lwgeom->bbox);
        }

	/* Collection */
	if ( (col=lwgeom_as_lwcollection(lwgeom)) )
	{
                LWDEBUG(3, "lwgeom_release: Releasing collection.");

		for (i=0; i<col->ngeoms; i++)
		{
			lwgeom_release(col->geoms[i]);
		}
		lwfree(lwgeom);
	}

	/* Single element */
	else lwfree(lwgeom);

}

/* Clone an LWGEOM object. POINTARRAY are not copied. */
LWGEOM *
lwgeom_clone(const LWGEOM *lwgeom)
{
        LWDEBUGF(2, "lwgeom_clone called with %p, %d", lwgeom, TYPE_GETTYPE(lwgeom->type));

	switch(TYPE_GETTYPE(lwgeom->type))
	{
		case POINTTYPE:
			return (LWGEOM *)lwpoint_clone((LWPOINT *)lwgeom);
		case LINETYPE:
			return (LWGEOM *)lwline_clone((LWLINE *)lwgeom);
                case CURVETYPE:
                        return (LWGEOM *)lwcurve_clone((LWCURVE *)lwgeom);
		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_clone((LWPOLY *)lwgeom);
                case COMPOUNDTYPE:
                case CURVEPOLYTYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
                case MULTICURVETYPE:
		case MULTIPOLYGONTYPE:
                case MULTISURFACETYPE:
		case COLLECTIONTYPE:
			return (LWGEOM *)lwcollection_clone((LWCOLLECTION *)lwgeom);
		default:
			return NULL;
	}
}

/*
 * Add 'what' to 'to' at position 'where'
 *
 * where=0 == prepend
 * where=-1 == append
 * Appended-to LWGEOM gets a new type based on new condition.
 * Mix of dimensions is not allowed (TODO: allow it?).
 */
LWGEOM *
lwgeom_add(const LWGEOM *to, uint32 where, const LWGEOM *what)
{
	if ( TYPE_NDIMS(what->type) != TYPE_NDIMS(to->type) )
	{
		lwerror("lwgeom_add: mixed dimensions not supported");
		return NULL;
	}

	LWDEBUGF(2, "lwgeom_add(%s, %d, %s) called",
		lwgeom_typename(TYPE_GETTYPE(to->type)),
		where,
		lwgeom_typename(TYPE_GETTYPE(what->type)));

	switch(TYPE_GETTYPE(to->type))
	{
		case POINTTYPE:
			return (LWGEOM *)lwpoint_add((const LWPOINT *)to, where, what);
		case LINETYPE:
			return (LWGEOM *)lwline_add((const LWLINE *)to, where, what);

                case CURVETYPE:
                        return (LWGEOM *)lwcurve_add((const LWCURVE *)to, where, what);

		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_add((const LWPOLY *)to, where, what);

                case COMPOUNDTYPE:
                        return (LWGEOM *)lwcompound_add((const LWCOMPOUND *)to, where, what);

                case CURVEPOLYTYPE:
                        return (LWGEOM *)lwcurvepoly_add((const LWCURVEPOLY *)to, where, what);

		case MULTIPOINTTYPE:
			return (LWGEOM *)lwmpoint_add((const LWMPOINT *)to,
				where, what);

		case MULTILINETYPE:
			return (LWGEOM *)lwmline_add((const LWMLINE *)to,
				where, what);

                case MULTICURVETYPE:
                        return (LWGEOM *)lwmcurve_add((const LWMCURVE *)to,
                                where, what);

		case MULTIPOLYGONTYPE:
			return (LWGEOM *)lwmpoly_add((const LWMPOLY *)to,
				where, what);

                case MULTISURFACETYPE:
                        return (LWGEOM *)lwmsurface_add((const LWMSURFACE *)to,
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
lwgeom_to_ewkt(LWGEOM *lwgeom, int flags)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	uchar *serialized = lwgeom_serialize(lwgeom);
	int result;

	if ( ! serialized ) {
		lwerror("Error serializing geom %p", lwgeom);
	}

	result = unparse_WKT(&lwg_unparser_result, serialized, lwalloc, lwfree, flags);
	lwfree(serialized);

	return lwg_unparser_result.wkoutput;
}

/*
 * Return an alloced string
 */
char *
lwgeom_to_hexwkb(LWGEOM *lwgeom, int flags, unsigned int byteorder)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	uchar *serialized = lwgeom_serialize(lwgeom);
	int result;

	result = unparse_WKB(&lwg_unparser_result, serialized, lwalloc, lwfree, flags, byteorder,1);

	lwfree(serialized);
	return lwg_unparser_result.wkoutput;
}

/*
 * Return an alloced string
 */
uchar *
lwgeom_to_ewkb(LWGEOM *lwgeom, int flags, char byteorder, size_t *outsize)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	uchar *serialized = lwgeom_serialize(lwgeom);
	int result;

	/*
	 * We cast return to "unsigned" char as we are
	 * requesting a "binary" output, not HEX
	 * (last argument set to 0)
	 */
	result = unparse_WKB(&lwg_unparser_result, serialized, lwalloc, lwfree,
		flags, byteorder, 0);
	lwfree(serialized);
	return (uchar *)lwg_unparser_result.wkoutput;
}

/*
 * Make an LWGEOM object from a EWKB binary representation.
 * Currently highly unoptimized as it:
 * 	- convert EWKB to HEXEWKB 
 *	- construct PG_LWGEOM
 *	- deserialize it
 */
LWGEOM *
lwgeom_from_ewkb(uchar *ewkb, int flags, size_t size)
{
	size_t hexewkblen = size*2;
	char *hexewkb;
	long int i;
	int result;
	LWGEOM *ret;
	LWGEOM_PARSER_RESULT lwg_parser_result;

	/* "HEXify" the EWKB */
	hexewkb = lwalloc(hexewkblen+1);
	for (i=0; i<size; ++i) deparse_hex(ewkb[i], &hexewkb[i*2]);
	hexewkb[hexewkblen] = '\0';

	/* Rely on grammar parser to construct a LWGEOM */
	result = serialized_lwgeom_from_ewkt(&lwg_parser_result, hexewkb, flags);

	/* Free intermediate HEXified representation */
	lwfree(hexewkb);

	/* Deserialize */
	ret = lwgeom_deserialize(lwg_parser_result.serialized_lwgeom);

	return ret;
}


/*
 * Parser functions for working with serialized LWGEOMs. Useful for cases where
 * the function input is already serialized, e.g. some input and output functions
 */

/*
 * Make a serialzed LWGEOM object from a WKT input string
 */
int
serialized_lwgeom_from_ewkt(LWGEOM_PARSER_RESULT *lwg_parser_result, char *wkt_input, int flags)
{

	int result = parse_lwg(lwg_parser_result, wkt_input, flags,
		lwalloc, lwerror);

	LWDEBUGF(2, "serialized_lwgeom_from_ewkt with %s",wkt_input);

	return result;
}

/*
 * Return an alloced string
 */
int
serialized_lwgeom_to_ewkt(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags)
{
	int result;

	result = unparse_WKT(lwg_unparser_result, serialized, lwalloc, lwfree, flags);

	return result;
}

/*
 * Return an alloced string
 */
int
serialized_lwgeom_to_hexwkb(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags, unsigned int byteorder)
{
	int result;

	result = unparse_WKB(lwg_unparser_result, serialized, lwalloc, lwfree, flags, byteorder, 1);

	return result;
}

/*
 * Return an alloced string
 */
int
serialized_lwgeom_to_ewkb(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags, unsigned int byteorder)
{
	int result;

	result = unparse_WKB(lwg_unparser_result, serialized, lwalloc, lwfree, flags, byteorder, 0);

	return result;
}


/*
 * geom1 same as geom2
 *  iff
 *      + have same type 
 *	+ have same # objects
 *      + have same bvol
 *      + each object in geom1 has a corresponding object in geom2 (see above)
 */
char
lwgeom_same(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2)
{
	LWDEBUGF(2, "lwgeom_same(%s, %s) called",
		lwgeom_typename(TYPE_GETTYPE(lwgeom1->type)),
		lwgeom_typename(TYPE_GETTYPE(lwgeom2->type)));

	if ( TYPE_GETTYPE(lwgeom1->type) != TYPE_GETTYPE(lwgeom2->type) )
	{
		LWDEBUG(3, " type differ");

		return 0;
	}

	if ( TYPE_GETZM(lwgeom1->type) != TYPE_GETZM(lwgeom2->type) )
	{
		LWDEBUG(3, " ZM flags differ");

		return 0;
	}

	/* Check boxes if both already computed  */
	if ( lwgeom1->bbox && lwgeom2->bbox )
	{
		/*lwnotice("bbox1:%p, bbox2:%p", lwgeom1->bbox, lwgeom2->bbox);*/
		if ( ! box2d_same(lwgeom1->bbox, lwgeom2->bbox) )
		{
			LWDEBUG(3, " bounding boxes differ");

			return 0;
		}
	}

	/* geoms have same type, invoke type-specific function */
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
