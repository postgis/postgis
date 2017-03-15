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
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"


/** Force Right-hand-rule on LWGEOM polygons **/
void
lwgeom_force_clockwise(LWGEOM *lwgeom)
{
	LWCOLLECTION *coll;
	int i;

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
			int i;
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

/** Reverse vertex order of LWGEOM **/
void
lwgeom_reverse(LWGEOM *lwgeom)
{
	int i;
	LWCOLLECTION *col;

	switch (lwgeom->type)
	{
	case LINETYPE:
		lwline_reverse((LWLINE *)lwgeom);
		return;
	case POLYGONTYPE:
		lwpoly_reverse((LWPOLY *)lwgeom);
		return;
	case TRIANGLETYPE:
		lwtriangle_reverse((LWTRIANGLE *)lwgeom);
		return;
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		col = (LWCOLLECTION *)lwgeom;
		for (i=0; i<col->ngeoms; i++)
			lwgeom_reverse(col->geoms[i]);
		return;
	}
}

LWPOINT *
lwgeom_as_lwpoint(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == POINTTYPE )
		return (LWPOINT *)lwgeom;
	else return NULL;
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
	int srid = lwgeom->srid;
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
		int i;
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
* Calculate the gbox for this goemetry, a cartesian box or
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
lwgeom_segmentize2d(LWGEOM *lwgeom, double dist)
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
	return lwgeom_force_dims(geom, 0, 0);
}

LWGEOM*
lwgeom_force_3dz(const LWGEOM *geom)
{	
	return lwgeom_force_dims(geom, 1, 0);
}

LWGEOM*
lwgeom_force_3dm(const LWGEOM *geom)
{	
	return lwgeom_force_dims(geom, 0, 1);
}

LWGEOM*
lwgeom_force_4d(const LWGEOM *geom)
{	
	return lwgeom_force_dims(geom, 1, 1);
}

LWGEOM*
lwgeom_force_dims(const LWGEOM *geom, int hasz, int hasm)
{	
	switch(geom->type)
	{
		case POINTTYPE:
			return lwpoint_as_lwgeom(lwpoint_force_dims((LWPOINT*)geom, hasz, hasm));
		case CIRCSTRINGTYPE:
		case LINETYPE:
		case TRIANGLETYPE:
			return lwline_as_lwgeom(lwline_force_dims((LWLINE*)geom, hasz, hasm));
		case POLYGONTYPE:
			return lwpoly_as_lwgeom(lwpoly_force_dims((LWPOLY*)geom, hasz, hasm));
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
			return lwcollection_as_lwgeom(lwcollection_force_dims((LWCOLLECTION*)geom, hasz, hasm));
		default:
			lwerror("lwgeom_force_2d: unsupported geom type: %s", lwtype_name(geom->type));
			return NULL;
	}
}

LWGEOM*
lwgeom_force_sfs(LWGEOM *geom, int version)
{	
	LWCOLLECTION *col;
	int i;
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

uint32_t
lwgeom_get_type(const LWGEOM *geom)
{
	if ( ! geom ) return 0;
	return geom->type;
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
	int i;
	
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
	int i;
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
		int i;
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
int
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
*/
int lwgeom_count_vertices(const LWGEOM *geom)
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
		int maxdim = 0, i;
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
int lwgeom_count_rings(const LWGEOM *geom)
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
		int i = 0;
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

int lwgeom_is_empty(const LWGEOM *geom)
{
	int result = LW_FALSE;
	LWDEBUGF(4, "lwgeom_is_empty: got type %s",
	         lwtype_name(geom->type));

	switch (geom->type)
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
	case TRIANGLETYPE:
		return lwtriangle_is_empty((LWTRIANGLE*)geom);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return lwcollection_is_empty((LWCOLLECTION *)geom);
		break;
	default:
		lwerror("lwgeom_is_empty: unsupported input geometry type: %s",
		        lwtype_name(geom->type));
		break;
	}
	return result;
}

int lwgeom_has_srid(const LWGEOM *geom)
{
	if ( geom->srid != SRID_UNKNOWN )
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
		return lwcollection_dimensionality((LWCOLLECTION *)geom);
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
	LWDEBUGF(4, "lwgeom_remove_repeated_points got type %s",
	         lwtype_name(in->type));

	if(lwgeom_is_empty(in))
	{
		return lwgeom_clone_deep(in);
	}

	switch (in->type)
	{
	case MULTIPOINTTYPE:
		return lwmpoint_remove_repeated_points((LWMPOINT*)in, tolerance);
		break;
	case LINETYPE:
		return lwline_remove_repeated_points((LWLINE*)in, tolerance);

	case MULTILINETYPE:
	case COLLECTIONTYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
		return lwcollection_remove_repeated_points((LWCOLLECTION *)in, tolerance);

	case POLYGONTYPE:
		return lwpoly_remove_repeated_points((LWPOLY *)in, tolerance);
		break;

	case POINTTYPE:
	case TRIANGLETYPE:
	case TINTYPE:
		/* No point is repeated for a single point, or for Triangle or TIN */
		return lwgeom_clone_deep(in);

	case CIRCSTRINGTYPE:
	case COMPOUNDTYPE:
	case MULTICURVETYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
		/* Dunno how to handle these, will return untouched */
		return lwgeom_clone_deep(in);

	default:
		lwnotice("%s: unsupported geometry type: %s",
		         __func__, lwtype_name(in->type));
		return lwgeom_clone_deep(in);
		break;
	}
	return 0;
}

LWGEOM* lwgeom_flip_coordinates(LWGEOM *in)
{
  lwgeom_swap_ordinates(in,LWORD_X,LWORD_Y);
  return in;
}

void lwgeom_swap_ordinates(LWGEOM *in, LWORD o1, LWORD o2)
{
	LWCOLLECTION *col;
	LWPOLY *poly;
	int i;

#if PARANOIA_LEVEL > 0
  assert(o1 < 4);
  assert(o2 < 4);
#endif

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
		lwgeom_drop_bbox(in);
		lwgeom_add_bbox(in);
	}
}

