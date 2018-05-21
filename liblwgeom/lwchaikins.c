
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



static POINTARRAY * ptarray_chaikin(POINTARRAY *inpts, int preserve_endpoint, int isclosed)
{
	uint32_t p, i, n_out_points=0, p1_set=0, p2_set=0;
	POINT4D p1, p2;
	POINTARRAY *opts;
	double *dlist;
	double deltaval, quarter_delta, val1, val2;
	uint32_t ndims = 2 + ptarray_has_z(inpts) + ptarray_has_m(inpts);
	int new_npoints = inpts->npoints * 2;
	opts = ptarray_construct_empty(FLAGS_GET_Z(inpts->flags), FLAGS_GET_M(inpts->flags), new_npoints);

	dlist = (double*)(opts->serialized_pointlist);

	p1 = getPoint4d(inpts, 0);
	/*if first point*/
	if(preserve_endpoint)
	{
		ptarray_append_point(opts, &p1, LW_TRUE);
		n_out_points++;
	}

	for (p=1;p<inpts->npoints;p++)
	{
		memcpy(&p2, &p1, sizeof(POINT4D));
		p1 = getPoint4d(inpts, p);
		if(p>0)
		{
			p1_set = p2_set = 0;
			for (i=0;i<ndims;i++)
			{
				val1 = ((double*) &(p1))[i];
				val2 = ((double*) &(p2))[i];
				deltaval =  val1 - val2;
				quarter_delta = deltaval * 0.25;
				if(!preserve_endpoint || p > 1)
				{
					dlist[n_out_points * ndims + i] = val2 + quarter_delta;
					p1_set = 1;
				}
				if(!preserve_endpoint || p < inpts->npoints - 1)
				{
					dlist[(n_out_points + p1_set) * ndims + i] = val1 - quarter_delta;
					p2_set = 1;
				}
			}
			n_out_points+=(p1_set + p2_set);
		}
	}

	/*if last point*/
	if(preserve_endpoint)
	{
		opts->npoints = n_out_points;
		ptarray_append_point(opts, &p1, LW_TRUE);
		n_out_points++;
	}

	if(isclosed && !preserve_endpoint)
	{
		opts->npoints = n_out_points;
		POINT4D first_point = getPoint4d(opts, 0);
		ptarray_append_point(opts, &first_point, LW_TRUE);
		n_out_points++;
	}
	opts->npoints = n_out_points;

	return opts;

}

static LWLINE* lwline_chaikin(const LWLINE *iline, int n_iterations)
{
	POINTARRAY *pa, *pa_new;
	int j;
	LWLINE *oline;

	if( lwline_is_empty(iline))
		return lwline_clone(iline);

	pa = iline->points;
	for (j=0;j<n_iterations;j++)
	{
		pa_new = ptarray_chaikin(pa,1,LW_FALSE);
		if(j>0)
			ptarray_free(pa);
		pa=pa_new;
	}
	oline = lwline_construct(iline->srid, NULL, pa);

	oline->type = iline->type;
	return oline;

}


static LWPOLY* lwpoly_chaikin(const LWPOLY *ipoly, int n_iterations, int preserve_endpoint)
{
	uint32_t i;
	int j;
	POINTARRAY *pa, *pa_new;
	LWPOLY *opoly = lwpoly_construct_empty(ipoly->srid, FLAGS_GET_Z(ipoly->flags), FLAGS_GET_M(ipoly->flags));

	if( lwpoly_is_empty(ipoly) )
		return opoly;
	for (i = 0; i < ipoly->nrings; i++)
	{
		pa = ipoly->rings[i];
		for(j=0;j<n_iterations;j++)
		{
			pa_new = ptarray_chaikin(pa,preserve_endpoint,LW_TRUE);
			if(j>0)
				ptarray_free(pa);
			pa=pa_new;
		}
		if(pa->npoints>=4)
		{
			if( lwpoly_add_ring(opoly,pa ) == LW_FAILURE )
				return NULL;
		}
	}

	opoly->type = ipoly->type;

	if( lwpoly_is_empty(opoly) )
		return NULL;

	return opoly;

}


static LWCOLLECTION* lwcollection_chaikin(const LWCOLLECTION *igeom, int n_iterations, int preserve_endpoint)
{
	LWDEBUG(2, "Entered  lwcollection_set_effective_area");
	uint32_t i;

	LWCOLLECTION *out = lwcollection_construct_empty(igeom->type, igeom->srid, FLAGS_GET_Z(igeom->flags), FLAGS_GET_M(igeom->flags));

	if( lwcollection_is_empty(igeom) )
		return out; /* should we return NULL instead ? */

	for( i = 0; i < igeom->ngeoms; i++ )
	{
		LWGEOM *ngeom = lwgeom_chaikin(igeom->geoms[i],n_iterations,preserve_endpoint);
		if ( ngeom ) out = lwcollection_add_lwgeom(out, ngeom);
	}

	return out;
}


LWGEOM* lwgeom_chaikin(const LWGEOM *igeom, int n_iterations, int preserve_endpoint)
{
	LWDEBUG(2, "Entered  lwgeom_set_effective_area");
	switch (igeom->type)
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		return lwgeom_clone(igeom);
	case LINETYPE:
		return (LWGEOM*)lwline_chaikin((LWLINE*)igeom, n_iterations);
	case POLYGONTYPE:
		return (LWGEOM*)lwpoly_chaikin((LWPOLY*)igeom,n_iterations,preserve_endpoint);
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM*)lwcollection_chaikin((LWCOLLECTION *)igeom,n_iterations,preserve_endpoint);
	default:
		lwerror("lwgeom_chaikin: unsupported geometry type: %s",lwtype_name(igeom->type));
	}
	return NULL;
}
