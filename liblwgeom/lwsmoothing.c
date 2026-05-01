
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
 * Chaikin smoothing: Copyright 2018 Nicklas Avén
 * Catmull-Rom smoothing: Copyright 2026 PostGIS contributors
 *
 **********************************************************************/


#include "liblwgeom_internal.h"


/* ========================================================================
 * Chaikin smoothing
 * ======================================================================== */

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


/* Forward declaration needed by lwcollection_catmull_rom */
LWGEOM *lwgeom_catmull_rom(const LWGEOM *igeom, int n_segments);

/* ========================================================================
 * Catmull-Rom spline smoothing
 *
 * An interpolating spline: the output curve passes through every original
 * vertex. Between each pair of original vertices, (n_segments - 1) new
 * intermediate points are inserted.
 *
 * Standard uniform Catmull-Rom formula for t in [0,1] between P1 and P2:
 *   q(t) = 0.5 * [ 2*P1
 *                  + (-P0 + P2)*t
 *                  + (2*P0 - 5*P1 + 4*P2 - P3)*t^2
 *                  + (-P0 + 3*P1 - 3*P2 + P3)*t^3 ]
 *
 * Applied per dimension (x, y, and optionally z and m).
 *
 * Boundary handling for open lines: phantom endpoints are constructed by
 * reflecting the adjacent point through the endpoint.
 *   P_{-1} = 2*P0 - P1
 *   P_N    = 2*P_{N-1} - P_{N-2}
 *
 * Boundary handling for closed rings: wrap-around indexing.
 * ======================================================================== */

/*
 * Core Catmull-Rom algorithm on a POINTARRAY.
 *
 * isclosed: LW_TRUE for polygon rings (first == last point).
 * n_segments: number of sub-segments per original span (>= 2).
 *
 * Returns a newly allocated POINTARRAY; caller must free.
 */
static POINTARRAY *
ptarray_catmull_rom(const POINTARRAY *inpts, int n_segments, int isclosed)
{
	uint32_t ndims = 2 + ptarray_has_z(inpts) + ptarray_has_m(inpts);
	uint32_t npoints = inpts->npoints;

	/* For closed rings the last point duplicates the first; work with
	 * N = npoints-1 unique points so we don't double-count. */
	uint32_t N = isclosed ? npoints - 1 : npoints;

	/* Too few unique points: return a clone unchanged */
	if (N < 4)
		return ptarray_clone(inpts);

	/* Number of spans between consecutive original vertices */
	uint32_t nspans = N - (isclosed ? 0 : 1);

	/* Output size: nspans * n_segments + 1 (we always close with the last orig point) */
	uint32_t out_cap = nspans * (uint32_t)n_segments + 1;
	POINTARRAY *opts = ptarray_construct_empty(
		FLAGS_GET_Z(inpts->flags), FLAGS_GET_M(inpts->flags), out_cap);

	/* Helper to get a point by index with phantom-endpoint / wrap-around logic.
	 * idx may be -1 or >= N; we handle accordingly. */
#define GET_COORD(idx, dim)                                           \
	(((int32_t)(idx) < 0)                                             \
		? (isclosed                                                   \
			? ((double *)getPoint_internal(inpts,                     \
			       (uint32_t)((int32_t)N + (int32_t)(idx))))[dim]    \
			: 2.0 * ((double *)getPoint_internal(inpts, 0))[dim]     \
			  - ((double *)getPoint_internal(inpts, 1))[dim])         \
		: ((uint32_t)(idx) >= N)                                      \
			? (isclosed                                               \
				? ((double *)getPoint_internal(inpts,                 \
				       (uint32_t)(idx) % N))[dim]                    \
				: 2.0 * ((double *)getPoint_internal(inpts, N-1))[dim]\
				  - ((double *)getPoint_internal(inpts, N-2))[dim])   \
			: ((double *)getPoint_internal(inpts, (uint32_t)(idx)))[dim])

	int first_point = 1; /* track whether we should include t=0 */

	for (uint32_t span = 0; span < nspans; span++)
	{
		int32_t i0 = (int32_t)span - 1;  /* P0 (leading control) */
		int32_t i1 = (int32_t)span;      /* P1 (start of span)   */
		int32_t i2 = (int32_t)span + 1;  /* P2 (end of span)     */
		int32_t i3 = (int32_t)span + 2;  /* P3 (trailing control)*/

		int k_start = first_point ? 0 : 1;
		first_point = 0;

		/* Run k from k_start to n_segments inclusive.
		 * At k=0: t=0.0 (original vertex, only for first span)
		 * At k=n_segments: t=1.0 (next original vertex, closes the span) */
		for (int k = k_start; k <= n_segments; k++)
		{
			double t  = k / (double)n_segments;
			double t2 = t * t;
			double t3 = t2 * t;
			POINT4D pt;

			for (uint32_t d = 0; d < ndims; d++)
			{
				double c0 = GET_COORD(i0, d);
				double c1 = GET_COORD(i1, d);
				double c2 = GET_COORD(i2, d);
				double c3 = GET_COORD(i3, d);

				((double *)&pt)[d] = 0.5 * (
					 2.0 * c1
					+ (-c0 + c2) * t
					+ (2.0*c0 - 5.0*c1 + 4.0*c2 - c3) * t2
					+ (-c0 + 3.0*c1 - 3.0*c2 + c3) * t3
				);
			}
			/* Zero out unused dimensions so POINT4D is clean */
			if (ndims < 3) pt.z = 0.0;
			if (ndims < 4) pt.m = 0.0;

			ptarray_append_point(opts, &pt, LW_TRUE);
		}
	}
	/* At k=n_segments (t=1.0), the Catmull-Rom formula evaluates to c2 (the span
	 * endpoint), so all original vertices are naturally included in the output,
	 * and closed rings are automatically closed by the last span's t=1 giving P0. */

#undef GET_COORD

	return opts;
}


