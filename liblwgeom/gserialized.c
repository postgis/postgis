#include "liblwgeom_internal.h"
#include "gserialized1.h"

/**
* Read the flags from a #GSERIALIZED and return a standard lwflag
* integer
*/
lwflags_t gserialized_get_lwflags(const GSERIALIZED *g)
{
	return gserialized1_get_lwflags(g);
};

/**
* Copy a new bounding box into an existing gserialized.
* If necessary a new #GSERIALIZED will be allocated. Test
* that input != output before freeing input.
*/
GSERIALIZED *gserialized_set_gbox(GSERIALIZED *g, GBOX *gbox)
{
	return gserialized1_set_gbox(g, gbox);
}


/**
* Remove the bounding box from a #GSERIALIZED. Returns a freshly
* allocated #GSERIALIZED every time.
*/
GSERIALIZED* gserialized_drop_gbox(GSERIALIZED *g)
{
	return gserialized1_drop_gbox(g);
}

/**
* Read the box from the #GSERIALIZED or calculate it if necessary.
* Return #LWFAILURE if box cannot be calculated (NULL or EMPTY
* input).
*/
int gserialized_get_gbox_p(const GSERIALIZED *g, GBOX *gbox)
{
	return gserialized1_get_gbox_p(g, gbox);
}

/**
* Read the box from the #GSERIALIZED or return #LWFAILURE if
* box is unavailable.
*/
int gserialized_fast_gbox_p(const GSERIALIZED *g, GBOX *gbox)
{
	return gserialized1_fast_gbox_p(g, gbox);
}

/**
* Extract the geometry type from the serialized form (it hides in
* the anonymous data area, so this is a handy function).
*/
uint32_t gserialized_get_type(const GSERIALIZED *g)
{
	return gserialized1_get_type(g);
}

/**
* Returns the size in bytes to read from toast to get the basic
* information from a geometry: GSERIALIZED struct, bbox and type
*/
uint32_t gserialized_max_header_size(void)
{
	return gserialized1_max_header_size();
}

/**
* Returns a hash code for the srid/type/geometry information
* in the GSERIALIZED. Ignores metadata like flags and optional
* boxes, etc.
*/
uint64_t gserialized_hash(const GSERIALIZED *g)
{
	return gserialized1_hash(g);
}

/**
* Extract the SRID from the serialized form (it is packed into
* three bytes so this is a handy function).
*/
int32_t gserialized_get_srid(const GSERIALIZED *g)
{
	return gserialized1_get_srid(g);
}

/**
* Write the SRID into the serialized form (it is packed into
* three bytes so this is a handy function).
*/
void gserialized_set_srid(GSERIALIZED *g, int32_t srid)
{
	return gserialized1_set_srid(g, srid);
}

/**
* Check if a #GSERIALIZED is empty without deserializing first.
* Only checks if the number of elements of the parent geometry
* is zero, will not catch collections of empty, eg:
* GEOMETRYCOLLECTION(POINT EMPTY)
*/
int gserialized_is_empty(const GSERIALIZED *g)
{
	return gserialized1_is_empty(g);
}

/**
* Check if a #GSERIALIZED has a bounding box without deserializing first.
*/
int gserialized_has_bbox(const GSERIALIZED *g)
{
	return gserialized1_has_bbox(g);
}

/**
* Check if a #GSERIALIZED has a Z ordinate.
*/
int gserialized_has_z(const GSERIALIZED *g)
{
	return gserialized1_has_z(g);
}

/**
* Check if a #GSERIALIZED has an M ordinate.
*/
int gserialized_has_m(const GSERIALIZED *g)
{
	return gserialized1_has_m(g);
}

/**
* Check if a #GSERIALIZED is a geography.
*/
int gserialized_is_geodetic(const GSERIALIZED *g)
{
	return gserialized1_is_geodetic(g);
}

/**
* Return the number of dimensions (2, 3, 4) in a geometry
*/
int gserialized_ndims(const GSERIALIZED *g)
{
	return gserialized1_ndims(g);
}

/**
* Return -1 if g1 is "less than" g2, 1 if g1 is "greater than"
* g2 and 0 if g1 and g2 are the "same". Equality is evaluated
* with a memcmp and size check. So it is possible that two
* identical objects where one lacks a bounding box could be
* evaluated as non-equal initially. Greater and less than
* are evaluated by calculating a sortable key from the center
* point of the object bounds.
*/
int gserialized_cmp(const GSERIALIZED *g1, const GSERIALIZED *g2)
{
	return gserialized1_cmp(g1, g2);
}

/**
* Allocate a new #GSERIALIZED from an #LWGEOM. For all non-point types, a bounding
* box will be calculated and embedded in the serialization. The geodetic flag is used
* to control the box calculation (cartesian or geocentric). If set, the size pointer
* will contain the size of the final output, which is useful for setting the PgSQL
* VARSIZE information.
*/
GSERIALIZED* gserialized_from_lwgeom(LWGEOM *geom, size_t *size)
{
	return gserialized1_from_lwgeom(geom, size);
}

/**
* Return the memory size a GSERIALIZED will occupy for a given LWGEOM.
*/
size_t gserialized_from_lwgeom_size(const LWGEOM *geom)
{
	return gserialized1_from_lwgeom_size(geom);
}

/**
* Allocate a new #LWGEOM from a #GSERIALIZED. The resulting #LWGEOM will have coordinates
* that are double aligned and suitable for direct reading using getPoint2d_p_ro
*/
LWGEOM* lwgeom_from_gserialized(const GSERIALIZED *g)
{
	return lwgeom_from_gserialized1(g);
}


const float * gserialized_get_float_box_p(const GSERIALIZED *g, size_t *ndims)
{
	return gserialized1_get_float_box_p(g, ndims);
}