void lwgeom_set_srid(LWGEOM *geom, int32_t srid)
{
	int i;

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

LWGEOM* lwgeom_simplify(const LWGEOM *igeom, double dist, int preserve_collapsed)
{
	switch (igeom->type)
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		return lwgeom_clone(igeom);
	case LINETYPE:
		return (LWGEOM*)lwline_simplify((LWLINE*)igeom, dist, preserve_collapsed);
	case POLYGONTYPE:
		return (LWGEOM*)lwpoly_simplify((LWPOLY*)igeom, dist, preserve_collapsed);
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM*)lwcollection_simplify((LWCOLLECTION *)igeom, dist, preserve_collapsed);
	default:
		lwerror("%s: unsupported geometry type: %s", __func__, lwtype_name(igeom->type));
	}
	return NULL;
}

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
		int i;
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
		int i;
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
		int i;
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
		int i;
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
		int i;
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
	int i;

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

}

void
lwgeom_scale(LWGEOM *geom, const POINT4D *factor)
{
	int type = geom->type;
	int i;

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

	if ( geom->bbox )
	{
		/* TODO: expose a gbox_scale function */
		geom->bbox->xmin *= factor->x;
		geom->bbox->xmax *= factor->x;
		geom->bbox->ymin *= factor->y;
		geom->bbox->ymax *= factor->y;
		geom->bbox->zmin *= factor->z;
		geom->bbox->zmax *= factor->z;
		geom->bbox->mmin *= factor->m;
		geom->bbox->mmax *= factor->m;
	}
}

LWGEOM*
lwgeom_construct_empty(uint8_t type, int srid, char hasz, char hasm)
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
lwgeom_startpoint(const LWGEOM* lwgeom, POINT4D* pt)
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
		case CURVEPOLYTYPE:
		case COMPOUNDTYPE:
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_startpoint((LWCOLLECTION*)lwgeom, pt);
		default:
			lwerror("int: unsupported geometry type: %s",
		        	lwtype_name(lwgeom->type));
			return LW_FAILURE;
	}
}


LWGEOM *
lwgeom_grid(const LWGEOM *lwgeom, const gridspec *grid)
{
	switch ( lwgeom->type )
	{
		case POINTTYPE:
			return (LWGEOM *)lwpoint_grid((LWPOINT *)lwgeom, grid);
		case LINETYPE:
			return (LWGEOM *)lwline_grid((LWLINE *)lwgeom, grid);
		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_grid((LWPOLY *)lwgeom, grid);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
		case COMPOUNDTYPE:
			return (LWGEOM *)lwcollection_grid((LWCOLLECTION *)lwgeom, grid);
		case CIRCSTRINGTYPE:
			return (LWGEOM *)lwcircstring_grid((LWCIRCSTRING *)lwgeom, grid);
		default:
			lwerror("lwgeom_grid: Unsupported geometry type: %s",
			        lwtype_name(lwgeom->type));
			return NULL;
	}
}


/* Prototype for recursion */
static int
lwgeom_subdivide_recursive(const LWGEOM *geom, int maxvertices, int depth, LWCOLLECTION *col, const GBOX *clip);

