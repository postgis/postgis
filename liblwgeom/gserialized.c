#include "liblwgeom_internal.h"
#include "gserialized1.h"
#include "gserialized2.h"

/* First four bits don't change between v0 and v1 */
#define GFLAG_Z         0x01
#define GFLAG_M         0x02
#define GFLAG_BBOX      0x04
#define GFLAG_GEODETIC  0x08
/* v1 and v2 MUST share the same version bits */
#define GFLAG_VER_0     0x40
#define GFLAGS_GET_VERSION(gflags) (((gflags) & GFLAG_VER_0)>>6)

/**
* Read the flags from a #GSERIALIZED and return a standard lwflag
* integer
*/
lwflags_t gserialized_get_lwflags(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_get_lwflags(g);
	else
		return gserialized1_get_lwflags(g);
};

/**
* Copy a new bounding box into an existing gserialized.
* If necessary a new #GSERIALIZED will be allocated. Test
* that input != output before freeing input.
*/
GSERIALIZED *gserialized_set_gbox(GSERIALIZED *g, GBOX *gbox)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_set_gbox(g, gbox);
	else
		return gserialized1_set_gbox(g, gbox);
}

/**
* Return the serialization version
*/
uint32_t gserialized_get_version(const GSERIALIZED *g)
{
	return GFLAGS_GET_VERSION(g->gflags);
}


/**
* Remove the bounding box from a #GSERIALIZED. Returns a freshly
* allocated #GSERIALIZED every time.
*/
GSERIALIZED* gserialized_drop_gbox(GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_drop_gbox(g);
	else
		return gserialized1_drop_gbox(g);
}

/**
* Read the box from the #GSERIALIZED or calculate it if necessary.
* Return #LWFAILURE if box cannot be calculated (NULL or EMPTY
* input).
*/
int gserialized_get_gbox_p(const GSERIALIZED *g, GBOX *gbox)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_get_gbox_p(g, gbox);
	else
		return gserialized1_get_gbox_p(g, gbox);
}

/**
* Read the box from the #GSERIALIZED or return #LWFAILURE if
* box is unavailable.
*/
int gserialized_fast_gbox_p(const GSERIALIZED *g, GBOX *gbox)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_fast_gbox_p(g, gbox);
	else
		return gserialized1_fast_gbox_p(g, gbox);
}

/**
* Extract the geometry type from the serialized form (it hides in
* the anonymous data area, so this is a handy function).
*/
uint32_t gserialized_get_type(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_get_type(g);
	else
		return gserialized1_get_type(g);
}

/**
* Returns the size in bytes to read from toast to get the basic
* information from a geometry: GSERIALIZED struct, bbox and type
*/
uint32_t gserialized_max_header_size(void)
{
	size_t sz1 = gserialized1_max_header_size();
	size_t sz2 = gserialized2_max_header_size();
	return sz1 > sz2 ? sz1 : sz2;
}

/**
* Returns a hash code for the srid/type/geometry information
* in the GSERIALIZED. Ignores metadata like flags and optional
* boxes, etc.
*/
int32_t
gserialized_hash(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_hash(g);
	else
		return gserialized1_hash(g);
}

/**
* Extract the SRID from the serialized form (it is packed into
* three bytes so this is a handy function).
*/
int32_t gserialized_get_srid(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_get_srid(g);
	else
		return gserialized1_get_srid(g);
}

/**
* Write the SRID into the serialized form (it is packed into
* three bytes so this is a handy function).
*/
void gserialized_set_srid(GSERIALIZED *g, int32_t srid)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_set_srid(g, srid);
	else
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
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_is_empty(g);
	else
		return gserialized1_is_empty(g);
}

/**
* Check if a #GSERIALIZED has a bounding box without deserializing first.
*/
int gserialized_has_bbox(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_has_bbox(g);
	else
		return gserialized1_has_bbox(g);
}

/**
* Check if a #GSERIALIZED has a Z ordinate.
*/
int gserialized_has_z(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_has_z(g);
	else
		return gserialized1_has_z(g);
}

