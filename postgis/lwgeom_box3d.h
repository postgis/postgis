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
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright 2009-2017 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2018 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

/* forward defs */
Datum BOX3D_in(PG_FUNCTION_ARGS);
Datum BOX3D_out(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS);
Datum BOX3D_to_LWGEOM(PG_FUNCTION_ARGS);
Datum BOX3D_expand(PG_FUNCTION_ARGS);
Datum BOX3D_to_BOX2D(PG_FUNCTION_ARGS);
Datum BOX3D_to_BOX(PG_FUNCTION_ARGS);
Datum BOX3D_xmin(PG_FUNCTION_ARGS);
Datum BOX3D_ymin(PG_FUNCTION_ARGS);
Datum BOX3D_zmin(PG_FUNCTION_ARGS);
Datum BOX3D_xmax(PG_FUNCTION_ARGS);
Datum BOX3D_ymax(PG_FUNCTION_ARGS);
Datum BOX3D_zmax(PG_FUNCTION_ARGS);
Datum BOX3D_combine(PG_FUNCTION_ARGS);
Datum BOX3D_combine_BOX3D(PG_FUNCTION_ARGS);

/*****************************************************************************
 * BOX3D operators
 *****************************************************************************/

bool BOX3D_contains_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_contained_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overlaps_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_same_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_left_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overleft_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_right_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overright_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_below_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overbelow_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_above_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overabove_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_front_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overfront_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_back_internal(BOX3D *box1, BOX3D *box2);
bool BOX3D_overback_internal(BOX3D *box1, BOX3D *box2);
double BOX3D_distance_internal(BOX3D *box1, BOX3D *box2);

/** needed for sp-gist support PostgreSQL 11+ **/
#if POSTGIS_PGSQL_VERSION > 100
Datum BOX3D_contains(PG_FUNCTION_ARGS);
Datum BOX3D_contained(PG_FUNCTION_ARGS);
Datum BOX3D_overlaps(PG_FUNCTION_ARGS);
Datum BOX3D_same(PG_FUNCTION_ARGS);
Datum BOX3D_left(PG_FUNCTION_ARGS);
Datum BOX3D_overleft(PG_FUNCTION_ARGS);
Datum BOX3D_right(PG_FUNCTION_ARGS);
Datum BOX3D_overright(PG_FUNCTION_ARGS);
Datum BOX3D_below(PG_FUNCTION_ARGS);
Datum BOX3D_overbelow(PG_FUNCTION_ARGS);
Datum BOX3D_above(PG_FUNCTION_ARGS);
Datum BOX3D_overabove(PG_FUNCTION_ARGS);
Datum BOX3D_front(PG_FUNCTION_ARGS);
Datum BOX3D_overfront(PG_FUNCTION_ARGS);
Datum BOX3D_back(PG_FUNCTION_ARGS);
Datum BOX3D_overback(PG_FUNCTION_ARGS);
Datum BOX3D_distance(PG_FUNCTION_ARGS);
#endif