static LWLINE *
lwline_catmull_rom(const LWLINE *iline, int n_segments)
{
	if (lwline_is_empty(iline))
		return lwline_clone(iline);

	if (iline->points->npoints < 4)
		return lwline_clone(iline);

	POINTARRAY *pa = ptarray_catmull_rom(iline->points, n_segments, LW_FALSE);
	LWLINE *oline = lwline_construct(iline->srid, NULL, pa);
	oline->type = iline->type;
	return oline;
}


static LWPOLY *
lwpoly_catmull_rom(const LWPOLY *ipoly, int n_segments)
{
	LWPOLY *opoly = lwpoly_construct_empty(
		ipoly->srid, FLAGS_GET_Z(ipoly->flags), FLAGS_GET_M(ipoly->flags));

	if (lwpoly_is_empty(ipoly))
		return opoly;

	for (uint32_t i = 0; i < ipoly->nrings; i++)
	{
		POINTARRAY *ring = ipoly->rings[i];
		POINTARRAY *pa_out;

		/* npoints-1 unique points; need at least 4 to smooth */
		if (ring->npoints - 1 < 4)
			pa_out = ptarray_clone(ring);
		else
			pa_out = ptarray_catmull_rom(ring, n_segments, LW_TRUE);

		if (pa_out->npoints >= 4)
		{
			if (lwpoly_add_ring(opoly, pa_out) == LW_FAILURE)
				return NULL;
		}
		else
		{
			ptarray_free(pa_out);
		}
	}

	opoly->type = ipoly->type;

	if (lwpoly_is_empty(opoly))
		return NULL;

	return opoly;
}


static LWCOLLECTION *
lwcollection_catmull_rom(const LWCOLLECTION *igeom, int n_segments)
{
	LWCOLLECTION *out = lwcollection_construct_empty(
		igeom->type, igeom->srid,
		FLAGS_GET_Z(igeom->flags), FLAGS_GET_M(igeom->flags));

	if (lwcollection_is_empty(igeom))
		return out;

	for (uint32_t i = 0; i < igeom->ngeoms; i++)
	{
		LWGEOM *ngeom = lwgeom_catmull_rom(igeom->geoms[i], n_segments);
		if (ngeom)
			out = lwcollection_add_lwgeom(out, ngeom);
	}

	return out;
}


LWGEOM *
lwgeom_catmull_rom(const LWGEOM *igeom, int n_segments)
{
	switch (igeom->type)
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		return lwgeom_clone(igeom);
	case LINETYPE:
		return (LWGEOM *)lwline_catmull_rom((LWLINE *)igeom, n_segments);
	case POLYGONTYPE:
		return (LWGEOM *)lwpoly_catmull_rom((LWPOLY *)igeom, n_segments);
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM *)lwcollection_catmull_rom((LWCOLLECTION *)igeom, n_segments);
	default:
		lwerror("lwgeom_catmull_rom: unsupported geometry type: %s", lwtype_name(igeom->type));
	}
	return NULL;
}
