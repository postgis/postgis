/**********************************************************************
 * $Id$
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

#include "liblwgeom_internal.h"
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
	case CIRCSTRINGTYPE:
		return (LWGEOM *)lwcircstring_deserialize(srl);
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
		lwerror("lwgeom_deserialize: Unknown geometry type: %s",
			lwgeom_typename(type));

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
	case CIRCSTRINGTYPE:
		return lwcircstring_serialize_size((LWCIRCSTRING *)lwgeom);
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
		lwerror("lwgeom_serialize_size: Unknown geometry type: %s",
			lwgeom_typename(type));

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
	case CIRCSTRINGTYPE:
		lwcircstring_serialize_buf((LWCIRCSTRING *)lwgeom, buf, retsize);
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
		lwerror("lwgeom_serialize_buf: Unknown geometry type: %s",
			lwgeom_typename(type));
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

/** Force Right-hand-rule on LWGEOM polygons **/
void
lwgeom_force_rhr(LWGEOM *lwgeom)
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
			lwgeom_force_rhr(coll->geoms[i]);
		return;
	}
}

/** Reverse vertex order of LWGEOM **/
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

BOX3D *lwgeom_compute_box3d(const LWGEOM *lwgeom)
{
	if ( ! lwgeom ) return NULL;

	switch (TYPE_GETTYPE(lwgeom->type))
	{
	case POINTTYPE:
		return lwpoint_compute_box3d((LWPOINT *)lwgeom);
	case LINETYPE:
		return lwline_compute_box3d((LWLINE *)lwgeom);
	case CIRCSTRINGTYPE:
		return lwcircstring_compute_box3d((LWCIRCSTRING *)lwgeom);
	case POLYGONTYPE:
		return lwpoly_compute_box3d((LWPOLY *)lwgeom);
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTICURVETYPE:
	case MULTIPOLYGONTYPE:
	case MULTISURFACETYPE:
	case COLLECTIONTYPE:
		return lwcollection_compute_box3d((LWCOLLECTION *)lwgeom);
	}
	/* Never get here, please. */
	return NULL;
}


