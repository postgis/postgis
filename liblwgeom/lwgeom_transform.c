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

LWPROJ *
lwproj_from_str(const char* str_in, const char* str_out)
{
	uint8_t source_is_latlong = LW_FALSE;
	double semi_major_metre = DBL_MAX, semi_minor_metre = DBL_MAX;

	/* Usable inputs? */
	if (! (str_in && str_out))
		return NULL;

	PJ* pj = proj_create_crs_to_crs(PJ_DEFAULT_CTX, str_in, str_out, NULL);
	if (!pj)
		return NULL;

	/* Fill in geodetic parameter information when a null-transform */
	/* is passed, because that's how we signal we want to store */
	/* that info in the cache */
	if (strcmp(str_in, str_out) == 0)
	{
		PJ *pj_source_crs = proj_get_source_crs(PJ_DEFAULT_CTX, pj);
		PJ *pj_ellps;
		PJ_TYPE pj_type = proj_get_type(pj_source_crs);
		if (pj_type == PJ_TYPE_UNKNOWN)
		{
			proj_destroy(pj);
			lwerror("%s: unable to access source crs type", __func__);
			return NULL;
		}
		source_is_latlong = (pj_type == PJ_TYPE_GEOGRAPHIC_2D_CRS) || (pj_type == PJ_TYPE_GEOGRAPHIC_3D_CRS);

		pj_ellps = proj_get_ellipsoid(PJ_DEFAULT_CTX, pj_source_crs);
		proj_destroy(pj_source_crs);
		if (!pj_ellps)
		{
			proj_destroy(pj);
			lwerror("%s: unable to access source crs ellipsoid", __func__);
			return NULL;
		}
		if (!proj_ellipsoid_get_parameters(PJ_DEFAULT_CTX,
						   pj_ellps,
						   &semi_major_metre,
						   &semi_minor_metre,
						   NULL,
						   NULL))
		{
			proj_destroy(pj_ellps);
			proj_destroy(pj);
			lwerror("%s: unable to access source crs ellipsoid parameters", __func__);
			return NULL;
		}
		proj_destroy(pj_ellps);
	}

	/* Add in an axis swap if necessary */
	PJ* pj_norm = proj_normalize_for_visualization(PJ_DEFAULT_CTX, pj);
	/* Swap failed for some reason? Fall back to coordinate operation */
	if (!pj_norm)
		pj_norm = pj;
	/* Swap is not a copy of input? Clean up input */
	else if (pj != pj_norm)
		proj_destroy(pj);

	/* Allocate and populate return value */
	LWPROJ *lp = lwalloc(sizeof(LWPROJ));
	lp->pj = pj_norm; /* Caller is going to have to explicitly proj_destroy this */
	lp->pipeline_is_forward = true;
	lp->source_is_latlong = source_is_latlong;
	lp->source_semi_major_metre = semi_major_metre;
	lp->source_semi_minor_metre = semi_minor_metre;
	return lp;
}

LWPROJ *
lwproj_from_str_pipeline(const char* str_pipeline, bool is_forward)
{
	/* Usable inputs? */
	if (!str_pipeline)
		return NULL;

	PJ* pj = proj_create(PJ_DEFAULT_CTX, str_pipeline);
	if (!pj)
		return NULL;

	/* check we have a transform, not a crs */
	if (proj_is_crs(pj))
		return NULL;

	/* Add in an axis swap if necessary */
	PJ* pj_norm = proj_normalize_for_visualization(PJ_DEFAULT_CTX, pj);
	if (!pj_norm)
		pj_norm = pj;
	/* Swap is not a copy of input? Clean up input */
	else if (pj != pj_norm)
		proj_destroy(pj);

	/* Allocate and populate return value */
	LWPROJ *lp = lwalloc(sizeof(LWPROJ));
	lp->pj = pj_norm; /* Caller is going to have to explicitly proj_destroy this */
	lp->pipeline_is_forward = is_forward;

	/* this is stuff for geography calculations; doesn't matter here */
	lp->source_is_latlong = LW_FALSE;
	lp->source_semi_major_metre = DBL_MAX;
	lp->source_semi_minor_metre = DBL_MAX;
	return lp;
}