/**
* Check if a #GSERIALIZED has an M ordinate.
*/
int gserialized_has_m(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_has_m(g);
	else
		return gserialized1_has_m(g);
}

/**
* Check if a #GSERIALIZED is a geography.
*/
int gserialized_is_geodetic(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_is_geodetic(g);
	else
		return gserialized1_is_geodetic(g);
}

/**
* Return the number of dimensions (2, 3, 4) in a geometry
*/
int gserialized_ndims(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_ndims(g);
	else
		return gserialized1_ndims(g);
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
	return gserialized2_from_lwgeom(geom, size);
}

/**
* Return the memory size a GSERIALIZED will occupy for a given LWGEOM.
*/
size_t gserialized_from_lwgeom_size(const LWGEOM *geom)
{
	return gserialized2_from_lwgeom_size(geom);
}

/**
* Allocate a new #LWGEOM from a #GSERIALIZED. The resulting #LWGEOM will have coordinates
* that are double aligned and suitable for direct reading using getPoint2d_p_ro
*/
LWGEOM* lwgeom_from_gserialized(const GSERIALIZED *g)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return lwgeom_from_gserialized2(g);
	else
		return lwgeom_from_gserialized1(g);
}


const float * gserialized_get_float_box_p(const GSERIALIZED *g, size_t *ndims)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_get_float_box_p(g, ndims);
	else
		return gserialized1_get_float_box_p(g, ndims);
}

int
gserialized_peek_first_point(const GSERIALIZED *g, POINT4D *out_point)
{
	if (GFLAGS_GET_VERSION(g->gflags))
		return gserialized2_peek_first_point(g, out_point);
	else
		return gserialized1_peek_first_point(g, out_point);
}

/**
* Return -1 if g1 is "less than" g2, 1 if g1 is "greater than"
* g2 and 0 if g1 and g2 are the "same". Equality is evaluated
* with a memcmp and size check. So it is possible that two
* identical objects where one lacks a bounding box could be
* evaluated as non-equal initially. Greater and less than
* are evaluated by calculating a sortable key from the center
* point of the object bounds.
* Because this function might have to handle GSERIALIZED
* objects of either version, we implement it up here at the
* switching layer rather than down lower.
*/
#define G2FLAG_EXTENDED 0x10
inline static size_t gserialized_header_size(const GSERIALIZED *g)
{
	size_t sz = 8; /* varsize (4) + srid(3) + flags (1) */

	if ((GFLAGS_GET_VERSION(g->gflags)) &&
	    (G2FLAG_EXTENDED & g->gflags))
		sz += 8;

	if (GFLAG_BBOX & g->gflags)
	{
		if (GFLAG_GEODETIC & g->gflags)
		{
			sz += 6 * sizeof(float);
		}
		else
		{
			sz += 4 * sizeof(float) +
			      ((GFLAG_Z & g->gflags) ? 2*sizeof(float) : 0) +
			      ((GFLAG_M & g->gflags) ? 2*sizeof(float) : 0);
		}
	}

	return sz;
}

inline static int gserialized_cmp_srid(const GSERIALIZED *g1, const GSERIALIZED *g2)
{
	return (
		g1->srid[0] == g2->srid[0] &&
		g1->srid[1] == g2->srid[1] &&
		g1->srid[2] == g2->srid[2]
	) ? 0 : 1;
}

