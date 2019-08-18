
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
 * Copyright 2018 Nicklas Avén
 *
 **********************************************************************/


#include "liblwgeom_internal.h"

static LWGEOM* lwgeom_filter_m_ignore_null(LWGEOM *geom,double min,double max, int returnm);

static POINTARRAY* ptarray_filterm(POINTARRAY *pa,double min, double max, int returnm)
{
	LWDEBUGF(2, "Entered %s", __func__);

	/*Check if M exists
	* This should already be tested earlier so we don't handle it properly.
	* If this happens it is because the function is used in another context than filterM
	and we throw an error*/
	if(!FLAGS_GET_M(pa->flags))
		lwerror("missing m-value in function %s\n",__func__);

	/*Dimensions in input geometry*/
	int ndims = FLAGS_NDIMS(pa->flags);

	/*Dimensions in result*/
	int res_ndims;
	if(returnm)
		res_ndims = ndims;
	else
		res_ndims = ndims-1;

	int pointsize = res_ndims * sizeof(double);

	double m_val;

	//M-value will always be the last dimension
	int m_pos = ndims-1;

	uint32_t i, counter=0;
	for(i=0;i<pa->npoints;i++)
	{
		m_val = *((double*)pa->serialized_pointlist + i*ndims + m_pos);
		if(m_val >= min && m_val <= max)
			counter++;
	}

	POINTARRAY *pa_res = ptarray_construct(FLAGS_GET_Z(pa->flags),returnm * FLAGS_GET_M(pa->flags), counter);

	double *res_cursor = (double*) pa_res->serialized_pointlist;
	for(i=0;i<pa->npoints;i++)
	{
		m_val = *((double*)pa->serialized_pointlist + i*ndims + m_pos);
		if(m_val >= min && m_val <= max)
		{
			memcpy(res_cursor, (double*) pa->serialized_pointlist + i*ndims, pointsize);
			res_cursor+=res_ndims;
		}
	}

	return pa_res;

}
static LWPOINT* lwpoint_filterm(LWPOINT  *pt,double min,double max, int returnm)
{
	LWDEBUGF(2, "Entered %s", __func__);

	POINTARRAY *pa;

	pa = ptarray_filterm(pt->point, min, max,returnm);
	if(pa->npoints < 1)
	{
		ptarray_free(pa);
		return NULL;
	}
	return lwpoint_construct(pt->srid, NULL, pa);

}

static LWLINE* lwline_filterm(LWLINE  *line,double min,double max, int returnm)
{
	LWDEBUGF(2, "Entered %s", __func__);

	POINTARRAY *pa;
	pa = ptarray_filterm(line->points, min, max,returnm);

	if(pa->npoints < 2 )
	{
		ptarray_free(pa);
		return NULL;
	}

	return lwline_construct(line->srid, NULL, pa);
}

static LWPOLY* lwpoly_filterm(LWPOLY  *poly,double min,double max, int returnm)
{
	LWDEBUGF(2, "Entered %s", __func__);
	int i, nrings;
	LWPOLY *poly_res = lwpoly_construct_empty(poly->srid, FLAGS_GET_Z(poly->flags),returnm * FLAGS_GET_M(poly->flags));

	nrings = poly->nrings;
	for( i = 0; i < nrings; i++ )
	{
		/* Ret number of points */
		POINTARRAY *pa = ptarray_filterm(poly->rings[i], min, max,returnm);

		/* Skip empty rings */
		if( pa == NULL )
			continue;

		if(pa->npoints>=4)
		{
			if (lwpoly_add_ring(poly_res, pa) == LW_FAILURE )
			{
				LWDEBUG(2, "Unable to add ring to polygon");
				lwerror("Unable to add ring to polygon");
			}
		}
		else if (i==0)
		{
			ptarray_free(pa);
			lwpoly_free(poly_res);
			return NULL;
		}
		else
			ptarray_free(pa);
	}
	return poly_res;
}



