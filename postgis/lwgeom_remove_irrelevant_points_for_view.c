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
 * Copyright (C) 2024 Sam Peters <gluser1357@gmx.de>
 *
 **********************************************************************/

#include "postgres.h"
#include "funcapi.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "access/htup_details.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"

// ===============================================================================
// Encode location of value related to min and max. Returns
// - 0x1 for value < min
// - 0x2 for min <= value <= max
// - 0x4 for value > max
// ===============================================================================
static int encodeToBits(double value, double min, double max) {
	return value < min ? 0x1 : value <= max ? 0x2 : 0x4;
}

// ===============================================================================
// Helper function for polygon and polyline POINTARRAY's.
// Removes points being irrelevant for rendering the geometry
// within a view specified by rectangular bounds.
// ===============================================================================
static void removePoints(POINTARRAY *points, GBOX *bounds, bool closed) {

	int npoints, minpoints;
	double xmin, ymin, xmax, ymax;

	int i, next, w;
	int vx, vy, vx0, vy0, vx1, vy1, vxall, vyall;
	bool sameX, sameY, insideX, insideY, inside, skip, clear;
	POINT4D p, p0, p1;  // current, previous, next;

	// point number check
	npoints = points->npoints;
	minpoints = closed ? 4 : 2; // min points for each polygon ring or linestring
	if (npoints < minpoints) {
		// clear if not expected minimum number of points
		points->npoints = 0;
		return;
	}

	xmin = bounds->xmin;
	ymin = bounds->ymin;
	xmax = bounds->xmax;
	ymax = bounds->ymax;

	// get previous point [i-1]
	if (closed) {
		getPoint4d_p(points, 0, &p);
		getPoint4d_p(points, npoints - 1, &p0);
		if (p.x != p0.x || p.y != p0.y) return; // requirement for polygons: startpoint equals endpoint. Leave untouched of not met.
		npoints--; // remove double here, and re-add at the end
		getPoint4d_p(points, npoints - 1, &p0);
	}
	else {
		getPoint4d_p(points, 0, &p0);  // for linestrings reuse start point
	}
	vx0 = encodeToBits(p0.x, xmin, xmax);
	vy0 = encodeToBits(p0.y, ymin, ymax);

	// for all points
	w = 0;
	vxall = 0;
	vyall = 0;
	for (i = 0; i < npoints; i++) {

		// get current point [i]
		getPoint4d_p(points, i, &p);
		vx = encodeToBits(p.x, xmin, xmax);
		vy = encodeToBits(p.y, ymin, ymax);

		// get subsequent point [i+1]
		next = i + 1;
		if (next == npoints) {
			if (closed) next = 0; // for polygons, use (new) start point as end point
			else next = i;  // for linestrings reuse last point as end point
		}
		getPoint4d_p(points, next, &p1);
		vx1 = encodeToBits(p1.x, xmin, xmax);
		vy1 = encodeToBits(p1.y, ymin, ymax);

		sameX = vx == vx1 && vx == vx0;
		sameY = vy == vy1 && vy == vy0;
		insideX = vx == 0x02;
		insideY = vy == 0x02;
		inside = insideX && insideY;

		skip = sameX && sameY && !inside;	// three consecutive points in same outside quarter, leave out central one
		skip |= sameX && !insideX;			// three consecutive points in same outside area (left or right), leave out central one
		skip |= sameY && !insideY;			// three consecutive points in same outside area (top or buttom), leave out central one

		if (skip) continue;

		// save current point at [w <= i]
		ptarray_set_point4d(points, w++, &p);
		vx0 = vx;
		vy0 = vy;
		vxall |= vx;
		vyall |= vy;
	}

	if (closed && w > 0) {
		// re-add first new point at the end if closed
		getPoint4d_p(points, 0, &p);
		ptarray_set_point4d(points, w++, &p);
	}

	// eval empty cases
	clear = w < minpoints; 		// too less points
	clear |= vxall == 0x01;		// completely left outside
	clear |= vxall == 0x04;		// completely right outside
	clear |= vyall == 0x01;		// completely top outside
	clear |= vyall == 0x04;		// completely bottom outside
	if (clear) w = 0;

	points->npoints = w;
}

