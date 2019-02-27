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

/***************************************************************************/

#if POSTGIS_PROJ_VERSION < 60


static int
point4d_transform(POINT4D *pt, PJ* pj)
{
	POINT3D orig_pt = {pt->x, pt->y, pt->z}; /* Copy for error report*/

	if (pj_is_latlong(pj->pj_from)) to_rad(pt) ;

	LWDEBUGF(4, "transforming POINT(%f %f) from '%s' to '%s'",
		 orig_pt.x, orig_pt.y, pj_get_def(pj->pj_from,0), pj_get_def(pj->pj_to,0));

	if (pj_transform(pj->pj_from, pj->pj_to, 1, 0, &(pt->x), &(pt->y), &(pt->z)) != 0)
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
			lwerror("transform: %s (%d)",
				pj_strerrno(pj_errno_val), pj_errno_val);
		}
		return LW_FAILURE;
	}

	if (pj_is_latlong(pj->pj_to)) to_dec(pt);
	return LW_SUCCESS;
}

/**
 * Transform given POINTARRAY
 * from inpj projection to outpj projection
 */
int
ptarray_transform(POINTARRAY *pa, PJ* pj)
{
	uint32_t i;
	POINT4D p;

	for ( i = 0; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &p);
		if ( ! point4d_transform(&p, pj) ) return LW_FAILURE;
		ptarray_set_point4d(pa, i, &p);
	}

	return LW_SUCCESS;
}

int
lwgeom_transform_from_str(LWGEOM *geom, const char* instr, const char* outstr)
{
	char *pj_errstr;
	int rv;
	PJ pj;

	pj.pj_from = lwproj_from_string(instr);
	if (!pj.pj_from)
	{
		pj_errstr = pj_strerrno(*pj_get_errno_ref());
		if (!pj_errstr) pj_errstr = "";
		lwerror("could not parse proj string '%s'", instr);
		return LW_FAILURE;
	}

	pj.pj_to = lwproj_from_string(outstr);
	if (!pj.pj_to)
	{
		pj_free(pj.pj_from);
		pj_errstr = pj_strerrno(*pj_get_errno_ref());
		if (!pj_errstr) pj_errstr = "";
		lwerror("could not parse proj string '%s'", outstr);
		return LW_FAILURE;
	}

	rv = lwgeom_transform(geom, &pj);
	pj_free(pj.pj_from);
	pj_free(pj.pj_to);
	return rv;
}

/**
 * Transform given LWGEOM geometry
 * from inpj projection to outpj projection
 */