static int
lwgeom_subdivide_recursive(const LWGEOM *geom, int maxvertices, int depth, LWCOLLECTION *col, const GBOX *clip)
{
	const int maxdepth = 50;
	int nvertices = 0;
	int i, n = 0;
	double width = clip->xmax - clip->xmin;
	double height = clip->ymax - clip->ymin;
	GBOX subbox1, subbox2;
	LWGEOM *clipped1, *clipped2;
	
	if ( geom->type == POLYHEDRALSURFACETYPE || geom->type == TINTYPE )
	{
		lwerror("%s: unsupported geometry type '%s'", __func__, lwtype_name(geom->type));
	}
	
	if ( width == 0.0 && height == 0.0 )
	{
		if ( geom->type == POINTTYPE )
		{
			lwcollection_add_lwgeom(col, lwgeom_clone_deep(geom));
			return 1;			
		}
		else
		{
			return 0;
		}
	}
	
	/* Always just recurse into collections */
	if ( lwgeom_is_collection(geom) && geom->type != MULTIPOINTTYPE )
	{
		LWCOLLECTION *incol = (LWCOLLECTION*)geom;
		int n = 0;
		for ( i = 0; i < incol->ngeoms; i++ )
		{
			/* Don't increment depth yet, since we aren't actually subdividing geomtries yet */
			n += lwgeom_subdivide_recursive(incol->geoms[i], maxvertices, depth, col, clip);
		}
		return n;
	}
	
	/* But don't go too far. 2^25 = 33M, that's enough subdivision */
	/* Signal callers above that we depth'ed out with a negative */
	/* return value */
	if ( depth > maxdepth )
	{
		return 0;
	}
	
	nvertices = lwgeom_count_vertices(geom);
	/* Skip empties entirely */
	if ( nvertices == 0 )
	{
		return 0;
	}
	
	/* If it is under the vertex tolerance, just add it, we're done */
	if ( nvertices < maxvertices )
	{
		lwcollection_add_lwgeom(col, lwgeom_clone_deep(geom));
		return 1;
	}
	
	subbox1 = subbox2 = *clip;
	if ( width > height )
	{
		subbox1.xmax = subbox2.xmin = (clip->xmin + clip->xmax)/2;
	}
	else
	{
		subbox1.ymax = subbox2.ymin = (clip->ymin + clip->ymax)/2;
	}
	
	if ( height == 0 )
	{
		subbox1.ymax += FP_TOLERANCE;
		subbox2.ymax += FP_TOLERANCE;
		subbox1.ymin -= FP_TOLERANCE;
		subbox2.ymin -= FP_TOLERANCE;
	}

	if ( width == 0 )
	{
		subbox1.xmax += FP_TOLERANCE;
		subbox2.xmax += FP_TOLERANCE;
		subbox1.xmin -= FP_TOLERANCE;
		subbox2.xmin -= FP_TOLERANCE;
	}
		
	clipped1 = lwgeom_clip_by_rect(geom, subbox1.xmin, subbox1.ymin, subbox1.xmax, subbox1.ymax);
	clipped2 = lwgeom_clip_by_rect(geom, subbox2.xmin, subbox2.ymin, subbox2.xmax, subbox2.ymax);
	
	if ( clipped1 )
	{
		n += lwgeom_subdivide_recursive(clipped1, maxvertices, ++depth, col, &subbox1);
		lwgeom_free(clipped1);
	}

	if ( clipped2 )
	{
		n += lwgeom_subdivide_recursive(clipped2, maxvertices, ++depth, col, &subbox2);
		lwgeom_free(clipped2);
	}
	
	return n;
	
}

LWCOLLECTION *
lwgeom_subdivide(const LWGEOM *geom, int maxvertices)
{
	static int startdepth = 0;
	static int minmaxvertices = 8;
	LWCOLLECTION *col;
	GBOX clip;
	
	col = lwcollection_construct_empty(COLLECTIONTYPE, geom->srid, lwgeom_has_z(geom), lwgeom_has_m(geom));

	if ( lwgeom_is_empty(geom) )
		return col;

	if ( maxvertices < minmaxvertices )
	{
		lwcollection_free(col);
		lwerror("%s: cannot subdivide to fewer than %d vertices per output", __func__, minmaxvertices);
	}
	
	clip = *(lwgeom_get_bbox(geom));
	lwgeom_subdivide_recursive(geom, maxvertices, startdepth, col, &clip);
	lwgeom_set_srid((LWGEOM*)col, geom->srid);
	return col;
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

