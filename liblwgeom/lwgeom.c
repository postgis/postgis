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
 * Copyright (C) 2001-2006 Refractions Research Inc.
 * Copyright (C) 2017-2018 Daniel Baston <dbaston@gmail.com>
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

#define out_stack_size 32

/** Force Right-hand-rule on LWGEOM polygons **/
void
lwgeom_force_clockwise(LWGEOM *lwgeom)
{
	LWCOLLECTION *coll;
	uint32_t i;

	switch (lwgeom->type)
	{
	case POLYGONTYPE:
		lwpoly_force_clockwise((LWPOLY *)lwgeom);
		return;

	case TRIANGLETYPE:
		lwtriangle_force_clockwise((LWTRIANGLE *)lwgeom);
		return;

		/* Not handle POLYHEDRALSURFACE and TIN
		   as they are supposed to be well oriented */
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		coll = (LWCOLLECTION *)lwgeom;
		for (i=0; i<coll->ngeoms; i++)
			lwgeom_force_clockwise(coll->geoms[i]);
		return;
	}
}

/** Check clockwise orientation on LWGEOM polygons **/
int
lwgeom_is_clockwise(LWGEOM *lwgeom)
{
	switch (lwgeom->type)
	{
		case POLYGONTYPE:
			return lwpoly_is_clockwise((LWPOLY *)lwgeom);

		case TRIANGLETYPE:
			return lwtriangle_is_clockwise((LWTRIANGLE *)lwgeom);

		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
		{
			uint32_t i;
			LWCOLLECTION* coll = (LWCOLLECTION *)lwgeom;

			for (i=0; i < coll->ngeoms; i++)
				if (!lwgeom_is_clockwise(coll->geoms[i]))
					return LW_FALSE;
			return LW_TRUE;
		}
		default:
			return LW_TRUE;
		return LW_FALSE;
	}
}

LWGEOM *
lwgeom_reverse(const LWGEOM *geom)
{
	LWGEOM *geomout = lwgeom_clone_deep(geom);
	lwgeom_reverse_in_place(geomout);
	return geomout;
}

/** Reverse vertex order of LWGEOM **/
void
lwgeom_reverse_in_place(LWGEOM *geom)
{
	uint32_t i;
	LWCOLLECTION *col;
	if (!geom)
		return;

	switch (geom->type)
	{
		case MULTIPOINTTYPE:
		case POINTTYPE:
		{
			return;
		}
		case TRIANGLETYPE:
		case CIRCSTRINGTYPE:
		case LINETYPE:
		{
			LWLINE *line = (LWLINE *)(geom);
			ptarray_reverse_in_place(line->points);
			return;
		}
		case POLYGONTYPE:
		{
			LWPOLY *poly = (LWPOLY *)(geom);
			if (!poly->rings)
				return;
			uint32_t r;
			for (r = 0; r < poly->nrings; r++)
				ptarray_reverse_in_place(poly->rings[r]);
			return;
		}
		case MULTICURVETYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case MULTISURFACETYPE:
		case POLYHEDRALSURFACETYPE:
		case TINTYPE:
		case COLLECTIONTYPE:
		case COMPOUNDTYPE:
		case CURVEPOLYTYPE:
		{
			col = (LWCOLLECTION *)(geom);
			if (!col->geoms)
				return;
			for (i=0; i<col->ngeoms; i++)
				lwgeom_reverse_in_place(col->geoms[i]);
			return;
		}
		default:
		{
			lwerror("%s: Unknown geometry type: %s", __func__, lwtype_name(geom->type));
			return;
		}

	}
}

LWLINE *
lwgeom_as_lwline(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == LINETYPE )
		return (LWLINE *)lwgeom;
	else return NULL;
}

LWCIRCSTRING *
lwgeom_as_lwcircstring(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == CIRCSTRINGTYPE )
		return (LWCIRCSTRING *)lwgeom;
	else return NULL;
}

LWCOMPOUND *
lwgeom_as_lwcompound(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == COMPOUNDTYPE )
		return (LWCOMPOUND *)lwgeom;
	else return NULL;
}

LWCURVEPOLY *
lwgeom_as_lwcurvepoly(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == CURVEPOLYTYPE )
		return (LWCURVEPOLY *)lwgeom;
	else return NULL;
}

LWPOLY *
lwgeom_as_lwpoly(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == POLYGONTYPE )
		return (LWPOLY *)lwgeom;
	else return NULL;
}

LWTRIANGLE *
lwgeom_as_lwtriangle(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == TRIANGLETYPE )
		return (LWTRIANGLE *)lwgeom;
	else return NULL;
}

LWCOLLECTION *
lwgeom_as_lwcollection(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom_is_collection(lwgeom) )
		return (LWCOLLECTION*)lwgeom;
	else return NULL;
}

LWMPOINT *
lwgeom_as_lwmpoint(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == MULTIPOINTTYPE )
		return (LWMPOINT *)lwgeom;
	else return NULL;
}

LWMLINE *
lwgeom_as_lwmline(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == MULTILINETYPE )
		return (LWMLINE *)lwgeom;
	else return NULL;
}

LWMPOLY *
lwgeom_as_lwmpoly(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == MULTIPOLYGONTYPE )
		return (LWMPOLY *)lwgeom;
	else return NULL;
}

LWPSURFACE *
lwgeom_as_lwpsurface(const LWGEOM *lwgeom)
{
	if ( lwgeom->type == POLYHEDRALSURFACETYPE )
		return (LWPSURFACE *)lwgeom;
	else return NULL;
}

LWTIN *
lwgeom_as_lwtin(const LWGEOM *lwgeom)
{
	if ( lwgeom->type == TINTYPE )
		return (LWTIN *)lwgeom;
	else return NULL;
}

LWGEOM *lwtin_as_lwgeom(const LWTIN *obj)
{
	return (LWGEOM *)obj;
}

LWGEOM *lwpsurface_as_lwgeom(const LWPSURFACE *obj)
{
	return (LWGEOM *)obj;
}