int
lwgeom_transform(LWGEOM *geom, PJ* pj)
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
			if ( ! ptarray_transform(g->points, pj) ) return LW_FAILURE;
			break;
		}
		case POLYGONTYPE:
		{
			LWPOLY *g = (LWPOLY*)geom;
			for ( i = 0; i < g->nrings; i++ )
			{
				if ( ! ptarray_transform(g->rings[i], pj) ) return LW_FAILURE;
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
				if ( ! lwgeom_transform(g->geoms[i], pj) ) return LW_FAILURE;
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

projPJ
lwproj_from_string(const char *str1)
{
	if (!str1 || str1[0] == '\0')
	{
		return NULL;
	}
	return pj_init_plus(str1);
}

/***************************************************************************/

#else /* POSTGIS_PROJ_VERION >= 60 */

int
lwgeom_transform_from_str(LWGEOM *geom, const char* instr, const char* outstr)
{
	PJ *pj = proj_create_crs_to_crs(NULL, instr, outstr, NULL);
	if (!pj)
	{
		PJ *pj_in = proj_create(NULL, instr);
		PJ *pj_out = proj_create(NULL, outstr);
		if (!pj_in)
		{
			lwerror("could not parse proj string '%s'", instr);
		}
		if (!pj_out)
		{
			proj_destroy(pj_in);
			lwerror("could not parse proj string '%s'", outstr);
		}
		return LW_FAILURE;
	}

	return lwgeom_transform(geom, pj);
}

int
lwgeom_transform(LWGEOM* geom, PJ* pj)
{
	uint32_t i;

	/* No points to transform in an empty! */
	if (lwgeom_is_empty(geom))
		return LW_SUCCESS;

	switch(geom->type)
	{
		case POINTTYPE:
		case LINETYPE:
		case CIRCSTRINGTYPE:
		case TRIANGLETYPE:
		{
			LWLINE *g = (LWLINE*)geom;
			if (!ptarray_transform(g->points, pj))
				return LW_FAILURE;
			break;
		}
		case POLYGONTYPE:
		{
			LWPOLY *g = (LWPOLY*)geom;
			for (i = 0; i < g->nrings; i++)
			{
				if (!ptarray_transform(g->rings[i], pj))
					return LW_FAILURE;
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
			for (i = 0; i < g->ngeoms; i++)
			{
				if (!lwgeom_transform(g->geoms[i], pj))
					return LW_FAILURE;
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

static int
proj_crs_is_swapped(const PJ* pj_crs)
{
	PJ *pj_cs;
	int rv = LW_FALSE;

	if (proj_get_type(pj_crs) == PJ_TYPE_COMPOUND_CRS)
	{
		PJ *pj_horiz_crs = proj_crs_get_sub_crs(NULL, pj_crs, 0);
		assert(pj_horiz_crs);
		pj_cs = proj_crs_get_coordinate_system(NULL, pj_horiz_crs);
		proj_destroy(pj_horiz_crs);
	}
	else if (proj_get_type(pj_crs) == PJ_TYPE_BOUND_CRS)
	{
		PJ *pj_src_crs = proj_get_source_crs(NULL, pj_crs);
		assert(pj_src_crs);
		pj_cs = proj_crs_get_coordinate_system(NULL, pj_src_crs);
		proj_destroy(pj_src_crs);
	}
	else
	{
		pj_cs = proj_crs_get_coordinate_system(NULL, pj_crs);
	}
	assert(pj_cs);
	int axis_count = proj_cs_get_axis_count(NULL, pj_cs);
	if (axis_count > 0)
	{
		const char *out_name, *out_abbrev, *out_direction;
		double out_unit_conv_factor;
		const char *out_unit_name, *out_unit_auth_name, *out_unit_code;
		/* Read only first axis, see if it is degrees / north */
		proj_cs_get_axis_info(NULL, pj_cs, 0,
			&out_name,
			&out_abbrev,
			&out_direction,
			&out_unit_conv_factor,
			&out_unit_name,
			&out_unit_auth_name,
			&out_unit_code
			);
		rv = (strcasecmp(out_direction, "north") == 0);
	}
	proj_destroy(pj_cs);
	return rv;
}


int
ptarray_transform(POINTARRAY* pa, PJ* pj)
{
	uint32_t i;
	POINT4D p;
	size_t n_converted;
	size_t n_points = pa->npoints;
	size_t point_size = ptarray_point_size(pa);
	int has_z = ptarray_has_z(pa);
	double *pa_double = (double*)(pa->serialized_pointlist);
	int input_swapped, output_swapped;

	PJ* pj_source_crs = proj_get_source_crs(NULL, pj);
	PJ* pj_target_crs = proj_get_target_crs(NULL, pj);

	if (!(pj_source_crs && pj_target_crs))
	{
		lwerror("ptarray_transform: unable to access source and target crs");
		return LW_FAILURE;
	}

	input_swapped = proj_crs_is_swapped(pj_source_crs);
	output_swapped = proj_crs_is_swapped(pj_target_crs);
	proj_destroy(pj_source_crs);
	proj_destroy(pj_target_crs);

	/* Convert to radians if necessary */
	if (proj_angular_input(pj, PJ_FWD))
	{
		for (i = 0; i < pa->npoints; i++)
		{
			getPoint4d_p(pa, i, &p);
			to_rad(&p);
		}
	}

	if (input_swapped)
		ptarray_swap_ordinates(pa, LWORD_X, LWORD_Y);

	/*
	* size_t proj_trans_generic(PJ *P, PJ_DIRECTION direction,
	* double *x, size_t sx, size_t nx,
	* double *y, size_t sy, size_t ny,
	* double *z, size_t sz, size_t nz,
	* double *t, size_t st, size_t nt)
	*/

	n_converted = proj_trans_generic(
		pj, PJ_FWD,
		pa_double,     point_size, n_points, /* X */
		pa_double + 1, point_size, n_points, /* Y */
		has_z ? pa_double + 2 : NULL,
		has_z ? point_size : 0,
		has_z ? n_points : 0, /* Z */
		NULL, 0, 0 /* M */
		);

	if (n_converted != n_points)
	{
		lwerror("ptarray_transform: converted (%d) != input (%d)",
			n_converted, n_points);
		return LW_FAILURE;
	}

	int pj_errno_val = proj_errno(pj);
	if (pj_errno_val)
	{
		lwerror("transform: %s (%d)",
			proj_errno_string(pj_errno_val), pj_errno_val);
		return LW_FAILURE;
	}

	if (output_swapped)
		ptarray_swap_ordinates(pa, LWORD_X, LWORD_Y);

	/* Convert radians to degrees if necessary */
	if (proj_angular_output(pj, PJ_FWD))
	{
		for (i = 0; i < pa->npoints; i++)
		{
			getPoint4d_p(pa, i, &p);
			to_dec(&p);
		}
	}

	return LW_SUCCESS;
}

#endif