/* ORDER BY hash(g), g::bytea, ST_SRID(g), hasz(g), hasm(g) */
int gserialized_cmp(const GSERIALIZED *g1, const GSERIALIZED *g2)
{
	GBOX box1 = {0}, box2 = {0};
	uint64_t hash1, hash2;
	size_t sz1 = LWSIZE_GET(g1->size);
	size_t sz2 = LWSIZE_GET(g2->size);
	size_t hsz1 = gserialized_header_size(g1);
	size_t hsz2 = gserialized_header_size(g2);
	uint8_t *b1 = (uint8_t*)g1 + hsz1;
	uint8_t *b2 = (uint8_t*)g2 + hsz2;
	size_t bsz1 = sz1 - hsz1;
	size_t bsz2 = sz2 - hsz2;
	size_t bsz_min = bsz1 < bsz2 ? bsz1 : bsz2;

	/* Equality fast path */
	/* Return equality for perfect equality only */
	int cmp_srid = gserialized_cmp_srid(g1, g2);
	int cmp = memcmp(b1, b2, bsz_min);
	int g1hasz = gserialized_has_z(g1);
	int g1hasm = gserialized_has_m(g1);
	int g2hasz = gserialized_has_z(g2);
	int g2hasm = gserialized_has_m(g2);

	if (bsz1 == bsz2 && cmp_srid == 0 && cmp == 0 && g1hasz == g2hasz && g1hasm == g2hasm)
		return 0;
	else
	{
		int g1_is_empty = (gserialized_get_gbox_p(g1, &box1) == LW_FAILURE);
		int g2_is_empty = (gserialized_get_gbox_p(g2, &box2) == LW_FAILURE);
		int32_t srid1 = gserialized_get_srid(g1);
		int32_t srid2 = gserialized_get_srid(g2);

		/* Empty < Non-empty */
		if (g1_is_empty && !g2_is_empty)
			return -1;

		/* Non-empty > Empty */
		if (!g1_is_empty && g2_is_empty)
			return 1;

		if (!g1_is_empty && !g2_is_empty)
		{
			/* Using the boxes, calculate sortable hash key. */
			hash1 = gbox_get_sortable_hash(&box1, srid1);
			hash2 = gbox_get_sortable_hash(&box2, srid2);

			if (hash1 > hash2)
				return 1;
			if (hash1 < hash2)
				return -1;
		}

		/* Prefix comes before longer one. */
		if (bsz1 != bsz2 && cmp == 0)
		{
			if (bsz1 < bsz2)
				return -1;
			else if (bsz1 > bsz2)
				return 1;
		}

		/* If SRID is not equal, sort on it */
		if (cmp_srid != 0)
			return (srid1 > srid2) ? 1 : -1;

		/* ZM flag sort*/
		if (g1hasz != g2hasz)
			return (g1hasz > g2hasz) ? 1 : -1;

		if (g1hasm != g2hasm)
			return (g1hasm > g2hasm) ? 1 : -1;

		assert(cmp != 0);
		return cmp > 0 ? 1 : -1;
	}
}

uint64_t
gserialized_get_sortable_hash(const GSERIALIZED *g)
{
	GBOX box;
	int is_empty = (gserialized_get_gbox_p(g, &box) == LW_FAILURE);

	if (is_empty)
		return 0;
	else
		return gbox_get_sortable_hash(&box, gserialized_get_srid(g));
}

void gserialized_error_if_srid_mismatch(const GSERIALIZED *g1, const GSERIALIZED *g2, const char *funcname);
void
gserialized_error_if_srid_mismatch(const GSERIALIZED *g1, const GSERIALIZED *g2, const char *funcname)
{
	int32_t srid1 = gserialized_get_srid(g1);
	int32_t srid2 = gserialized_get_srid(g2);
	if (srid1 != srid2)
		lwerror("%s: Operation on mixed SRID geometries (%s, %d) != (%s, %d)",
			funcname,
			lwtype_name(gserialized1_get_type(g1)),
			srid1,
			lwtype_name(gserialized_get_type(g2)),
			srid2);
}

void gserialized_error_if_srid_mismatch_reference(const GSERIALIZED *g1, const int32_t srid2, const char *funcname);
void
gserialized_error_if_srid_mismatch_reference(const GSERIALIZED *g1, const int32_t srid2, const char *funcname)
{
	int32_t srid1 = gserialized_get_srid(g1);
	if (srid1 != srid2)
		lwerror("%s: Operation on mixed SRID geometries %s %d != %d",
			funcname,
			lwtype_name(gserialized1_get_type(g1)),
			srid1,
			srid2);
}