int
lwgeom_transform_from_str(LWGEOM *geom, const char* instr, const char* outstr)
{
	LWPROJ *lp = lwproj_from_str(instr, outstr);
	if (!lp)
	{
		PJ *pj_in = proj_create(PJ_DEFAULT_CTX, instr);
		if (!pj_in)
		{
			proj_errno_reset(NULL);
			lwerror("could not parse proj string '%s'", instr);
		}
		proj_destroy(pj_in);

		PJ *pj_out = proj_create(PJ_DEFAULT_CTX, outstr);
		if (!pj_out)
		{
			proj_errno_reset(NULL);
			lwerror("could not parse proj string '%s'", outstr);
		}
		proj_destroy(pj_out);
		lwerror("%s: Failed to transform", __func__);
		return LW_FAILURE;
	}
	int ret = lwgeom_transform(geom, lp);
	proj_destroy(lp->pj);
	lwfree(lp);
	return ret;
}

int
lwgeom_transform_pipeline(LWGEOM *geom, const char* pipelinestr, bool is_forward)
{
	LWPROJ *lp = lwproj_from_str_pipeline(pipelinestr, is_forward);
	if (!lp)
	{
		PJ *pj_in = proj_create(PJ_DEFAULT_CTX, pipelinestr);
		if (!pj_in)
		{
			proj_errno_reset(NULL);
			lwerror("could not parse coordinate operation '%s'", pipelinestr);
		}
		proj_destroy(pj_in);
		lwerror("%s: Failed to transform", __func__);
		return LW_FAILURE;
	}
	int ret = lwgeom_transform(geom, lp);
	proj_destroy(lp->pj);
	lwfree(lp);
	return ret;
}

int
ptarray_transform(POINTARRAY *pa, LWPROJ *pj)
{
	uint32_t i;
	POINT4D p;
	size_t n_converted;
	size_t n_points = pa->npoints;
	size_t point_size = ptarray_point_size(pa);
	int has_z = ptarray_has_z(pa);
	double *pa_double = (double*)(pa->serialized_pointlist);

	PJ_DIRECTION direction = pj->pipeline_is_forward ? PJ_FWD : PJ_INV;

	/* Convert to radians if necessary */
	if (proj_angular_input(pj->pj, direction))
	{
		for (i = 0; i < pa->npoints; i++)
		{
			getPoint4d_p(pa, i, &p);
			to_rad(&p);
			ptarray_set_point4d(pa, i, &p);
		}
	}

	if (n_points == 1)
	{
		/* For single points it's faster to call proj_trans */
		PJ_XYZT v = {pa_double[0], pa_double[1], has_z ? pa_double[2] : 0.0, 0.0};
		PJ_COORD c;
		c.xyzt = v;
		PJ_COORD t = proj_trans(pj->pj, direction, c);

		int pj_errno_val = proj_errno_reset(pj->pj);
		if (pj_errno_val)
		{
			lwerror("transform: %s (%d)", proj_errno_string(pj_errno_val), pj_errno_val);
			return LW_FAILURE;
		}
		pa_double[0] = (t.xyzt).x;
		pa_double[1] = (t.xyzt).y;
		if (has_z)
			pa_double[2] = (t.xyzt).z;
	}
	else
	{
		/*
		 * size_t proj_trans_generic(PJ *P, PJ_DIRECTION direction,
		 * double *x, size_t sx, size_t nx,
		 * double *y, size_t sy, size_t ny,
		 * double *z, size_t sz, size_t nz,
		 * double *t, size_t st, size_t nt)
		 */

		n_converted = proj_trans_generic(pj->pj,
						 direction,
						 pa_double,
						 point_size,
						 n_points, /* X */
						 pa_double + 1,
						 point_size,
						 n_points, /* Y */
						 has_z ? pa_double + 2 : NULL,
						 has_z ? point_size : 0,
						 has_z ? n_points : 0, /* Z */
						 NULL,
						 0,
						 0 /* M */
		);

		if (n_converted != n_points)
		{
			lwerror("ptarray_transform: converted (%d) != input (%d)", n_converted, n_points);
			return LW_FAILURE;
		}

		int pj_errno_val = proj_errno_reset(pj->pj);
		if (pj_errno_val)
		{
			lwerror("transform: %s (%d)", proj_errno_string(pj_errno_val), pj_errno_val);
			return LW_FAILURE;
		}
	}

	/* Convert radians to degrees if necessary */
	if (proj_angular_output(pj->pj, direction))
	{
		for (i = 0; i < pa->npoints; i++)
		{
			getPoint4d_p(pa, i, &p);
			to_dec(&p);
			ptarray_set_point4d(pa, i, &p);
		}
	}

	return LW_SUCCESS;
}

/**
 * Transform given LWGEOM geometry
 * from inpj projection to outpj projection
 */
int
lwgeom_transform(LWGEOM *geom, LWPROJ *pj)
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