LWGEOM *lwmpoly_as_lwgeom(const LWMPOLY *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwmline_as_lwgeom(const LWMLINE *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwmpoint_as_lwgeom(const LWMPOINT *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwcollection_as_lwgeom(const LWCOLLECTION *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwcircstring_as_lwgeom(const LWCIRCSTRING *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwcurvepoly_as_lwgeom(const LWCURVEPOLY *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwcompound_as_lwgeom(const LWCOMPOUND *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwpoly_as_lwgeom(const LWPOLY *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwtriangle_as_lwgeom(const LWTRIANGLE *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwline_as_lwgeom(const LWLINE *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}
LWGEOM *lwpoint_as_lwgeom(const LWPOINT *obj)
{
	if ( obj == NULL ) return NULL;
	return (LWGEOM *)obj;
}


/**
** Look-up for the correct MULTI* type promotion for singleton types.
*/
uint8_t MULTITYPE[NUMTYPES] =
{
	0,
	MULTIPOINTTYPE,        /*  1 */
	MULTILINETYPE,         /*  2 */
	MULTIPOLYGONTYPE,      /*  3 */
	0,0,0,0,
	MULTICURVETYPE,        /*  8 */
	MULTICURVETYPE,        /*  9 */
	MULTISURFACETYPE,      /* 10 */
	POLYHEDRALSURFACETYPE, /* 11 */
	0, 0,
	TINTYPE,               /* 14 */
	0
};

uint8_t lwtype_multitype(uint8_t type)
{
	if (type > 15) return 0;
	return MULTITYPE[type];
}

/**
* Create a new LWGEOM of the appropriate MULTI* type.
*/
LWGEOM *
lwgeom_as_multi(const LWGEOM *lwgeom)
{
	LWGEOM **ogeoms;
	LWGEOM *ogeom = NULL;
	GBOX *box = NULL;
	int type;

	type = lwgeom->type;

	if ( ! MULTITYPE[type] ) return lwgeom_clone(lwgeom);

	if( lwgeom_is_empty(lwgeom) )
	{
		ogeom = (LWGEOM *)lwcollection_construct_empty(
			MULTITYPE[type],
			lwgeom->srid,
			FLAGS_GET_Z(lwgeom->flags),
			FLAGS_GET_M(lwgeom->flags)
		);
	}
	else
	{
		ogeoms = lwalloc(sizeof(LWGEOM*));
		ogeoms[0] = lwgeom_clone(lwgeom);

		/* Sub-geometries are not allowed to have bboxes or SRIDs, move the bbox to the collection */
		box = ogeoms[0]->bbox;
		ogeoms[0]->bbox = NULL;
		ogeoms[0]->srid = SRID_UNKNOWN;

		ogeom = (LWGEOM *)lwcollection_construct(MULTITYPE[type], lwgeom->srid, box, 1, ogeoms);
	}

	return ogeom;
}

/**
* Create a new LWGEOM of the appropriate CURVE* type.
*/
LWGEOM *
lwgeom_as_curve(const LWGEOM *lwgeom)
{
	LWGEOM *ogeom;
	int type = lwgeom->type;
	/*
	int hasz = FLAGS_GET_Z(lwgeom->flags);
	int hasm = FLAGS_GET_M(lwgeom->flags);
	int32_t srid = lwgeom->srid;
	*/

	switch(type)
	{
		case LINETYPE:
			/* turn to COMPOUNDCURVE */
			ogeom = (LWGEOM*)lwcompound_construct_from_lwline((LWLINE*)lwgeom);
			break;
		case POLYGONTYPE:
			ogeom = (LWGEOM*)lwcurvepoly_construct_from_lwpoly(lwgeom_as_lwpoly(lwgeom));
			break;
		case MULTILINETYPE:
			/* turn to MULTICURVE */
			ogeom = lwgeom_clone(lwgeom);
			ogeom->type = MULTICURVETYPE;
			break;
		case MULTIPOLYGONTYPE:
			/* turn to MULTISURFACE */
			ogeom = lwgeom_clone(lwgeom);
			ogeom->type = MULTISURFACETYPE;
			break;
		case COLLECTIONTYPE:
		default:
			ogeom = lwgeom_clone(lwgeom);
			break;
	}

	/* TODO: copy bbox from input geom ? */

	return ogeom;
}


/**
* Free the containing LWGEOM and the associated BOX. Leave the underlying
* geoms/points/point objects intact. Useful for functions that are stripping
* out subcomponents of complex objects, or building up new temporary objects
* on top of subcomponents.
*/
void
lwgeom_release(LWGEOM *lwgeom)
{
	if ( ! lwgeom )
		lwerror("lwgeom_release: someone called on 0x0");

	LWDEBUGF(3, "releasing type %s", lwtype_name(lwgeom->type));

	/* Drop bounding box (always a copy) */
	if ( lwgeom->bbox )
	{
		LWDEBUGF(3, "lwgeom_release: releasing bbox. %p", lwgeom->bbox);
		lwfree(lwgeom->bbox);
	}
	lwfree(lwgeom);

}


/* @brief Clone LWGEOM object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
LWGEOM *
lwgeom_clone(const LWGEOM *lwgeom)
{
	LWDEBUGF(2, "lwgeom_clone called with %p, %s",
	         lwgeom, lwtype_name(lwgeom->type));

	switch (lwgeom->type)
	{
	case POINTTYPE:
		return (LWGEOM *)lwpoint_clone((LWPOINT *)lwgeom);
	case LINETYPE:
		return (LWGEOM *)lwline_clone((LWLINE *)lwgeom);
	case CIRCSTRINGTYPE:
		return (LWGEOM *)lwcircstring_clone((LWCIRCSTRING *)lwgeom);
	case POLYGONTYPE:
		return (LWGEOM *)lwpoly_clone((LWPOLY *)lwgeom);
	case TRIANGLETYPE:
		return (LWGEOM *)lwtriangle_clone((LWTRIANGLE *)lwgeom);
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM *)lwcollection_clone((LWCOLLECTION *)lwgeom);
	default:
		lwerror("lwgeom_clone: Unknown geometry type: %s", lwtype_name(lwgeom->type));
		return NULL;
	}
}

/**
* Deep-clone an #LWGEOM object. #POINTARRAY <em>are</em> copied.
*/
LWGEOM *
lwgeom_clone_deep(const LWGEOM *lwgeom)
{
	LWDEBUGF(2, "lwgeom_clone called with %p, %s",
	         lwgeom, lwtype_name(lwgeom->type));

	switch (lwgeom->type)
	{
	case POINTTYPE:
	case LINETYPE:
	case CIRCSTRINGTYPE:
	case TRIANGLETYPE:
		return (LWGEOM *)lwline_clone_deep((LWLINE *)lwgeom);
	case POLYGONTYPE:
		return (LWGEOM *)lwpoly_clone_deep((LWPOLY *)lwgeom);
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM *)lwcollection_clone_deep((LWCOLLECTION *)lwgeom);
	default:
		lwerror("lwgeom_clone_deep: Unknown geometry type: %s", lwtype_name(lwgeom->type));
		return NULL;
	}
}


/**
 * Return an alloced string
 */
char*
lwgeom_to_ewkt(const LWGEOM *lwgeom)
{
	char* wkt = NULL;
	size_t wkt_size = 0;

	wkt = lwgeom_to_wkt(lwgeom, WKT_EXTENDED, 12, &wkt_size);

	if ( ! wkt )
	{
		lwerror("Error writing geom %p to WKT", lwgeom);
	}

	return wkt;
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
	         lwtype_name(lwgeom1->type),
	         lwtype_name(lwgeom2->type));

	if ( lwgeom1->type != lwgeom2->type )
	{
		LWDEBUG(3, " type differ");

		return LW_FALSE;
	}

	if ( FLAGS_GET_ZM(lwgeom1->flags) != FLAGS_GET_ZM(lwgeom2->flags) )
	{
		LWDEBUG(3, " ZM flags differ");

		return LW_FALSE;
	}

	/* Check boxes if both already computed  */
	if ( lwgeom1->bbox && lwgeom2->bbox )
	{
		/*lwnotice("bbox1:%p, bbox2:%p", lwgeom1->bbox, lwgeom2->bbox);*/
		if ( ! gbox_same(lwgeom1->bbox, lwgeom2->bbox) )
		{
			LWDEBUG(3, " bounding boxes differ");

			return LW_FALSE;
		}
	}

	/* geoms have same type, invoke type-specific function */
	switch (lwgeom1->type)
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
	case TRIANGLETYPE:
		return lwtriangle_same((LWTRIANGLE *)lwgeom1,
		                       (LWTRIANGLE *)lwgeom2);
	case CIRCSTRINGTYPE:
		return lwcircstring_same((LWCIRCSTRING *)lwgeom1,
					 (LWCIRCSTRING *)lwgeom2);
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return lwcollection_same((LWCOLLECTION *)lwgeom1,
		                         (LWCOLLECTION *)lwgeom2);
	default:
		lwerror("lwgeom_same: unsupported geometry type: %s",
		        lwtype_name(lwgeom1->type));
		return LW_FALSE;
	}

}

int
lwpoint_inside_circle(const LWPOINT *p, double cx, double cy, double rad)
{
	const POINT2D *pt;
	POINT2D center;

	if ( ! p || ! p->point )
		return LW_FALSE;

	pt = getPoint2d_cp(p->point, 0);

	center.x = cx;
	center.y = cy;

	if ( distance2d_pt_pt(pt, &center) < rad )
		return LW_TRUE;

	return LW_FALSE;
}

void
lwgeom_drop_bbox(LWGEOM *lwgeom)
{
	if ( lwgeom->bbox ) lwfree(lwgeom->bbox);
	lwgeom->bbox = NULL;
	FLAGS_SET_BBOX(lwgeom->flags, 0);
}

/**
 * Ensure there's a box in the LWGEOM.
 * If the box is already there just return,
 * else compute it.
 */
void
lwgeom_add_bbox(LWGEOM *lwgeom)
{
	/* an empty LWGEOM has no bbox */
	if ( lwgeom_is_empty(lwgeom) ) return;

	if ( lwgeom->bbox ) return;
	FLAGS_SET_BBOX(lwgeom->flags, 1);
	lwgeom->bbox = gbox_new(lwgeom->flags);
	lwgeom_calculate_gbox(lwgeom, lwgeom->bbox);
}

void
lwgeom_refresh_bbox(LWGEOM *lwgeom)
{
	lwgeom_drop_bbox(lwgeom);
	lwgeom_add_bbox(lwgeom);
}

void
lwgeom_add_bbox_deep(LWGEOM *lwgeom, GBOX *gbox)
{
	if ( lwgeom_is_empty(lwgeom) ) return;

	FLAGS_SET_BBOX(lwgeom->flags, 1);

	if ( ! ( gbox || lwgeom->bbox ) )
	{
		lwgeom->bbox = gbox_new(lwgeom->flags);
		lwgeom_calculate_gbox(lwgeom, lwgeom->bbox);
	}
	else if ( gbox && ! lwgeom->bbox )
	{
		lwgeom->bbox = gbox_clone(gbox);
	}

	if ( lwgeom_is_collection(lwgeom) )
	{
		uint32_t i;
		LWCOLLECTION *lwcol = (LWCOLLECTION*)lwgeom;

		for ( i = 0; i < lwcol->ngeoms; i++ )
		{
			lwgeom_add_bbox_deep(lwcol->geoms[i], lwgeom->bbox);
		}
	}
}

const GBOX *
lwgeom_get_bbox(const LWGEOM *lwg)
{
	/* add it if not already there */
	lwgeom_add_bbox((LWGEOM *)lwg);
	return lwg->bbox;
}


/**
* Calculate the gbox for this geometry, a cartesian box or
* geodetic box, depending on how it is flagged.
*/
int lwgeom_calculate_gbox(const LWGEOM *lwgeom, GBOX *gbox)
{
	gbox->flags = lwgeom->flags;
	if( FLAGS_GET_GEODETIC(lwgeom->flags) )
		return lwgeom_calculate_gbox_geodetic(lwgeom, gbox);
	else
		return lwgeom_calculate_gbox_cartesian(lwgeom, gbox);
}

void
lwgeom_drop_srid(LWGEOM *lwgeom)
{
	lwgeom->srid = SRID_UNKNOWN;	/* TODO: To be changed to SRID_UNKNOWN */
}

LWGEOM *
lwgeom_segmentize2d(const LWGEOM *lwgeom, double dist)
{
	switch (lwgeom->type)
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

LWGEOM*
lwgeom_force_2d(const LWGEOM *geom)
{
	return lwgeom_force_dims(geom, 0, 0, 0, 0);
}

LWGEOM*
lwgeom_force_3dz(const LWGEOM *geom, double zval)
{
	return lwgeom_force_dims(geom, 1, 0, zval, 0);
}

LWGEOM*
lwgeom_force_3dm(const LWGEOM *geom, double mval)
{
	return lwgeom_force_dims(geom, 0, 1, 0, mval);
}

LWGEOM*
lwgeom_force_4d(const LWGEOM *geom, double zval, double mval)
{
	return lwgeom_force_dims(geom, 1, 1, zval, mval);
}

LWGEOM*
lwgeom_force_dims(const LWGEOM *geom, int hasz, int hasm, double zval, double mval)
{
	if (!geom)
		return NULL;
	switch(geom->type)
	{
		case POINTTYPE:
			return lwpoint_as_lwgeom(lwpoint_force_dims((LWPOINT*)geom, hasz, hasm, zval, mval));
		case CIRCSTRINGTYPE:
		case LINETYPE:
		case TRIANGLETYPE:
			return lwline_as_lwgeom(lwline_force_dims((LWLINE*)geom, hasz, hasm, zval, mval));
		case POLYGONTYPE:
			return lwpoly_as_lwgeom(lwpoly_force_dims((LWPOLY*)geom, hasz, hasm, zval, mval));
		case COMPOUNDTYPE:
		case CURVEPOLYTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case POLYHEDRALSURFACETYPE:
		case TINTYPE:
		case COLLECTIONTYPE:
			return lwcollection_as_lwgeom(lwcollection_force_dims((LWCOLLECTION*)geom, hasz, hasm, zval, mval));
		default:
			lwerror("lwgeom_force_2d: unsupported geom type: %s", lwtype_name(geom->type));
			return NULL;
	}
}

LWGEOM*
lwgeom_force_sfs(LWGEOM *geom, int version)
{
	LWCOLLECTION *col;
	uint32_t i;
	LWGEOM *g;

	/* SFS 1.2 version */
	if (version == 120)
	{
		switch(geom->type)
		{
			/* SQL/MM types */
			case CIRCSTRINGTYPE:
			case COMPOUNDTYPE:
			case CURVEPOLYTYPE:
			case MULTICURVETYPE:
			case MULTISURFACETYPE:
				return lwgeom_stroke(geom, 32);

			case COLLECTIONTYPE:
				col = (LWCOLLECTION*)geom;
				for ( i = 0; i < col->ngeoms; i++ )
					col->geoms[i] = lwgeom_force_sfs((LWGEOM*)col->geoms[i], version);

				return lwcollection_as_lwgeom((LWCOLLECTION*)geom);

			default:
				return (LWGEOM *)geom;
		}
	}


	/* SFS 1.1 version */
	switch(geom->type)
	{
		/* SQL/MM types */
		case CIRCSTRINGTYPE:
		case COMPOUNDTYPE:
		case CURVEPOLYTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
			return lwgeom_stroke(geom, 32);

		/* SFS 1.2 types */
		case TRIANGLETYPE:
			g = lwpoly_as_lwgeom(lwpoly_from_lwlines((LWLINE*)geom, 0, NULL));
			lwgeom_free(geom);
			return g;

		case TINTYPE:
			col = (LWCOLLECTION*) geom;
			for ( i = 0; i < col->ngeoms; i++ )
			{
				g = lwpoly_as_lwgeom(lwpoly_from_lwlines((LWLINE*)col->geoms[i], 0, NULL));
				lwgeom_free(col->geoms[i]);
				col->geoms[i] = g;
			}
			col->type = COLLECTIONTYPE;
			return lwmpoly_as_lwgeom((LWMPOLY*)geom);

		case POLYHEDRALSURFACETYPE:
			geom->type = COLLECTIONTYPE;
			return (LWGEOM *)geom;

		/* Collection */
		case COLLECTIONTYPE:
			col = (LWCOLLECTION*)geom;
			for ( i = 0; i < col->ngeoms; i++ )
				col->geoms[i] = lwgeom_force_sfs((LWGEOM*)col->geoms[i], version);

			return lwcollection_as_lwgeom((LWCOLLECTION*)geom);

		default:
			return (LWGEOM *)geom;
	}
}

int32_t
lwgeom_get_srid(const LWGEOM *geom)
{
	if ( ! geom ) return SRID_UNKNOWN;
	return geom->srid;
}

int
lwgeom_has_z(const LWGEOM *geom)
{
	if ( ! geom ) return LW_FALSE;
	return FLAGS_GET_Z(geom->flags);
}

int
lwgeom_has_m(const LWGEOM *geom)
{
	if ( ! geom ) return LW_FALSE;
	return FLAGS_GET_M(geom->flags);
}

int
lwgeom_is_solid(const LWGEOM *geom)
{
	if ( ! geom ) return LW_FALSE;
	return FLAGS_GET_GEODETIC(geom->flags);
}

int
lwgeom_ndims(const LWGEOM *geom)
{
	if ( ! geom ) return 0;
	return FLAGS_NDIMS(geom->flags);
}



void
lwgeom_set_geodetic(LWGEOM *geom, int value)
{
	LWPOINT *pt;
	LWLINE *ln;
	LWPOLY *ply;
	LWCOLLECTION *col;
	uint32_t i;

	FLAGS_SET_GEODETIC(geom->flags, value);
	if ( geom->bbox )
		FLAGS_SET_GEODETIC(geom->bbox->flags, value);

	switch(geom->type)
	{
		case POINTTYPE:
			pt = (LWPOINT*)geom;
			if ( pt->point )
				FLAGS_SET_GEODETIC(pt->point->flags, value);
			break;
		case LINETYPE:
			ln = (LWLINE*)geom;
			if ( ln->points )
				FLAGS_SET_GEODETIC(ln->points->flags, value);
			break;
		case POLYGONTYPE:
			ply = (LWPOLY*)geom;
			for ( i = 0; i < ply->nrings; i++ )
				FLAGS_SET_GEODETIC(ply->rings[i]->flags, value);
			break;
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			col = (LWCOLLECTION*)geom;
			for ( i = 0; i < col->ngeoms; i++ )
				lwgeom_set_geodetic(col->geoms[i], value);
			break;
		default:
			lwerror("lwgeom_set_geodetic: unsupported geom type: %s", lwtype_name(geom->type));
			return;
	}
}

void
lwgeom_longitude_shift(LWGEOM *lwgeom)
{
	uint32_t i;
	switch (lwgeom->type)
	{
		LWPOINT *point;
		LWLINE *line;
		LWPOLY *poly;
		LWTRIANGLE *triangle;
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
	case TRIANGLETYPE:
		triangle = (LWTRIANGLE *)lwgeom;
		ptarray_longitude_shift(triangle->points);
		return;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		coll = (LWCOLLECTION *)lwgeom;
		for (i=0; i<coll->ngeoms; i++)
			lwgeom_longitude_shift(coll->geoms[i]);
		return;
	default:
		lwerror("lwgeom_longitude_shift: unsupported geom type: %s",
		        lwtype_name(lwgeom->type));
	}
}

int
lwgeom_is_closed(const LWGEOM *geom)
{
	int type = geom->type;

	if( lwgeom_is_empty(geom) )
		return LW_FALSE;

	/* Test linear types for closure */
	switch (type)
	{
	case LINETYPE:
		return lwline_is_closed((LWLINE*)geom);
	case POLYGONTYPE:
		return lwpoly_is_closed((LWPOLY*)geom);
	case CIRCSTRINGTYPE:
		return lwcircstring_is_closed((LWCIRCSTRING*)geom);
	case COMPOUNDTYPE:
		return lwcompound_is_closed((LWCOMPOUND*)geom);
	case TINTYPE:
		return lwtin_is_closed((LWTIN*)geom);
	case POLYHEDRALSURFACETYPE:
		return lwpsurface_is_closed((LWPSURFACE*)geom);
	}

	/* Recurse into collections and see if anything is not closed */
	if ( lwgeom_is_collection(geom) )
	{
		LWCOLLECTION *col = lwgeom_as_lwcollection(geom);
		uint32_t i;
		int closed;
		for ( i = 0; i < col->ngeoms; i++ )
		{
			closed = lwgeom_is_closed(col->geoms[i]);
			if ( ! closed )
				return LW_FALSE;
		}
		return LW_TRUE;
	}

	/* All non-linear non-collection types we will call closed */
	return LW_TRUE;
}

int
lwgeom_is_collection(const LWGEOM *geom)
{
	if( ! geom ) return LW_FALSE;
	return lwtype_is_collection(geom->type);
}

/** Return TRUE if the geometry may contain sub-geometries, i.e. it is a MULTI* or COMPOUNDCURVE */
int
lwtype_is_collection(uint8_t type)
{
	switch (type)
	{
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
	case CURVEPOLYTYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
		return LW_TRUE;
		break;

	default:
		return LW_FALSE;
	}
}

/**
* Given an lwtype number, what homogeneous collection can hold it?
*/
uint32_t
lwtype_get_collectiontype(uint8_t type)
{
	switch (type)
	{
		case POINTTYPE:
			return MULTIPOINTTYPE;
		case LINETYPE:
			return MULTILINETYPE;
		case POLYGONTYPE:
			return MULTIPOLYGONTYPE;
		case CIRCSTRINGTYPE:
			return MULTICURVETYPE;
		case COMPOUNDTYPE:
			return MULTICURVETYPE;
		case CURVEPOLYTYPE:
			return MULTISURFACETYPE;
		case TRIANGLETYPE:
			return TINTYPE;
		default:
			return COLLECTIONTYPE;
	}
}


void lwgeom_free(LWGEOM *lwgeom)
{

	/* There's nothing here to free... */
	if( ! lwgeom ) return;

	LWDEBUGF(5,"freeing a %s",lwtype_name(lwgeom->type));

	switch (lwgeom->type)
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
	case CIRCSTRINGTYPE:
		lwcircstring_free((LWCIRCSTRING *)lwgeom);
		break;
	case TRIANGLETYPE:
		lwtriangle_free((LWTRIANGLE *)lwgeom);
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
	case POLYHEDRALSURFACETYPE:
		lwpsurface_free((LWPSURFACE *)lwgeom);
		break;
	case TINTYPE:
		lwtin_free((LWTIN *)lwgeom);
		break;
	case CURVEPOLYTYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case COLLECTIONTYPE:
		lwcollection_free((LWCOLLECTION *)lwgeom);
		break;
	default:
		lwerror("lwgeom_free called with unknown type (%d) %s", lwgeom->type, lwtype_name(lwgeom->type));
	}
	return;
}

int lwgeom_needs_bbox(const LWGEOM *geom)
{
	assert(geom);
	if ( geom->type == POINTTYPE )
	{
		return LW_FALSE;
	}
	else if ( geom->type == LINETYPE )
	{
		if ( lwgeom_count_vertices(geom) <= 2 )
			return LW_FALSE;
		else
			return LW_TRUE;
	}
	else if ( geom->type == MULTIPOINTTYPE )
	{
		if ( ((LWCOLLECTION*)geom)->ngeoms == 1 )
			return LW_FALSE;
		else
			return LW_TRUE;
	}
	else if ( geom->type == MULTILINETYPE )
	{
		if ( ((LWCOLLECTION*)geom)->ngeoms == 1 && lwgeom_count_vertices(geom) <= 2 )
			return LW_FALSE;
		else
			return LW_TRUE;
	}
	else
	{
		return LW_TRUE;
	}
}

/**
* Count points in an #LWGEOM.
* TODO: Make sure the internal functions don't overflow
*/
uint32_t lwgeom_count_vertices(const LWGEOM *geom)
{
	int result = 0;

	/* Null? Zero. */
	if( ! geom ) return 0;

	LWDEBUGF(4, "lwgeom_count_vertices got type %s",
	         lwtype_name(geom->type));

	/* Empty? Zero. */
	if( lwgeom_is_empty(geom) ) return 0;

	switch (geom->type)
	{
	case POINTTYPE:
		result = 1;
		break;
	case TRIANGLETYPE:
	case CIRCSTRINGTYPE:
	case LINETYPE:
		result = lwline_count_vertices((LWLINE *)geom);
		break;
	case POLYGONTYPE:
		result = lwpoly_count_vertices((LWPOLY *)geom);
		break;
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		result = lwcollection_count_vertices((LWCOLLECTION *)geom);
		break;
	default:
		lwerror("%s: unsupported input geometry type: %s",
		        __func__, lwtype_name(geom->type));
		break;
	}
	LWDEBUGF(3, "counted %d vertices", result);
	return result;
}

/**
* For an #LWGEOM, returns 0 for points, 1 for lines,
* 2 for polygons, 3 for volume, and the max dimension
* of a collection.
*/
int lwgeom_dimension(const LWGEOM *geom)
{

	/* Null? Zero. */
	if( ! geom ) return -1;

	LWDEBUGF(4, "lwgeom_dimension got type %s",
	         lwtype_name(geom->type));

	/* Empty? Zero. */
	/* if( lwgeom_is_empty(geom) ) return 0; */

	switch (geom->type)
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		return 0;
	case CIRCSTRINGTYPE:
	case LINETYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
	case MULTILINETYPE:
		return 1;
	case TRIANGLETYPE:
	case POLYGONTYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
	case MULTIPOLYGONTYPE:
	case TINTYPE:
		return 2;
	case POLYHEDRALSURFACETYPE:
	{
		/* A closed polyhedral surface contains a volume. */
		int closed = lwpsurface_is_closed((LWPSURFACE*)geom);
		return ( closed ? 3 : 2 );
	}
	case COLLECTIONTYPE:
	{
		int maxdim = 0;
		uint32_t i;
		LWCOLLECTION *col = (LWCOLLECTION*)geom;
		for( i = 0; i < col->ngeoms; i++ )
		{
			int dim = lwgeom_dimension(col->geoms[i]);
			maxdim = ( dim > maxdim ? dim : maxdim );
		}
		return maxdim;
	}
	default:
		lwerror("%s: unsupported input geometry type: %s",
		        __func__, lwtype_name(geom->type));
	}
	return -1;
}

/**
* Count rings in an #LWGEOM.
*/
uint32_t lwgeom_count_rings(const LWGEOM *geom)
{
	int result = 0;

	/* Null? Empty? Zero. */
	if( ! geom || lwgeom_is_empty(geom) )
		return 0;

	switch (geom->type)
	{
	case POINTTYPE:
	case CIRCSTRINGTYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case LINETYPE:
		result = 0;
		break;
	case TRIANGLETYPE:
		result = 1;
		break;
	case POLYGONTYPE:
		result = ((LWPOLY *)geom)->nrings;
		break;
	case CURVEPOLYTYPE:
		result = ((LWCURVEPOLY *)geom)->nrings;
		break;
	case MULTISURFACETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
	{
		LWCOLLECTION *col = (LWCOLLECTION*)geom;
		uint32_t i = 0;
		for( i = 0; i < col->ngeoms; i++ )
			result += lwgeom_count_rings(col->geoms[i]);
		break;
	}
	default:
		lwerror("lwgeom_count_rings: unsupported input geometry type: %s", lwtype_name(geom->type));
		break;
	}
	LWDEBUGF(3, "counted %d rings", result);
	return result;
}

int lwgeom_has_srid(const LWGEOM *geom)
{
	if ( geom->srid != SRID_UNKNOWN )
		return LW_TRUE;

	return LW_FALSE;
}


static int lwcollection_dimensionality(const LWCOLLECTION *col)
{
	uint32_t i;
	int dimensionality = 0;
	for ( i = 0; i < col->ngeoms; i++ )
	{
		int d = lwgeom_dimensionality(col->geoms[i]);
		if ( d > dimensionality )
			dimensionality = d;
	}
	return dimensionality;
}

extern int lwgeom_dimensionality(const LWGEOM *geom)
{
	int dim;

	LWDEBUGF(3, "lwgeom_dimensionality got type %s",
	         lwtype_name(geom->type));

	switch (geom->type)
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
	case TRIANGLETYPE:
	case CURVEPOLYTYPE:
	case MULTIPOLYGONTYPE:
	case MULTISURFACETYPE:
		return 2;
		break;

	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
		dim = lwgeom_is_closed(geom)?3:2;
		return dim;
		break;

	case COLLECTIONTYPE:
		return lwcollection_dimensionality((const LWCOLLECTION *)geom);
		break;
	default:
		lwerror("lwgeom_dimensionality: unsupported input geometry type: %s",
		        lwtype_name(geom->type));
		break;
	}
	return 0;
}

extern LWGEOM* lwgeom_remove_repeated_points(const LWGEOM *in, double tolerance)
{
	LWGEOM *out = lwgeom_clone_deep(in);
	lwgeom_remove_repeated_points_in_place(out, tolerance);
	return out;
}

void lwgeom_swap_ordinates(LWGEOM *in, LWORD o1, LWORD o2)
{
	LWCOLLECTION *col;
	LWPOLY *poly;
	uint32_t i;

	if ( (!in) || lwgeom_is_empty(in) ) return;

  /* TODO: check for lwgeom NOT having the specified dimension ? */

	LWDEBUGF(4, "lwgeom_flip_coordinates, got type: %s",
	         lwtype_name(in->type));

	switch (in->type)
	{
	case POINTTYPE:
		ptarray_swap_ordinates(lwgeom_as_lwpoint(in)->point, o1, o2);
		break;

	case LINETYPE:
		ptarray_swap_ordinates(lwgeom_as_lwline(in)->points, o1, o2);
		break;

	case CIRCSTRINGTYPE:
		ptarray_swap_ordinates(lwgeom_as_lwcircstring(in)->points, o1, o2);
		break;

	case POLYGONTYPE:
		poly = (LWPOLY *) in;
		for (i=0; i<poly->nrings; i++)
		{
			ptarray_swap_ordinates(poly->rings[i], o1, o2);
		}
		break;

	case TRIANGLETYPE:
		ptarray_swap_ordinates(lwgeom_as_lwtriangle(in)->points, o1, o2);
		break;

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
	case MULTICURVETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
		col = (LWCOLLECTION *) in;
		for (i=0; i<col->ngeoms; i++)
		{
			lwgeom_swap_ordinates(col->geoms[i], o1, o2);
		}
		break;

	default:
		lwerror("lwgeom_swap_ordinates: unsupported geometry type: %s",
		        lwtype_name(in->type));
		return;
	}

	/* only refresh bbox if X or Y changed */
	if ( in->bbox && (o1 < 2 || o2 < 2) )
	{
		lwgeom_refresh_bbox(in);
	}
}

void lwgeom_set_srid(LWGEOM *geom, int32_t srid)
{
	uint32_t i;

	LWDEBUGF(4,"entered with srid=%d",srid);

	geom->srid = srid;

	if ( lwgeom_is_collection(geom) )
	{
		/* All the children are set to the same SRID value */
		LWCOLLECTION *col = lwgeom_as_lwcollection(geom);
		for ( i = 0; i < col->ngeoms; i++ )
		{
			lwgeom_set_srid(col->geoms[i], srid);
		}
	}
}


/**************************************************************/


int
lwgeom_remove_repeated_points_in_place(LWGEOM *geom, double tolerance)
{
	int geometry_modified = LW_FALSE;
	switch (geom->type)
	{
		/* No-op! Cannot remote points */
		case POINTTYPE:
		case TRIANGLETYPE:
			return geometry_modified;
		case LINETYPE:
		{
			LWLINE *g = (LWLINE*)(geom);
			POINTARRAY *pa = g->points;
			uint32_t npoints = pa->npoints;
			ptarray_remove_repeated_points_in_place(pa, tolerance, 2);
			geometry_modified = npoints != pa->npoints;
			/* Invalid output */
			if (pa->npoints == 1 && pa->maxpoints > 1)
			{
				/* Use first point as last point */
				pa->npoints = 2;
				ptarray_copy_point(pa, 0, 1);
			}
			break;
		}
		case POLYGONTYPE:
		{
			uint32_t i, j = 0;
			LWPOLY *g = (LWPOLY*)(geom);
			for (i = 0; i < g->nrings; i++)
			{
				POINTARRAY *pa = g->rings[i];
				int minpoints = 4;
				uint32_t npoints = 0;
				/* Skip zero'ed out rings */
				if(!pa)
					continue;
				npoints = pa->npoints;
				ptarray_remove_repeated_points_in_place(pa, tolerance, minpoints);
				geometry_modified |= npoints != pa->npoints;
				/* Drop collapsed rings */
				if(pa->npoints < 4)
				{
					geometry_modified = LW_TRUE;
					ptarray_free(pa);
					continue;
				}
				g->rings[j++] = pa;
			}
			/* Update ring count */
			g->nrings = j;
			break;
		}
		case MULTIPOINTTYPE:
		{
			double tolsq = tolerance*tolerance;
			uint32_t i, j, n = 0;
			LWMPOINT *mpt = (LWMPOINT *)(geom);
			LWPOINT **out;
			LWPOINT *out_stack[out_stack_size];
			int use_heap = (mpt->ngeoms > out_stack_size);

			/* No-op on empty */
			if (mpt->ngeoms < 2)
				return geometry_modified;

			/* We cannot write directly back to the multipoint */
			/* geoms array because we're reading out of it still */
			/* so we use a side array */
			if (use_heap)
				out = lwalloc(sizeof(LWMPOINT *) * mpt->ngeoms);
			else
				out = out_stack;

			/* Inefficient O(n^2) implementation */
			for (i = 0; i < mpt->ngeoms; i++)
			{
				int seen = 0;
				LWPOINT *p1 = mpt->geoms[i];
				const POINT2D *pt1 = getPoint2d_cp(p1->point, 0);
				for (j = 0; j < n; j++)
				{
					LWPOINT *p2 = out[j];
					const POINT2D *pt2 = getPoint2d_cp(p2->point, 0);
					if (distance2d_sqr_pt_pt(pt1, pt2) <= tolsq)
					{
						seen = 1;
						break;
					}
				}
				if (seen)
				{
					lwpoint_free(p1);
					continue;
				}
				out[n++] = p1;
			}

			/* Copy remaining points back into the input */
			/* array */
			memcpy(mpt->geoms, out, sizeof(LWPOINT *) * n);
			geometry_modified = mpt->ngeoms != n;
			mpt->ngeoms = n;
			if (use_heap) lwfree(out);
			break;
		}

		case CIRCSTRINGTYPE:
			/* Dunno how to handle these, will return untouched */
			return geometry_modified;

		/* Can process most multi* types as generic collection */
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case TINTYPE:
		case COLLECTIONTYPE:
		/* Curve types we mostly ignore, but allow the linear */
		/* portions to be processed by recursing into them */
		case MULTICURVETYPE:
		case CURVEPOLYTYPE:
		case MULTISURFACETYPE:
		case COMPOUNDTYPE:
		{
			uint32_t i, j = 0;
			LWCOLLECTION *col = (LWCOLLECTION*)(geom);
			for (i = 0; i < col->ngeoms; i++)
			{
				LWGEOM *g = col->geoms[i];
				if (!g) continue;
				geometry_modified |= lwgeom_remove_repeated_points_in_place(g, tolerance);
				/* Drop zero'ed out geometries */
				if(lwgeom_is_empty(g))
				{
					lwgeom_free(g);
					continue;
				}
				col->geoms[j++] = g;
			}
			/* Update geometry count */
			col->ngeoms = j;
			break;
		}
		default:
		{
			lwerror("%s: unsupported geometry type: %s", __func__, lwtype_name(geom->type));
			break;
		}
	}

	if (geometry_modified)
	{
		lwgeom_drop_bbox(geom);
	}
	return geometry_modified;
}


/**************************************************************/

int
lwgeom_simplify_in_place(LWGEOM *geom, double epsilon, int preserve_collapsed)
{
	int modified = LW_FALSE;
	switch (geom->type)
	{
		/* No-op! Cannot simplify points or triangles */
		case POINTTYPE:
			return modified;
		case TRIANGLETYPE:
		{
			if (preserve_collapsed)
				return modified;
			LWTRIANGLE *t = lwgeom_as_lwtriangle(geom);
			POINTARRAY *pa = t->points;
			ptarray_simplify_in_place(pa, epsilon, 0);
			if (pa->npoints < 3)
			{
				pa->npoints = 0;
				modified = LW_TRUE;
			}
			break;
		}
		case LINETYPE:
		{
			LWLINE *g = (LWLINE*)(geom);
			POINTARRAY *pa = g->points;
			uint32_t in_npoints = pa->npoints;
			ptarray_simplify_in_place(pa, epsilon, 2);
			modified = in_npoints != pa->npoints;
			/* Invalid output */
			if (pa->npoints == 1 && pa->maxpoints > 1)
			{
				/* Use first point as last point */
				if (preserve_collapsed)
				{
					pa->npoints = 2;
					ptarray_copy_point(pa, 0, 1);
				}
				/* Finish the collapse process */
				else
				{
					pa->npoints = 0;
				}
			}
			/* Duped output, force collapse */
			if (pa->npoints == 2 && !preserve_collapsed)
			{
				if (p2d_same(getPoint2d_cp(pa, 0), getPoint2d_cp(pa, 1)))
					pa->npoints = 0;
			}
			break;
		}
		case POLYGONTYPE:
		{
			uint32_t i, j = 0;
			LWPOLY *g = (LWPOLY*)(geom);
			for (i = 0; i < g->nrings; i++)
			{
				POINTARRAY *pa = g->rings[i];
				/* Only stop collapse on first ring */
				int minpoints = (preserve_collapsed && i == 0) ? 4 : 0;
				/* Skip zero'ed out rings */
				if(!pa)
					continue;
				uint32_t in_npoints = pa->npoints;
				ptarray_simplify_in_place(pa, epsilon, minpoints);
				modified |= in_npoints != pa->npoints;
				/* Drop collapsed rings */
				if(pa->npoints < 4)
				{
					if (i == 0)
					{
						/* If the outter ring is dropped, all can be dropped */
						for (i = 0; i < g->nrings; i++)
						{
							pa = g->rings[i];
							ptarray_free(pa);
						}
						break;
					}
					else
					{
						/* Drop this inner ring only */
						ptarray_free(pa);
						continue;
					}
				}
				g->rings[j++] = pa;
			}
			/* Update ring count */
			g->nrings = j;
			break;
		}
		/* Can process all multi* types as generic collection */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case TINTYPE:
		case COLLECTIONTYPE:
		{
			uint32_t i, j = 0;
			LWCOLLECTION *col = (LWCOLLECTION*)geom;
			for (i = 0; i < col->ngeoms; i++)
			{
				LWGEOM *g = col->geoms[i];
				if (!g) continue;
				modified |= lwgeom_simplify_in_place(g, epsilon, preserve_collapsed);
				/* Drop zero'ed out geometries */
				if(lwgeom_is_empty(g))
				{
					lwgeom_free(g);
					continue;
				}
				col->geoms[j++] = g;
			}
			/* Update geometry count */
			col->ngeoms = j;
			break;
		}
		default:
		{
			lwerror("%s: unsupported geometry type: %s", __func__, lwtype_name(geom->type));
			break;
		}
	}

	if (modified)
	{
		lwgeom_drop_bbox(geom);
	}
	return modified;
}

LWGEOM* lwgeom_simplify(const LWGEOM *igeom, double dist, int preserve_collapsed)
{
	LWGEOM *lwgeom_out = lwgeom_clone_deep(igeom);
	lwgeom_simplify_in_place(lwgeom_out, dist, preserve_collapsed);
	if (lwgeom_is_empty(lwgeom_out))
	{
		lwgeom_free(lwgeom_out);
		return NULL;
	}
	return lwgeom_out;
}

/**************************************************************/


double lwgeom_area(const LWGEOM *geom)
{
	int type = geom->type;

	if ( type == POLYGONTYPE )
		return lwpoly_area((LWPOLY*)geom);
	else if ( type == CURVEPOLYTYPE )
		return lwcurvepoly_area((LWCURVEPOLY*)geom);
	else if (type ==  TRIANGLETYPE )
		return lwtriangle_area((LWTRIANGLE*)geom);
	else if ( lwgeom_is_collection(geom) )
	{
		double area = 0.0;
		uint32_t i;
		LWCOLLECTION *col = (LWCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			area += lwgeom_area(col->geoms[i]);
		return area;
	}
	else
		return 0.0;
}

double lwgeom_perimeter(const LWGEOM *geom)
{
	int type = geom->type;
	if ( type == POLYGONTYPE )
		return lwpoly_perimeter((LWPOLY*)geom);
	else if ( type == CURVEPOLYTYPE )
		return lwcurvepoly_perimeter((LWCURVEPOLY*)geom);
	else if ( type == TRIANGLETYPE )
		return lwtriangle_perimeter((LWTRIANGLE*)geom);
	else if ( lwgeom_is_collection(geom) )
	{
		double perimeter = 0.0;
		uint32_t i;
		LWCOLLECTION *col = (LWCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			perimeter += lwgeom_perimeter(col->geoms[i]);
		return perimeter;
	}
	else
		return 0.0;
}

double lwgeom_perimeter_2d(const LWGEOM *geom)
{
	int type = geom->type;
	if ( type == POLYGONTYPE )
		return lwpoly_perimeter_2d((LWPOLY*)geom);
	else if ( type == CURVEPOLYTYPE )
		return lwcurvepoly_perimeter_2d((LWCURVEPOLY*)geom);
	else if ( type == TRIANGLETYPE )
		return lwtriangle_perimeter_2d((LWTRIANGLE*)geom);
	else if ( lwgeom_is_collection(geom) )
	{
		double perimeter = 0.0;
		uint32_t i;
		LWCOLLECTION *col = (LWCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			perimeter += lwgeom_perimeter_2d(col->geoms[i]);
		return perimeter;
	}
	else
		return 0.0;
}

double lwgeom_length(const LWGEOM *geom)
{
	int type = geom->type;
	if ( type == LINETYPE )
		return lwline_length((LWLINE*)geom);
	else if ( type == CIRCSTRINGTYPE )
		return lwcircstring_length((LWCIRCSTRING*)geom);
	else if ( type == COMPOUNDTYPE )
		return lwcompound_length((LWCOMPOUND*)geom);
	else if ( lwgeom_is_collection(geom) )
	{
		double length = 0.0;
		uint32_t i;
		LWCOLLECTION *col = (LWCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			length += lwgeom_length(col->geoms[i]);
		return length;
	}
	else
		return 0.0;
}

double lwgeom_length_2d(const LWGEOM *geom)
{
	int type = geom->type;
	if ( type == LINETYPE )
		return lwline_length_2d((LWLINE*)geom);
	else if ( type == CIRCSTRINGTYPE )
		return lwcircstring_length_2d((LWCIRCSTRING*)geom);
	else if ( type == COMPOUNDTYPE )
		return lwcompound_length_2d((LWCOMPOUND*)geom);
	else if ( lwgeom_is_collection(geom) )
	{
		double length = 0.0;
		uint32_t i;
		LWCOLLECTION *col = (LWCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			length += lwgeom_length_2d(col->geoms[i]);
		return length;
	}
	else
		return 0.0;
}

void
lwgeom_affine(LWGEOM *geom, const AFFINE *affine)
{
	int type = geom->type;
	uint32_t i;

	switch(type)
	{
		/* Take advantage of fact tht pt/ln/circ/tri have same memory structure */
		case POINTTYPE:
		case LINETYPE:
		case CIRCSTRINGTYPE:
		case TRIANGLETYPE:
		{
			LWLINE *l = (LWLINE*)geom;
			ptarray_affine(l->points, affine);
			break;
		}
		case POLYGONTYPE:
		{
			LWPOLY *p = (LWPOLY*)geom;
			for( i = 0; i < p->nrings; i++ )
				ptarray_affine(p->rings[i], affine);
			break;
		}
		case CURVEPOLYTYPE:
		{
			LWCURVEPOLY *c = (LWCURVEPOLY*)geom;
			for( i = 0; i < c->nrings; i++ )
				lwgeom_affine(c->rings[i], affine);
			break;
		}
		default:
		{
			if( lwgeom_is_collection(geom) )
			{
				LWCOLLECTION *c = (LWCOLLECTION*)geom;
				for( i = 0; i < c->ngeoms; i++ )
				{
					lwgeom_affine(c->geoms[i], affine);
				}
			}
			else
			{
				lwerror("lwgeom_affine: unable to handle type '%s'", lwtype_name(type));
			}
		}
	}

	/* Recompute bbox if needed */
	if (geom->bbox)
		lwgeom_refresh_bbox(geom);
}

void
lwgeom_scale(LWGEOM *geom, const POINT4D *factor)
{
	int type = geom->type;
	uint32_t i;

	switch(type)
	{
		/* Take advantage of fact tht pt/ln/circ/tri have same memory structure */
		case POINTTYPE:
		case LINETYPE:
		case CIRCSTRINGTYPE:
		case TRIANGLETYPE:
		{
			LWLINE *l = (LWLINE*)geom;
			ptarray_scale(l->points, factor);
			break;
		}
		case POLYGONTYPE:
		{
			LWPOLY *p = (LWPOLY*)geom;
			for( i = 0; i < p->nrings; i++ )
				ptarray_scale(p->rings[i], factor);
			break;
		}
		case CURVEPOLYTYPE:
		{
			LWCURVEPOLY *c = (LWCURVEPOLY*)geom;
			for( i = 0; i < c->nrings; i++ )
				lwgeom_scale(c->rings[i], factor);
			break;
		}
		default:
		{
			if( lwgeom_is_collection(geom) )
			{
				LWCOLLECTION *c = (LWCOLLECTION*)geom;
				for( i = 0; i < c->ngeoms; i++ )
				{
					lwgeom_scale(c->geoms[i], factor);
				}
			}
			else
			{
				lwerror("lwgeom_scale: unable to handle type '%s'", lwtype_name(type));
			}
		}
	}

	/* Recompute bbox if needed */
	if (geom->bbox)
		lwgeom_refresh_bbox(geom);
}

LWGEOM *
lwgeom_construct_empty(uint8_t type, int32_t srid, char hasz, char hasm)
{
	switch(type)
	{
		case POINTTYPE:
			return lwpoint_as_lwgeom(lwpoint_construct_empty(srid, hasz, hasm));
		case LINETYPE:
			return lwline_as_lwgeom(lwline_construct_empty(srid, hasz, hasm));
		case POLYGONTYPE:
			return lwpoly_as_lwgeom(lwpoly_construct_empty(srid, hasz, hasm));
		case CURVEPOLYTYPE:
			return lwcurvepoly_as_lwgeom(lwcurvepoly_construct_empty(srid, hasz, hasm));
		case CIRCSTRINGTYPE:
			return lwcircstring_as_lwgeom(lwcircstring_construct_empty(srid, hasz, hasm));
		case TRIANGLETYPE:
			return lwtriangle_as_lwgeom(lwtriangle_construct_empty(srid, hasz, hasm));
		case COMPOUNDTYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_as_lwgeom(lwcollection_construct_empty(type, srid, hasz, hasm));
		default:
			lwerror("lwgeom_construct_empty: unsupported geometry type: %s",
		        	lwtype_name(type));
			return NULL;
	}
}

int
lwgeom_startpoint(const LWGEOM *lwgeom, POINT4D *pt)
{
	if ( ! lwgeom )
		return LW_FAILURE;

	switch( lwgeom->type )
	{
		case POINTTYPE:
			return ptarray_startpoint(((LWPOINT*)lwgeom)->point, pt);
		case TRIANGLETYPE:
		case CIRCSTRINGTYPE:
		case LINETYPE:
			return ptarray_startpoint(((LWLINE*)lwgeom)->points, pt);
		case POLYGONTYPE:
			return lwpoly_startpoint((LWPOLY*)lwgeom, pt);
		case TINTYPE:
		case CURVEPOLYTYPE:
		case COMPOUNDTYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
		case POLYHEDRALSURFACETYPE:
			return lwcollection_startpoint((LWCOLLECTION*)lwgeom, pt);
		default:
			lwerror("lwgeom_startpoint: unsupported geometry type: %s", lwtype_name(lwgeom->type));
			return LW_FAILURE;
	}
}

void
lwgeom_grid_in_place(LWGEOM *geom, const gridspec *grid)
{
	if (!geom) return;
	switch ( geom->type )
	{
		case POINTTYPE:
		{
			LWPOINT *pt = (LWPOINT*)(geom);
			ptarray_grid_in_place(pt->point, grid);
			return;
		}
		case CIRCSTRINGTYPE:
		case TRIANGLETYPE:
		case LINETYPE:
		{
			LWLINE *ln = (LWLINE*)(geom);
			ptarray_grid_in_place(ln->points, grid);
			/* For invalid line, return an EMPTY */
			if (ln->points->npoints < 2)
				ln->points->npoints = 0;
			return;
		}
		case POLYGONTYPE:
		{
			LWPOLY *ply = (LWPOLY*)(geom);
			if (!ply->rings) return;

			/* Check first the external ring */
			uint32_t i = 0;
			POINTARRAY *pa = ply->rings[0];
			ptarray_grid_in_place(pa, grid);
			if (pa->npoints < 4)
			{
				/* External ring collapsed: free everything */
				for (i = 0; i < ply->nrings; i++)
				{
					ptarray_free(ply->rings[i]);
				}
				ply->nrings = 0;
				return;
			}

			/* Check the other rings */
			uint32_t j = 1;
			for (i = 1; i < ply->nrings; i++)
			{
				POINTARRAY *pa = ply->rings[i];
				ptarray_grid_in_place(pa, grid);

				/* Skip bad rings */
				if (pa->npoints >= 4)
				{
					ply->rings[j++] = pa;
				}
				else
				{
					ptarray_free(pa);
				}
			}
			/* Adjust ring count appropriately */
			ply->nrings = j;
			return;
		}
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case TINTYPE:
		case COLLECTIONTYPE:
		case COMPOUNDTYPE:
		{
			LWCOLLECTION *col = (LWCOLLECTION*)(geom);
			uint32_t i, j = 0;
			if (!col->geoms) return;
			for (i = 0; i < col->ngeoms; i++)
			{
				LWGEOM *g = col->geoms[i];
				lwgeom_grid_in_place(g, grid);
				/* Empty geoms need to be freed */
				/* before we move on */
				if (lwgeom_is_empty(g))
				{
					lwgeom_free(g);
					continue;
				}
				col->geoms[j++] = g;
			}
			col->ngeoms = j;
			return;
		}
		default:
		{
			lwerror("%s: Unsupported geometry type: %s", __func__,
			        lwtype_name(geom->type));
			return;
		}
	}
}


LWGEOM *
lwgeom_grid(const LWGEOM *lwgeom, const gridspec *grid)
{
	LWGEOM *lwgeom_out = lwgeom_clone_deep(lwgeom);
	lwgeom_grid_in_place(lwgeom_out, grid);
	return lwgeom_out;
}


/* Prototype for recursion */
static void lwgeom_subdivide_recursive(const LWGEOM *geom,
				       uint8_t dimension,
				       uint32_t maxvertices,
				       uint32_t depth,
				       LWCOLLECTION *col,
				       double gridSize);

static void
lwgeom_subdivide_recursive(const LWGEOM *geom,
			   uint8_t dimension,
			   uint32_t maxvertices,
			   uint32_t depth,
			   LWCOLLECTION *col,
			   double gridSize)
{
	const uint32_t maxdepth = 50;

	if (!geom)
		return;

	const GBOX *box_in = lwgeom_get_bbox(geom);
	if (!box_in)
		return;

	LW_ON_INTERRUPT(return);

	GBOX clip;
	gbox_duplicate(box_in, &clip);
	double width = clip.xmax - clip.xmin;
	double height = clip.ymax - clip.ymin;

	if ( geom->type == POLYHEDRALSURFACETYPE || geom->type == TINTYPE )
		lwerror("%s: unsupported geometry type '%s'", __func__, lwtype_name(geom->type));

	if ( width == 0.0 && height == 0.0 )
	{
		if ( geom->type == POINTTYPE && dimension == 0)
			lwcollection_add_lwgeom(col, lwgeom_clone_deep(geom));
		return;
	}

	if (width == 0.0)
	{
		clip.xmax += FP_TOLERANCE;
		clip.xmin -= FP_TOLERANCE;
		width = 2 * FP_TOLERANCE;
	}
	if (height == 0.0)
	{
		clip.ymax += FP_TOLERANCE;
		clip.ymin -= FP_TOLERANCE;
		height = 2 * FP_TOLERANCE;
	}

	/* Always just recurse into collections */
	if ( lwgeom_is_collection(geom) && geom->type != MULTIPOINTTYPE )
	{
		LWCOLLECTION *incol = (LWCOLLECTION*)geom;
		/* Don't increment depth yet, since we aren't actually
		 * subdividing geometries yet */
		for (uint32_t i = 0; i < incol->ngeoms; i++ )
			lwgeom_subdivide_recursive(incol->geoms[i], dimension, maxvertices, depth, col, gridSize);
		return;
	}

	if (lwgeom_dimension(geom) < dimension)
	{
		/* We've hit a lower dimension object produced by clipping at
		 * a shallower recursion level. Ignore it. */
		return;
	}

	/* But don't go too far. 2^50 ~= 10^15, that's enough subdivision */
	/* Just add what's left */
	if ( depth > maxdepth )
	{
		lwcollection_add_lwgeom(col, lwgeom_clone_deep(geom));
		return;
	}

	uint32_t nvertices = lwgeom_count_vertices(geom);

	/* Skip empties entirely */
	if (nvertices == 0)
		return;

	/* If it is under the vertex tolerance, just add it, we're done */
	if (nvertices <= maxvertices)
	{
		lwcollection_add_lwgeom(col, lwgeom_clone_deep(geom));
		return;
	}

	uint8_t split_ordinate = (width > height) ? 0 : 1;
	double center = (split_ordinate == 0) ? (clip.xmin + clip.xmax) / 2 : (clip.ymin + clip.ymax) / 2;
	double pivot = DBL_MAX;
	if (geom->type == POLYGONTYPE)
	{
		uint32_t ring_to_trim = 0;
		double ring_area = 0;
		double pivot_eps = DBL_MAX;
		double pt_eps = DBL_MAX;
		POINTARRAY *pa;
		LWPOLY *lwpoly = (LWPOLY *)geom;

		/* if there are more points in holes than in outer ring */
		if (nvertices >= 2 * lwpoly->rings[0]->npoints)
		{
			/* trim holes starting from biggest */
			for (uint32_t i = 1; i < lwpoly->nrings; i++)
			{
				double current_ring_area = fabs(ptarray_signed_area(lwpoly->rings[i]));
				if (current_ring_area >= ring_area)
				{
					ring_area = current_ring_area;
					ring_to_trim = i;
				}
			}
		}

		pa = lwpoly->rings[ring_to_trim];

		/* find most central point in chosen ring */
		for (uint32_t i = 0; i < pa->npoints; i++)
		{
			double pt;
			if (split_ordinate == 0)
				pt = getPoint2d_cp(pa, i)->x;
			else
				pt = getPoint2d_cp(pa, i)->y;
			pt_eps = fabs(pt - center);
			if (pivot_eps > pt_eps)
			{
				pivot = pt;
				pivot_eps = pt_eps;
			}
		}
	}
	GBOX subbox1, subbox2;
	gbox_duplicate(&clip, &subbox1);
	gbox_duplicate(&clip, &subbox2);

	if (pivot == DBL_MAX)
		pivot = center;

	if (split_ordinate == 0)
	{
		if (FP_NEQUALS(subbox1.xmax, pivot) && FP_NEQUALS(subbox1.xmin, pivot))
			subbox1.xmax = subbox2.xmin = pivot;
		else
			subbox1.xmax = subbox2.xmin = center;
	}
	else
	{
		if (FP_NEQUALS(subbox1.ymax, pivot) && FP_NEQUALS(subbox1.ymin, pivot))
			subbox1.ymax = subbox2.ymin = pivot;
		else
			subbox1.ymax = subbox2.ymin = center;
	}

	++depth;

	{
		LWGEOM *subbox = (LWGEOM *)lwpoly_construct_envelope(
		    geom->srid, subbox1.xmin, subbox1.ymin, subbox1.xmax, subbox1.ymax);
		LWGEOM *clipped = lwgeom_intersection_prec(geom, subbox, gridSize);
		lwgeom_simplify_in_place(clipped, 0.0, LW_TRUE);
		lwgeom_free(subbox);
		if (clipped && !lwgeom_is_empty(clipped))
		{
			lwgeom_subdivide_recursive(clipped, dimension, maxvertices, depth, col, gridSize);
			lwgeom_free(clipped);
		}
	}
	{
		LWGEOM *subbox = (LWGEOM *)lwpoly_construct_envelope(
		    geom->srid, subbox2.xmin, subbox2.ymin, subbox2.xmax, subbox2.ymax);
		LWGEOM *clipped = lwgeom_intersection_prec(geom, subbox, gridSize);
		lwgeom_simplify_in_place(clipped, 0.0, LW_TRUE);
		lwgeom_free(subbox);
		if (clipped && !lwgeom_is_empty(clipped))
		{
			lwgeom_subdivide_recursive(clipped, dimension, maxvertices, depth, col, gridSize);
			lwgeom_free(clipped);
		}
	}
}

LWCOLLECTION *
lwgeom_subdivide_prec(const LWGEOM *geom, uint32_t maxvertices, double gridSize)
{
	static uint32_t startdepth = 0;
	static uint32_t minmaxvertices = 5;
	LWCOLLECTION *col;

	col = lwcollection_construct_empty(COLLECTIONTYPE, geom->srid, lwgeom_has_z(geom), lwgeom_has_m(geom));

	if ( lwgeom_is_empty(geom) )
		return col;

	if ( maxvertices < minmaxvertices )
	{
		lwcollection_free(col);
		lwerror("%s: cannot subdivide to fewer than %d vertices per output", __func__, minmaxvertices);
	}

	lwgeom_subdivide_recursive(geom, lwgeom_dimension(geom), maxvertices, startdepth, col, gridSize);
	lwgeom_set_srid((LWGEOM*)col, geom->srid);
	return col;
}

LWCOLLECTION *
lwgeom_subdivide(const LWGEOM *geom, uint32_t maxvertices)
{
	return lwgeom_subdivide_prec(geom, maxvertices, -1);
}



int
lwgeom_is_trajectory(const LWGEOM *geom)
{
	int type = geom->type;

	if( type != LINETYPE )
	{
		lwnotice("Geometry is not a LINESTRING");
		return LW_FALSE;
	}
	return lwline_is_trajectory((LWLINE*)geom);
}

static uint8_t
bits_for_precision(int32_t significant_digits)
{
	int32_t bits_needed = ceil(significant_digits / log10(2));

	if (bits_needed > 52)
	{
		return 52;
	}
	else if (bits_needed < 1)
	{
		return 1;
	}

	return bits_needed;
}

static double trim_preserve_decimal_digits(double d, int32_t decimal_digits)
{
	if (d == 0)
		return 0;

	int digits_left_of_decimal = (int) (1 + log10(fabs(d)));
	uint8_t bits_needed = bits_for_precision(decimal_digits + digits_left_of_decimal);
	uint64_t mask = 0xffffffffffffffffULL << (52 - bits_needed);
	uint64_t dint = 0;
	size_t dsz = sizeof(d) < sizeof(dint) ? sizeof(d) : sizeof(dint);

	memcpy(&dint, &d, dsz);
	dint &= mask;
	memcpy(&d, &dint, dsz);
	return d;
}

void lwgeom_trim_bits_in_place(LWGEOM* geom, int32_t prec_x, int32_t prec_y, int32_t prec_z, int32_t prec_m)
{
	LWPOINTITERATOR* it = lwpointiterator_create_rw(geom);
	POINT4D p;

	while (lwpointiterator_has_next(it))
	{
		lwpointiterator_peek(it, &p);
		p.x = trim_preserve_decimal_digits(p.x, prec_x);
		p.y = trim_preserve_decimal_digits(p.y, prec_y);
		if (lwgeom_has_z(geom))
			p.z = trim_preserve_decimal_digits(p.z, prec_z);
		if (lwgeom_has_m(geom))
			p.m = trim_preserve_decimal_digits(p.m, prec_m);
		lwpointiterator_modify_next(it, &p);
	}

	lwpointiterator_destroy(it);
}
