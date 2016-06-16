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
 * Copyright 2016 Sandro Santilli <strk@kbt.net>
 *
 **********************************************************************/

#include "../postgis_config.h"
#define POSTGIS_DEBUG_LEVEL 4
#include "lwgeom_geos.h"
#include "liblwgeom_internal.h"

#include <string.h>
#include <assert.h>

LWGEOM* lwgeom_wrapx(const LWGEOM* lwgeom_in, double cutx, double amount);
static LWCOLLECTION* lwcollection_wrapx(const LWCOLLECTION* lwcoll_in, double cutx, double amount);

static LWGEOM*
lwgeom_split_wrapx(const LWGEOM* geom_in, double cutx, double amount)
{
	LWGEOM *blade, *split;
	POINTARRAY *bladepa;
	POINT4D pt;
	const GBOX *box_in;
	AFFINE affine = {
		1, 0, 0,
		0, 1, 0,
		0, 0, 1,
		amount, 0, 0,
	};

	/* Extract box */
	/* TODO: check if the bbox should be force-recomputed */
	box_in = lwgeom_get_bbox(geom_in);
	if ( ! box_in ) {
		/* must be empty */
		return lwgeom_clone(geom_in);
	}

	LWDEBUGF(2, "BOX X range is %g..%g, cutx:%g, amount:%g", box_in->xmin, box_in->xmax, cutx, amount);

	/* Check if geometry is fully on the side needing shift */
	if ( ( amount < 0 && box_in->xmin >= cutx ) || ( amount > 0 && box_in->xmax <= cutx ) )
	{
		split = lwgeom_clone_deep(geom_in);
		LWDEBUG(2, "returning the translated geometry");
		lwgeom_affine(split, &affine);
		return split;
	}

//DEBUG2: [lwgeom_wrapx.c:lwgeom_split_wrapx:58] BOX X range is 8..10, cutx:8, amount:-10

	/* Check if geometry is fully on the side needing no shift */
	if ( ( amount < 0 && box_in->xmax <= cutx ) || ( amount > 0 && box_in->xmin >= cutx ) )
	{
		LWDEBUG(2, "returning the cloned geometry");
		return lwgeom_clone_deep(geom_in);
	}

	/* We need splitting here */

	/* construct blade */
	bladepa = ptarray_construct(0, 0, 2);
	pt.x = cutx;
	pt.y = box_in->ymin - 1;
	ptarray_set_point4d(bladepa, 0, &pt);
	pt.y = box_in->ymax + 1;
	ptarray_set_point4d(bladepa, 1, &pt);
	blade = lwline_as_lwgeom(lwline_construct(geom_in->srid, NULL, bladepa));

	LWDEBUG(2, "splitting the geometry");

	/* split by blade */
	split = lwgeom_split(geom_in, blade);
	lwgeom_free(blade);

	/* iterate over components, translate if needed */
	const LWCOLLECTION *col = lwgeom_as_lwcollection(split);
	if ( ! col ) {
		/* not split, this is unexpected */
		lwnotice("WARNING: unexpected lack of split in lwgeom_split_wrapx");
		return lwgeom_clone(geom_in);
	}
	LWCOLLECTION *col_out = lwcollection_wrapx(col, cutx, amount);
	lwgeom_free(split);

	/* unary-union the result (homogenize too ?) */
	LWGEOM* out = lwgeom_unaryunion(lwcollection_as_lwgeom(col_out));
	lwcollection_free(col_out);

	return out;
}

static LWCOLLECTION*
lwcollection_wrapx(const LWCOLLECTION* lwcoll_in, double cutx, double amount)
{
	LWGEOM** wrap_geoms=NULL;
	LWCOLLECTION* out;
	size_t i;

	wrap_geoms = lwalloc(lwcoll_in->ngeoms * sizeof(LWGEOM*));
	if ( ! wrap_geoms )
	{
		lwerror("Out of virtual memory");
		return NULL;
	}

	for (i=0; i<lwcoll_in->ngeoms; ++i)
	{
		wrap_geoms[i] = lwgeom_wrapx(lwcoll_in->geoms[i], cutx, amount);
		/* an exception should prevent this from ever returning NULL */
		if ( ! wrap_geoms[i] ) {
			while (--i>=0) lwgeom_free(wrap_geoms[i]);
			lwfree(wrap_geoms);
			return NULL;
		}
	}

	/* Now wrap_geoms has wrap_geoms_size geometries */
	out = lwcollection_construct(lwcoll_in->type, lwcoll_in->srid,
	                             NULL, lwcoll_in->ngeoms, wrap_geoms);

	return out;
}

/* exported */
LWGEOM*
lwgeom_wrapx(const LWGEOM* lwgeom_in, double cutx, double amount)
{
	/* Nothing to wrap in an empty geom */
	if ( lwgeom_is_empty(lwgeom_in) ) return lwgeom_clone(lwgeom_in);

	/* Nothing to wrap if shift amount is zero */
	if ( amount == 0 ) return lwgeom_clone(lwgeom_in);

	switch (lwgeom_in->type)
	{
	case LINETYPE:
	case POLYGONTYPE:
		return lwgeom_split_wrapx(lwgeom_in, cutx, amount);

	case POINTTYPE:
	{
		const LWPOINT *pt = lwgeom_as_lwpoint(lwgeom_clone_deep(lwgeom_in));
		POINT4D pt4d;
		getPoint4d_p(pt->point, 0, &pt4d);

		LWDEBUGF(2, "POINT X is %g, cutx:%g, amount:%g", pt4d.x, cutx, amount);

		if ( ( amount < 0 && pt4d.x > cutx ) || ( amount > 0 && pt4d.x < cutx ) )
		{
			pt4d.x += amount;
			ptarray_set_point4d(pt->point, 0, &pt4d);
		}
		return lwpoint_as_lwgeom(pt);
	}

	case MULTIPOINTTYPE:
	case MULTIPOLYGONTYPE:
	case MULTILINETYPE:
	case COLLECTIONTYPE:
		return lwcollection_as_lwgeom(
						lwcollection_wrapx((const LWCOLLECTION*)lwgeom_in, cutx, amount)
					 );

	default:
		lwerror("Wrapping of %s geometries is unsupported",
		        lwtype_name(lwgeom_in->type));
		return NULL;
	}

}