static LWCOLLECTION* lwcollection_filterm(const LWCOLLECTION *igeom,double min,double max, int returnm)
{
	LWDEBUGF(2, "Entered %s",__func__);

	uint32_t i;

	LWCOLLECTION *out = lwcollection_construct_empty(igeom->type, igeom->srid, FLAGS_GET_Z(igeom->flags),returnm * FLAGS_GET_M(igeom->flags));

	if( lwcollection_is_empty(igeom) )
		return out;

	for( i = 0; i < igeom->ngeoms; i++ )
	{
		LWGEOM *ngeom = lwgeom_filter_m_ignore_null(igeom->geoms[i],min, max,returnm);
		if ( ngeom ) out = lwcollection_add_lwgeom(out, ngeom);
	}

	return out;
}

static LWGEOM* lwgeom_filter_m_ignore_null(LWGEOM *geom,double min,double max, int returnm)
{
	LWDEBUGF(2, "Entered %s",__func__);

	LWGEOM *geom_out = NULL;

	if(!FLAGS_GET_M(geom->flags))
		return geom;
	switch ( geom->type )
	{
		case POINTTYPE:
		{
			LWDEBUGF(4,"Type found is Point, %d", geom->type);
			geom_out = lwpoint_as_lwgeom(lwpoint_filterm((LWPOINT*) geom, min, max,returnm));
			break;
		}
		case LINETYPE:
		{
			LWDEBUGF(4,"Type found is Linestring, %d", geom->type);
			geom_out = lwline_as_lwgeom(lwline_filterm((LWLINE*) geom, min,max,returnm));
			break;
		}
		/* Polygon has 'nrings' and 'rings' elements */
		case POLYGONTYPE:
		{
			LWDEBUGF(4,"Type found is Polygon, %d", geom->type);
			geom_out = lwpoly_as_lwgeom(lwpoly_filterm((LWPOLY*)geom, min, max,returnm));
			break;
		}

		/* All these Collection types have 'ngeoms' and 'geoms' elements */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
		{
			LWDEBUGF(4,"Type found is collection, %d", geom->type);
			geom_out = (LWGEOM*) lwcollection_filterm((LWCOLLECTION*) geom, min, max,returnm);
			break;
		}
		/* Unknown type! */
		default:
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(geom->type));
	}
	return geom_out;

}

LWGEOM* lwgeom_filter_m(LWGEOM *geom,double min,double max, int returnm)
{
	LWDEBUGF(2, "Entered %s",__func__);

	LWGEOM *ngeom = lwgeom_filter_m_ignore_null(geom,min, max,returnm);

	if(ngeom)
		return ngeom;
	else
	{
		switch ( geom->type )
		{
			case POINTTYPE:
			{
				return (LWGEOM*) lwpoint_construct_empty(geom->srid, FLAGS_GET_Z(geom->flags), returnm * FLAGS_GET_M(geom->flags));
				break;
			}
			case LINETYPE:
			{
				return (LWGEOM*) lwline_construct_empty(geom->srid, FLAGS_GET_Z(geom->flags), returnm * FLAGS_GET_M(geom->flags));
				break;
			}
			/* Polygon has 'nrings' and 'rings' elements */
			case POLYGONTYPE:
			{
				return (LWGEOM*) lwpoly_construct_empty(geom->srid, FLAGS_GET_Z(geom->flags), returnm * FLAGS_GET_M(geom->flags));
				break;
			}

			/* All these Collection types have 'ngeoms' and 'geoms' elements */
			case MULTIPOINTTYPE:
			case MULTILINETYPE:
			case MULTIPOLYGONTYPE:
			case COLLECTIONTYPE:
			{
				return (LWGEOM*) lwcollection_construct_empty(geom->type, geom->srid, FLAGS_GET_Z(geom->flags), returnm * FLAGS_GET_M(geom->flags));
				break;
			}
			/* Unknown type! */
			default:
				lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(geom->type));
		}
	}
	/*Shouldn't be possible*/
	return NULL;
}



