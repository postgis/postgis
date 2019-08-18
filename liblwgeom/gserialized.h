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
 * Copyright 2019 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/

/*
* GSERIALIZED PUBLIC API
*/

/**
* Read the flags from a #GSERIALIZED and return a standard lwflag
* integer
*/
lwflags_t gserialized_get_lwflags(const GSERIALIZED *g);

/**
* Copy a new bounding box into an existing gserialized.
* If necessary a new #GSERIALIZED will be allocated. Test
* that input != output before freeing input.
*/
GSERIALIZED *gserialized_set_gbox(GSERIALIZED *g, GBOX *gbox);

/**
* Remove the bounding box from a #GSERIALIZED. Returns a freshly
* allocated #GSERIALIZED every time.
*/
GSERIALIZED *gserialized_drop_gbox(GSERIALIZED *g);

/**
* Read the box from the #GSERIALIZED or calculate it if necessary.
* Return #LWFAILURE if box cannot be calculated (NULL or EMPTY
* input).
*/
int gserialized_get_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/**
* Read the box from the #GSERIALIZED or return #LWFAILURE if
* box is unavailable.
*/
int gserialized_fast_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/**
* Extract the geometry type from the serialized form (it hides in
* the anonymous data area, so this is a handy function).
*/
extern uint32_t gserialized_get_type(const GSERIALIZED *g);

/**
* Returns the size in bytes to read from toast to get the basic
* information from a geometry: GSERIALIZED struct, bbox and type
*/
extern uint32_t gserialized_max_header_size(void);

/**
* Returns a hash code for the srid/type/geometry information
* in the GSERIALIZED. Ignores metadata like flags and optional
* boxes, etc.
*/
extern int32_t gserialized_hash(const GSERIALIZED *g);

/**
* Extract the SRID from the serialized form (it is packed into
* three bytes so this is a handy function).
*/
extern int32_t gserialized_get_srid(const GSERIALIZED *g);

/**
* Write the SRID into the serialized form (it is packed into
* three bytes so this is a handy function).
*/
extern void gserialized_set_srid(GSERIALIZED *g, int32_t srid);

/**
* Check if a #GSERIALIZED is empty without deserializing first.
* Only checks if the number of elements of the parent geometry
* is zero, will not catch collections of empty, eg:
* GEOMETRYCOLLECTION(POINT EMPTY)
*/
extern int gserialized_is_empty(const GSERIALIZED *g);

/**
* Check if a #GSERIALIZED has a bounding box without deserializing first.
*/
extern int gserialized_has_bbox(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED has a Z ordinate.
*/
extern int gserialized_has_z(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED has an M ordinate.
*/
extern int gserialized_has_m(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED is a geography.
*/
extern int gserialized_is_geodetic(const GSERIALIZED *gser);

/**
* Return the number of dimensions (2, 3, 4) in a geometry
*/
extern int gserialized_ndims(const GSERIALIZED *gser);

/**
* Return -1 if g1 is "less than" g2, 1 if g1 is "greater than"
* g2 and 0 if g1 and g2 are the "same". Equality is evaluated
* with a memcmp and size check. So it is possible that two
* identical objects where one lacks a bounding box could be
* evaluated as non-equal initially. Greater and less than
* are evaluated by calculating a sortable key from the center
* point of the object bounds.
*/
extern int gserialized_cmp(const GSERIALIZED *g1, const GSERIALIZED *g2);

/**
* Allocate a new #GSERIALIZED from an #LWGEOM. For all non-point types, a bounding
* box will be calculated and embedded in the serialization. The geodetic flag is used
* to control the box calculation (cartesian or geocentric). If set, the size pointer
* will contain the size of the final output, which is useful for setting the PgSQL
* VARSIZE information.
*/
GSERIALIZED *gserialized_from_lwgeom(LWGEOM *geom, size_t *size);

/**
* Return the memory size a GSERIALIZED will occupy for a given LWGEOM.
*/
size_t gserialized_from_lwgeom_size(const LWGEOM *geom);

/**
* Allocate a new #LWGEOM from a #GSERIALIZED. The resulting #LWGEOM will have coordinates
* that are double aligned and suitable for direct reading using getPoint2d_p_ro
*/
LWGEOM *lwgeom_from_gserialized(const GSERIALIZED *g);

/**
* Pull a #GBOX from the header of a #GSERIALIZED, if one is available. If
* it is not, calculate it from the geometry. If that doesn't work (null
* or empty) return LW_FAILURE.
*/
int gserialized_get_gbox_p(const GSERIALIZED *g, GBOX *box);

/**
* Pull a #GBOX from the header of a #GSERIALIZED, if one is available. If
* it is not, return LW_FAILURE.
*/
int gserialized_fast_gbox_p(const GSERIALIZED *g, GBOX *box);

/**
 * Pull the first point values of a #GSERIALIZED. Only works for POINTTYPE
 */
int gserialized_peek_first_point(const GSERIALIZED *g, POINT4D *out_point);
