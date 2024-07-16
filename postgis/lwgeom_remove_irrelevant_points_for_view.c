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
#include "lwgeom_remove_irrelevant_points_for_view_helpers.h"

// ===============================================================================
// remove points that are irrelevant for rendering the geometry within
// a view specified by rectangular bounds.
// 2D-(MULTI)POLYGONs and (MULTI)LINESTRINGs are evaluated, others keep untouched.
// ===============================================================================
PG_FUNCTION_INFO_V1(ST_RemoveIrrelevantPointsForView);
Datum ST_RemoveIrrelevantPointsForView(PG_FUNCTION_ARGS) {

	unsigned int i, j, iw, jw;
	bool cartesian_hint;

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

	// get (optional) cartesian_hint flag for advanced optimizations
	// that assume the the coordinates can be seen as coordinates
	// to be rendered in a cartesian coordinate system.
	if (PG_NARGS() > 2 && (!PG_ARGISNULL(2))) {
		cartesian_hint = PG_GETARG_BOOL(2);
	}
	else {
		cartesian_hint = false;
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
		removePoints(line->points, bbox, false, cartesian_hint);
	}

	if (geom->type == MULTILINETYPE) {

		LWMLINE* mline = (LWMLINE*)geom;
		iw = 0;
		for (i=0; i<mline->ngeoms; i++) {
			LWLINE* line = mline->geoms[i];
			removePoints(line->points, bbox, false, cartesian_hint);

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
			removePoints(polygon->rings[i], bbox, true, cartesian_hint);

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
				removePoints(polygon->rings[i], bbox, true, cartesian_hint);

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
