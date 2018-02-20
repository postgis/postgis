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
 * Copyright (C) 2001-2003 Refractions Research Inc.
 *
 **********************************************************************/


#include "../postgis_config.h"
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include <string.h>


/** convert decimal degress to radians */
static void
to_rad(POINT4D *pt)
{
	pt->x *= M_PI/180.0;
	pt->y *= M_PI/180.0;
}

/** convert radians to decimal degress */
static void
to_dec(POINT4D *pt)
{
	pt->x *= 180.0/M_PI;
	pt->y *= 180.0/M_PI;
}

/**
 * Transform given POINTARRAY
 * from inpj projection to outpj projection
 */
int
ptarray_transform(POINTARRAY *pa, projPJ inpj, projPJ outpj)
{
	uint32_t i;
	POINT4D p;

	for ( i = 0; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &p);
		if ( ! point4d_transform(&p, inpj, outpj) ) return LW_FAILURE;
		ptarray_set_point4d(pa, i, &p);
	}

	return LW_SUCCESS;
}


/**
 * Transform given SERIALIZED geometry
 * from inpj projection to outpj projection
 */
int
lwgeom_transform(LWGEOM *geom, projPJ inpj, projPJ outpj)
{
	uint32_t i;

	/* No points to transform in an empty! */
	if ( lwgeom_is_empty(geom) )
		return LW_SUCCESS;

	switch(geom->type)
	{
		case POINTTYPE:
		case LINETYPE:
		case CIRCSTRINGTYPE:
		case TRIANGLETYPE:
		{
			LWLINE *g = (LWLINE*)geom;
			if ( ! ptarray_transform(g->points, inpj, outpj) ) return LW_FAILURE;
			break;
		}
		case POLYGONTYPE:
		{
			LWPOLY *g = (LWPOLY*)geom;
			for ( i = 0; i < g->nrings; i++ )
			{
				if ( ! ptarray_transform(g->rings[i], inpj, outpj) ) return LW_FAILURE;
			}
			break;
		}
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
		case COMPOUNDTYPE:
		case CURVEPOLYTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case POLYHEDRALSURFACETYPE:
		case TINTYPE:
		{
			LWCOLLECTION *g = (LWCOLLECTION*)geom;
			for ( i = 0; i < g->ngeoms; i++ )
			{
				if ( ! lwgeom_transform(g->geoms[i], inpj, outpj) ) return LW_FAILURE;
			}
			break;
		}
		default:
		{
			lwerror("lwgeom_transform: Cannot handle type '%s'",
			          lwtype_name(geom->type));
			return LW_FAILURE;
		}
	}
	return LW_SUCCESS;
}

int
point4d_transform(POINT4D *pt, projPJ srcpj, projPJ dstpj)
{
	POINT3D orig_pt = {pt->x, pt->y, pt->z}; /* Copy for error report*/

	if (pj_is_latlong(srcpj)) to_rad(pt) ;

	LWDEBUGF(4, "transforming POINT(%f %f) from '%s' to '%s'",
		 orig_pt.x, orig_pt.y, pj_get_def(srcpj,0), pj_get_def(dstpj,0));

	if (pj_transform(srcpj, dstpj, 1, 0, &(pt->x), &(pt->y), &(pt->z)) != 0)
	{
		int pj_errno_val = *pj_get_errno_ref();
		if (pj_errno_val == -38)
		{
			lwnotice("PostGIS was unable to transform the point because either no grid"
				 " shift files were found, or the point does not lie within the "
				 "range for which the grid shift is defined. Refer to the "
				 "ST_Transform() section of the PostGIS manual for details on how "
				 "to configure PostGIS to alter this behaviour.");
			lwerror("transform: couldn't project point (%g %g %g): %s (%d)",
				orig_pt.x, orig_pt.y, orig_pt.z,
				pj_strerrno(pj_errno_val), pj_errno_val);
		}
		else
		{
			lwerror("transform: couldn't project point (%g %g %g): %s (%d)",
				orig_pt.x, orig_pt.y, orig_pt.z,
				pj_strerrno(pj_errno_val), pj_errno_val);
		}
		return LW_FAILURE;
	}

	if (pj_is_latlong(dstpj)) to_dec(pt);
	return LW_SUCCESS;
}

projPJ
lwproj_from_string(const char *str1)
{
	if (!str1 || str1[0] == '\0')
	{
		return NULL;
	}
	return pj_init_plus(str1);
}



