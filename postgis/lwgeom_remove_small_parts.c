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
// helper function for polygon and polyline POINTARRAY's
// which removes small parts given by dx and dy.
// ===============================================================================
static void ptarray_remove_dim_helper(POINTARRAY *points, double mindx, double mindy) {

	int r;
	double xmin = 0, ymin = 0, xmax = 0, ymax = 0;
	double x, y;
	POINT4D point;

	int npoints = points->npoints;
	for (r=0; r < npoints; r++) {

		getPoint4d_p(points, r, &point);

		x = point.x;
		y = point.y;

		if (mindx > 0) {
			if (!r || xmin > x) xmin = x;
			if (!r || xmax < x) xmax = x;
		}
		if (mindy > 0) {
			if (!r || ymin > y) ymin = y;
			if (!r || ymax < y) ymax = y;
		}
	}

	if ((mindx > 0 && xmax - xmin < mindx) ||
		(mindy > 0 && ymax - ymin < mindy)) {
		// skip part
		points->npoints = 0;
	}
}

// ===============================================================================
// remove small (sub-)geometries being smaller than given dimensions.
// 2D-(MULTI)POLYGONs and (MULTI)LINESTRINGs are evaluated, others keep untouched.
// ===============================================================================
PG_FUNCTION_INFO_V1(ST_RemoveSmallParts);
Datum ST_RemoveSmallParts(PG_FUNCTION_ARGS) {

	double mindx = 0, mindy = 0;
	unsigned int i, j, iw, jw;

	GSERIALIZED *serialized_in;
	GSERIALIZED *serialized_out;

	LWGEOM *geom;

	// geom input check
	if (PG_GETARG_POINTER(0) == NULL) {
		PG_RETURN_NULL();
	}

	serialized_in = (GSERIALIZED *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	if (PG_NARGS() == 3) {

		if (PG_ARGISNULL(1) || PG_ARGISNULL(2)) {
			// no valid args given, leave untouched
			PG_RETURN_POINTER(serialized_in);
		}

		mindx = PG_GETARG_FLOAT8(1);
		mindy = PG_GETARG_FLOAT8(2);
		if (mindx <= 0 && mindy <= 0) {
			// nothing to do
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

	}

	else {
		// unknown params, leave untouched
		PG_RETURN_POINTER(serialized_in);
	}

	// deserialize geom and copy coordinates (no clone_deep)
	geom = lwgeom_from_gserialized(serialized_in);

	if (geom->type == LINETYPE) {

		LWLINE* line = (LWLINE*)geom;
		ptarray_remove_dim_helper(line->points, mindx, mindy);
	}

	if (geom->type == MULTILINETYPE) {

		LWMLINE* mline = (LWMLINE*)geom;
		iw = 0;
		for (i=0; i<mline->ngeoms; i++) {
			LWLINE* line = mline->geoms[i];
			ptarray_remove_dim_helper(line->points, mindx, mindy);

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
			ptarray_remove_dim_helper(polygon->rings[i], mindx, mindy);

			if (polygon->rings[i]->npoints) {
				// keep (reduced) ring
				polygon->rings[iw++] = polygon->rings[i];
			}
			else {
				if (!i) {
					// exterior ring too small, free and skip all rings
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
				ptarray_remove_dim_helper(polygon->rings[i], mindx, mindy);

				if (polygon->rings[i]->npoints) {
					// keep (reduced) ring
					polygon->rings[iw++] = polygon->rings[i];
				}
				else {
					if (!i) {
						// exterior ring too small, free and skip all rings
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

	PG_FREE_IF_COPY(serialized_in, 0);
	PG_RETURN_POINTER(serialized_out);
}