// ===============================================================================
// remove points that are irrelevant for rendering the geometry within
// a view specified by rectangular bounds.
// 2D-(MULTI)POLYGONs and (MULTI)LINESTRINGs are evaluated, others keep untouched.
// ===============================================================================
PG_FUNCTION_INFO_V1(ST_RemoveIrrelevantPointsForView);
Datum ST_RemoveIrrelevantPointsForView(PG_FUNCTION_ARGS) {

	unsigned int i, j, iw, jw;

	// gserialized logic see for example in /postgis/lwgeom_functions_basic.c,
	// type definitions see /liblwgeom/liblwgeom.h(.in)

	GSERIALIZED *serialized_in;
	GSERIALIZED *serialized_out;

	LWGEOM *geom;
	GBOX *bbox;

	// geom input check
	if (PG_GETARG_POINTER(0) == NULL) {
		PG_RETURN_NULL();
	}

	serialized_in = (GSERIALIZED *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	// box input check
	if (PG_GETARG_POINTER(1) == NULL) {
		// no BBOX given, leave untouched
		PG_RETURN_POINTER(serialized_in);
	}

	// type check (only polygon and line types are supported yet)
	if (gserialized_get_type(serialized_in) != POLYGONTYPE &&
		gserialized_get_type(serialized_in) != MULTIPOLYGONTYPE &&
		gserialized_get_type(serialized_in) != LINETYPE &&
		gserialized_get_type(serialized_in) != MULTILINETYPE) {

		// no (multi)polygon or (multi)linetype, leave untouched
		PG_RETURN_POINTER(serialized_in);
	}

	// deserialize geom and copy coordinates (no clone_deep)
	geom = lwgeom_from_gserialized(serialized_in);

	// bbox checks
	bbox = (GBOX*)PG_GETARG_DATUM(1);
	if (!geom->bbox) {
		lwgeom_add_bbox(geom);
	}

	if (!geom->bbox) {
		// no bbox determinable, leave untouched
		lwgeom_free(geom);
		PG_RETURN_POINTER(serialized_in);
	}

	if (bbox->xmin <= geom->bbox->xmin &&
		bbox->ymin <= geom->bbox->ymin &&
		bbox->xmax >= geom->bbox->xmax &&
		bbox->ymax >= geom->bbox->ymax) {

		// trivial case: geometry is fully covered by requested bbox
		lwgeom_free(geom);
		PG_RETURN_POINTER(serialized_in);
	}

	if (geom->type == LINETYPE) {

		LWLINE* line = (LWLINE*)geom;
		removePoints(line->points, bbox, false);
	}

	if (geom->type == MULTILINETYPE) {

		LWMLINE* mline = (LWMLINE*)geom;
		iw = 0;
		for (i=0; i<mline->ngeoms; i++) {
			LWLINE* line = mline->geoms[i];
			removePoints(line->points, bbox, false);

			if (line->points->npoints) {
				// keep (reduced) line
				mline->geoms[iw++] = line;
			}
			else {
				// discard current line
				lwfree(line);
			}
		}
		mline->ngeoms = iw;
	}

	if (geom->type == POLYGONTYPE) {

		LWPOLY* polygon = (LWPOLY*)geom;
		iw = 0;
		for (i=0; i<polygon->nrings; i++) {
			removePoints(polygon->rings[i], bbox, true);

			if (polygon->rings[i]->npoints) {
				// keep (reduced) ring
				polygon->rings[iw++] = polygon->rings[i];
			}
			else {
				if (!i) {
					// exterior ring outside, free and skip all rings
					unsigned int k;
					for (k=0; k<polygon->nrings; k++) {
						lwfree(polygon->rings[k]);
					}
					break;
				}
				else {
					// free and remove current interior ring
					lwfree(polygon->rings[i]);
				}
			}
		}
		polygon->nrings = iw;
	}

	if (geom->type == MULTIPOLYGONTYPE) {

		LWMPOLY* mpolygon = (LWMPOLY*)geom;
		jw = 0;
		for (j=0; j<mpolygon->ngeoms; j++) {

			LWPOLY* polygon = mpolygon->geoms[j];
			iw = 0;
			for (i=0; i<polygon->nrings; i++) {
				removePoints(polygon->rings[i], bbox, true);

				if (polygon->rings[i]->npoints) {
					// keep (reduced) ring
					polygon->rings[iw++] = polygon->rings[i];
				}
				else {
					if (!i) {
						// exterior ring outside, free and skip all rings
						unsigned int k;
						for (k=0; k<polygon->nrings; k++) {
							lwfree(polygon->rings[k]);
						}
						break;
					}
					else {
						// free and remove current interior ring
						lwfree(polygon->rings[i]);
					}
				}
			}
			polygon->nrings = iw;

			if (iw) {
				mpolygon->geoms[jw++] = polygon;
			}
			else {
				// free and remove polygon from multipolygon
				lwfree(polygon);
			}
		}
		mpolygon->ngeoms = jw;
	}

	// recompute bbox if computed previously (may result in NULL)
	lwgeom_drop_bbox(geom);
	lwgeom_add_bbox(geom);

	serialized_out = gserialized_from_lwgeom(geom, 0);
	lwgeom_free(geom);

	PG_FREE_IF_COPY(serialized_in, 0); // see postgis/lwgeom_functions_basic.c, force_2d
	PG_RETURN_POINTER(serialized_out);
}
