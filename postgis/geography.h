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
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/



/**********************************************************************
**  Useful functions for all GSERIALIZED handlers.
**  TODO: Move to common.h in pgcommon
*/

/* Check that the typmod matches the flags on the lwgeom */
GSERIALIZED* postgis_valid_typmod(GSERIALIZED *gser, int32_t typmod);
/* Check that the type is legal in geography (no curves please!) */
void geography_valid_type(uint8_t type);

/* Expand the embedded bounding box in a #GSERIALIZED */
GSERIALIZED* gserialized_expand(GSERIALIZED *g, double distance);