int
lwgeom_compute_box2d_p(LWGEOM *lwgeom, BOX2DFLOAT4 *buf)
{
	LWDEBUGF(2, "lwgeom_compute_box2d_p called of %p of type %s.",
		lwgeom, lwgeom_typename(TYPE_GETTYPE(lwgeom->type)));

	switch (TYPE_GETTYPE(lwgeom->type))
	{
	case POINTTYPE:
		return lwpoint_compute_box2d_p((LWPOINT *)lwgeom, buf);
	case LINETYPE:
		return lwline_compute_box2d_p((LWLINE *)lwgeom, buf);
	case CIRCSTRINGTYPE:
		return lwcircstring_compute_box2d_p((LWCIRCSTRING *)lwgeom, buf);
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

/**
 * do not forget to lwfree() result
 */
BOX2DFLOAT4 *
lwgeom_compute_box2d(LWGEOM *lwgeom)
{
	BOX2DFLOAT4 *result = lwalloc(sizeof(BOX2DFLOAT4));
	if ( lwgeom_compute_box2d_p(lwgeom, result) ) return result;
	else
	{
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

LWCIRCSTRING *
lwgeom_as_lwcircstring(LWGEOM *lwgeom)
{
	if ( TYPE_GETTYPE(lwgeom->type) == CIRCSTRINGTYPE )
		return (LWCIRCSTRING *)lwgeom;
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

LWGEOM *lwmpoly_as_lwgeom(LWMPOLY *obj)
{
	return (LWGEOM *)obj;
}
LWGEOM *lwmline_as_lwgeom(LWMLINE *obj)
{
	return (LWGEOM *)obj;
}
LWGEOM *lwmpoint_as_lwgeom(LWMPOINT *obj)
{
	return (LWGEOM *)obj;
}
LWGEOM *lwcollection_as_lwgeom(LWCOLLECTION *obj)
{
	return (LWGEOM *)obj;
}
LWGEOM *lwcircstring_as_lwgeom(LWCIRCSTRING *obj)
{
	return (LWGEOM *)obj;
}
LWGEOM *lwpoly_as_lwgeom(LWPOLY *obj)
{
	return (LWGEOM *)obj;
}
LWGEOM *lwline_as_lwgeom(LWLINE *obj)
{
	return (LWGEOM *)obj;
}
LWGEOM *lwpoint_as_lwgeom(LWPOINT *obj)
{
	return (LWGEOM *)obj;
}


/**
** Look-up for the correct MULTI* type promotion for singleton types.
*/
static unsigned char MULTITYPE[16] =
{
	0,
	MULTIPOINTTYPE,
	MULTILINETYPE,
	MULTIPOLYGONTYPE,
	0,0,0,0,
	MULTICURVETYPE,
	MULTICURVETYPE,
	0,0,0,
	MULTISURFACETYPE,
	0,0
};

/**
* Create a new LWGEOM of the appropriate MULTI* type.
*/
LWGEOM *
lwgeom_as_multi(LWGEOM *lwgeom)
{
	LWGEOM **ogeoms;
	LWGEOM *ogeom = NULL;
	BOX2DFLOAT4 *box = NULL;
	int type;

	ogeoms = lwalloc(sizeof(LWGEOM*));

	/*
	** This funx is a no-op only if a bbox cache is already present
	** in input.
	*/
	if ( lwgeom_is_collection(TYPE_GETTYPE(lwgeom->type)) )
	{
		return lwgeom_clone(lwgeom);
	}

	type = TYPE_GETTYPE(lwgeom->type);

	if ( MULTITYPE[type] )
	{
		ogeoms[0] = lwgeom_clone(lwgeom);

		/* Sub-geometries are not allowed to have bboxes or SRIDs, move the bbox to the collection */
		box = ogeoms[0]->bbox;
		ogeoms[0]->bbox = NULL;
		ogeoms[0]->SRID = -1;

		ogeom = (LWGEOM *)lwcollection_construct(MULTITYPE[type], lwgeom->SRID, box, 1, ogeoms);
	}
	else
	{
		return lwgeom_clone(lwgeom);
	}

	return ogeom;
}

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
	if ( lwgeom->bbox )
	{
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


/** Clone an LWGEOM object. POINTARRAY are not copied. **/
LWGEOM *
lwgeom_clone(const LWGEOM *lwgeom)
{
	LWDEBUGF(2, "lwgeom_clone called with %p, %s",
		lwgeom, lwgeom_typename(TYPE_GETTYPE(lwgeom->type)));

	switch (TYPE_GETTYPE(lwgeom->type))
	{
	case POINTTYPE:
		return (LWGEOM *)lwpoint_clone((LWPOINT *)lwgeom);
	case LINETYPE:
		return (LWGEOM *)lwline_clone((LWLINE *)lwgeom);
	case CIRCSTRINGTYPE:
		return (LWGEOM *)lwcircstring_clone((LWCIRCSTRING *)lwgeom);
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

/**
 * Add 'what' to 'to' at position 'where'
 *
 * @param where =0 == prepend if -1 == append
 *
 * Appended-to LWGEOM gets a new type based on new condition.
 * Mix of dimensions is not allowed.
 * @todo TODO: allow mix of dimensions?
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

	switch (TYPE_GETTYPE(to->type))
	{
	case POINTTYPE:
		return (LWGEOM *)lwpoint_add((const LWPOINT *)to, where, what);
	case LINETYPE:
		return (LWGEOM *)lwline_add((const LWLINE *)to, where, what);

	case CIRCSTRINGTYPE:
		return (LWGEOM *)lwcircstring_add((const LWCIRCSTRING *)to, where, what);

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
		lwerror("lwgeom_add: unknown geometry type: %s",
		        lwgeom_typename(TYPE_GETTYPE(to->type)));
		return NULL;
	}
}


/**
 * Return an alloced string
 */
char *
lwgeom_to_ewkt(LWGEOM *lwgeom, int flags)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	uchar *serialized = lwgeom_serialize(lwgeom);
	int result;

	if ( ! serialized )
	{
		lwerror("Error serializing geom %p", lwgeom);
	}

	result = unparse_WKT(&lwg_unparser_result, serialized, lwalloc, lwfree, flags);
	lwfree(serialized);

	return lwg_unparser_result.wkoutput;
}

/**
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

/**
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

/**
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
	if (result)
		lwerror("%s", (char *)lwg_parser_result.message);

	/* Free intermediate HEXified representation */
	lwfree(hexewkb);

	/* Deserialize */
	ret = lwgeom_deserialize(lwg_parser_result.serialized_lwgeom);

	return ret;
}

/**
 * Make an LWGEOM object from a EWKT representation.
 */
LWGEOM *
lwgeom_from_ewkt(char *ewkt, int flags)
{
	int result;
	LWGEOM *ret;
	LWGEOM_PARSER_RESULT lwg_parser_result;

	/* Rely on grammar parser to construct a LWGEOM */
	result = serialized_lwgeom_from_ewkt(&lwg_parser_result, ewkt, flags);
	if (result)
		lwerror("%s", (char *)lwg_parser_result.message);

	/* Deserialize */
	ret = lwgeom_deserialize(lwg_parser_result.serialized_lwgeom);

	return ret;
}

/*
 * Parser functions for working with serialized LWGEOMs. Useful for cases where
 * the function input is already serialized, e.g. some input and output functions
 */

/**
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

/**
 * Return an alloced string
 */
int
serialized_lwgeom_to_ewkt(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags)
{
	int result;

	result = unparse_WKT(lwg_unparser_result, serialized, lwalloc, lwfree, flags);

	return result;
}

/**
 * Return an alloced string
 */
int
serialized_lwgeom_from_hexwkb(LWGEOM_PARSER_RESULT *lwg_parser_result, char *hexwkb_input, int flags)
{
	/* NOTE: it is actually the same combined WKT/WKB parser that decodes HEXEWKB into LWGEOMs! */
	int result = parse_lwg(lwg_parser_result, hexwkb_input, flags,
	                       lwalloc, lwerror);

	LWDEBUGF(2, "serialized_lwgeom_from_hexwkb with %s", hexwkb_input);

	return result;
}

/**
 * Return an alloced string
 */
int
serialized_lwgeom_to_hexwkb(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags, unsigned int byteorder)
{
	int result;

	result = unparse_WKB(lwg_unparser_result, serialized, lwalloc, lwfree, flags, byteorder, 1);

	return result;
}

/**
 * Return an alloced string
 */
int
serialized_lwgeom_to_ewkb(LWGEOM_UNPARSER_RESULT *lwg_unparser_result, uchar *serialized, int flags, unsigned int byteorder)
{
	int result;

	result = unparse_WKB(lwg_unparser_result, serialized, lwalloc, lwfree, flags, byteorder, 0);

	return result;
}

/**
 * @brief geom1 same as geom2
 *  	iff
 *      	+ have same type
 *			+ have same # objects
 *      	+ have same bvol
 *      	+ each object in geom1 has a corresponding object in geom2 (see above)
 *	@param lwgeom1
 *	@param lwgeom2
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

		return LW_FALSE;
	}

	if ( TYPE_GETZM(lwgeom1->type) != TYPE_GETZM(lwgeom2->type) )
	{
		LWDEBUG(3, " ZM flags differ");

		return LW_FALSE;
	}

	/* Check boxes if both already computed  */
	if ( lwgeom1->bbox && lwgeom2->bbox )
	{
		/*lwnotice("bbox1:%p, bbox2:%p", lwgeom1->bbox, lwgeom2->bbox);*/
		if ( ! box2d_same(lwgeom1->bbox, lwgeom2->bbox) )
		{
			LWDEBUG(3, " bounding boxes differ");

			return LW_FALSE;
		}
	}

	/* geoms have same type, invoke type-specific function */
	switch (TYPE_GETTYPE(lwgeom1->type))
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
		lwerror("lwgeom_same: unsupported geometry type: %s",
		        lwgeom_typename(TYPE_GETTYPE(lwgeom1->type)));
		return LW_FALSE;
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
lwgeom_drop_bbox(LWGEOM *lwgeom)
{
	if ( lwgeom->bbox ) lwfree(lwgeom->bbox);
	lwgeom->bbox = NULL;
	TYPE_SETHASBBOX(lwgeom->type, 0);
}

/**
 * Ensure there's a box in the LWGEOM.
 * If the box is already there just return,
 * else compute it.
 */
void
lwgeom_add_bbox(LWGEOM *lwgeom)
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
	switch (TYPE_GETTYPE(lwgeom->type))
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
	switch (TYPE_GETTYPE(lwgeom->type))
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
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		coll = (LWCOLLECTION *)lwgeom;
		for (i=0; i<coll->ngeoms; i++)
			lwgeom_longitude_shift(coll->geoms[i]);
		return;
	default:
		lwerror("lwgeom_longitude_shift: unsupported geom type: %s",
		        lwgeom_typename(TYPE_GETTYPE(lwgeom->type)));
	}
}

/** Return TRUE if the geometry may contain sub-geometries, i.e. it is a MULTI* or COMPOUNDCURVE */
int
lwgeom_is_collection(int type)
{

	switch (type)
	{
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
		return -1;
		break;

	default:
		return 0;
	}
}

void lwgeom_free(LWGEOM *lwgeom)
{

	switch (TYPE_GETTYPE(lwgeom->type))
	{
	case POINTTYPE:
		lwpoint_free((LWPOINT *)lwgeom);
		break;
	case LINETYPE:
		lwline_free((LWLINE *)lwgeom);
		break;
	case POLYGONTYPE:
		lwpoly_free((LWPOLY *)lwgeom);
		break;
	case MULTIPOINTTYPE:
		lwmpoint_free((LWMPOINT *)lwgeom);
		break;
	case MULTILINETYPE:
		lwmline_free((LWMLINE *)lwgeom);
		break;
	case MULTIPOLYGONTYPE:
		lwmpoly_free((LWMPOLY *)lwgeom);
		break;
	case COLLECTIONTYPE:
		lwcollection_free((LWCOLLECTION *)lwgeom);
		break;
	}
	return;

}


int lwgeom_needs_bbox(const LWGEOM *geom)
{
	assert(geom);
	if ( TYPE_GETTYPE(geom->type) == POINTTYPE )
	{
		return LW_FALSE;
	}
	return LW_TRUE;
}


/*
** Count points in an LWGEOM.
*/

static int lwcollection_count_vertices(LWCOLLECTION *col)
{
	int i = 0;
	int v = 0; /* vertices */
	assert(col);
	for ( i = 0; i < col->ngeoms; i++ )
	{
		v += lwgeom_count_vertices(col->geoms[i]);
	}
	return v;
}

static int lwpolygon_count_vertices(LWPOLY *poly)
{
	int i = 0;
	int v = 0; /* vertices */
	assert(poly);
	for ( i = 0; i < poly->nrings; i ++ )
	{
		v += poly->rings[i]->npoints;
	}
	return v;
}

static int lwline_count_vertices(LWLINE *line)
{
	assert(line);
	if ( ! line->points )
		return 0;
	return line->points->npoints;
}

static int lwpoint_count_vertices(LWPOINT *point)
{
	assert(point);
	if ( ! point->point )
		return 0;
	return 1;
}

int lwgeom_count_vertices(LWGEOM *geom)
{
	int result = 0;
	LWDEBUGF(4, "lwgeom_count_vertices got type %s",
		lwgeom_typename(TYPE_GETTYPE(geom->type)));

	switch (TYPE_GETTYPE(geom->type))
	{
	case POINTTYPE:
		result = lwpoint_count_vertices((LWPOINT *)geom);;
		break;
	case LINETYPE:
		result = lwline_count_vertices((LWLINE *)geom);
		break;
	case POLYGONTYPE:
		result = lwpolygon_count_vertices((LWPOLY *)geom);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		result = lwcollection_count_vertices((LWCOLLECTION *)geom);
		break;
	default:
		lwerror("lwgeom_count_vertices: unsupported input geometry type: %s",
			lwgeom_typename(TYPE_GETTYPE(geom->type)));
		break;
	}
	LWDEBUGF(3, "counted %d vertices", result);
	return result;
}

static int lwpoint_is_empty(const LWPOINT *point)
{
	if ( ! point->point || point->point->npoints == 0 )
		return LW_TRUE;
	return LW_FALSE;
}

static int lwline_is_empty(const LWLINE *line)
{
	if ( !line->points || line->points->npoints == 0 )
		return LW_TRUE;
	return LW_FALSE;
}

static int lwpoly_is_empty(const LWPOLY *poly)
{
	if ( !poly->rings || poly->nrings == 0 )
		return LW_TRUE;
	return LW_FALSE;
}

static int lwcircstring_is_empty(const LWCIRCSTRING *circ)
{
	if ( !circ->points || circ->points->npoints == 0 )
		return LW_TRUE;
	return LW_FALSE;
}

static int lwcollection_is_empty(const LWCOLLECTION *col)
{
	if ( !col->geoms || col->ngeoms == 0 )
		return LW_TRUE;
	return LW_FALSE;
}

int lwgeom_is_empty(const LWGEOM *geom)
{
	int result = LW_FALSE;
	LWDEBUGF(4, "lwgeom_is_empty: got type %s",
		lwgeom_typename(TYPE_GETTYPE(geom->type)));

	switch (TYPE_GETTYPE(geom->type))
	{
	case POINTTYPE:
		return lwpoint_is_empty((LWPOINT*)geom);
		break;
	case LINETYPE:
		return lwline_is_empty((LWLINE*)geom);
		break;
	case CIRCSTRINGTYPE:
		return lwcircstring_is_empty((LWCIRCSTRING*)geom);
		break;
	case POLYGONTYPE:
		return lwpoly_is_empty((LWPOLY*)geom);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case COLLECTIONTYPE:
		return lwcollection_is_empty((LWCOLLECTION *)geom);
		break;
	default:
		lwerror("lwgeom_is_empty: unsupported input geometry type: %s",
			lwgeom_typename(TYPE_GETTYPE(geom->type)));
		break;
	}
	return result;
}

int lwgeom_has_srid(const LWGEOM *geom)
{
	if( (int)(geom->SRID) > 0 )
		return LW_TRUE;
	
	return LW_FALSE;
}


static int lwcollection_dimensionality(LWCOLLECTION *col)
{
	int i;
	int dimensionality = 0;
	for ( i = 0; i < col->ngeoms; i++ )
	{
		int d = lwgeom_dimensionality(col->geoms[i]);
		if ( d > dimensionality )
			dimensionality = d;
	}
	return dimensionality;
}

extern int lwgeom_dimensionality(LWGEOM *geom)
{
	LWDEBUGF(3, "lwgeom_dimensionality got type %s",
		lwgeom_typename(TYPE_GETTYPE(geom->type)));

	switch (TYPE_GETTYPE(geom->type))
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		return 0;
		break;
	case LINETYPE:
	case CIRCSTRINGTYPE:
	case MULTILINETYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
		return 1;
		break;
	case POLYGONTYPE:
	case CURVEPOLYTYPE:
	case MULTIPOLYGONTYPE:
	case MULTISURFACETYPE:
		return 2;
		break;
	case COLLECTIONTYPE:
		return lwcollection_dimensionality((LWCOLLECTION *)geom);
		break;
	default:
		lwerror("lwgeom_dimensionality: unsupported input geometry type: %s",
			lwgeom_typename(TYPE_GETTYPE(geom->type)));
		break;
	}
	return 0;
}

extern LWGEOM* lwgeom_remove_repeated_points(LWGEOM *in)
{
	LWDEBUGF(4, "lwgeom_remove_repeated_points got type %s",
		lwgeom_typename(TYPE_GETTYPE(in->type)));

	switch (TYPE_GETTYPE(in->type))
	{
	case MULTIPOINTTYPE:
		return lwmpoint_remove_repeated_points((LWMPOINT*)in);
		break;
	case LINETYPE:
		return lwline_remove_repeated_points((LWLINE*)in);

	case MULTILINETYPE:
	case COLLECTIONTYPE:
	case MULTIPOLYGONTYPE:
		return lwcollection_remove_repeated_points((LWCOLLECTION *)in);

	case POLYGONTYPE:
		return lwpoly_remove_repeated_points((LWPOLY *)in);
		break;

	case POINTTYPE:
		/* No point is repeated for a single point */
		return in;

	case CIRCSTRINGTYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
		/* Dunno how to handle these, will return untouched */
		return in;

	default:
		lwnotice("lwgeom_remove_repeated_points: unsupported geometry type: %s",
			lwgeom_typename(TYPE_GETTYPE(in->type)));
		return in;
		break;
	}
	return 0;
}

extern LWGEOM* lwgeom_flip_coordinates(LWGEOM *in)
{
	LWCOLLECTION *col;
	LWPOLY *poly;
	int i;

	LWDEBUGF(3, "lwgeom_flip_coordinates: unsupported type: %s",
		lwgeom_typename(TYPE_GETTYPE(in->type)));

	switch (TYPE_GETTYPE(in->type))
	{
	case POINTTYPE:
		ptarray_flip_coordinates(lwgeom_as_lwpoint(in)->point);
		return in;

	case LINETYPE:
		ptarray_flip_coordinates(lwgeom_as_lwline(in)->points);
		return in;

	case CIRCSTRINGTYPE:
		ptarray_flip_coordinates(lwgeom_as_lwcircstring(in)->points);
		return in;

	case POLYGONTYPE:
		poly = (LWPOLY *) in;
		for (i=0; i<poly->nrings; i++)
			ptarray_flip_coordinates(poly->rings[i]);
		return in;

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
	case MULTICURVETYPE:
		col = (LWCOLLECTION *) in;
		for (i=0; i<col->ngeoms; i++)
			lwgeom_flip_coordinates(col->geoms[i]);
		return in;

	default:
		lwerror("lwgeom_flip_coordinates: unsupported geometry type: %s",
			lwgeom_typename(TYPE_GETTYPE(in->type)));
	}
	return NULL;
}
