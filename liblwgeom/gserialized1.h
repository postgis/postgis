/**
* Macros for manipulating the 'flags' byte. A uint8_t used as follows:
* VVSRGBMZ
* Version bit, followed by
* Validty, Solid, ReadOnly, Geodetic, HasBBox, HasM and HasZ flags.
*/
#define G1FLAG_Z        0x01
#define G1FLAG_M        0x02
#define G1FLAG_BBOX     0x04
#define G1FLAG_GEODETIC 0x08
#define G1FLAG_READONLY 0x10
#define G1FLAG_SOLID    0x20
/* VERSION BITS         0x40 */
/* VERSION BITS         0x80 */

#define G1FLAGS_GET_Z(gflags)         ((gflags) & G1FLAG_Z)
#define G1FLAGS_GET_M(gflags)        (((gflags) & G1FLAG_M)>>1)
#define G1FLAGS_GET_BBOX(gflags)     (((gflags) & G1FLAG_BBOX)>>2)
#define G1FLAGS_GET_GEODETIC(gflags) (((gflags) & G1FLAG_GEODETIC)>>3)
#define G1FLAGS_GET_SOLID(gflags)    (((gflags) & G1FLAG_SOLID)>>5)


#define G1FLAGS_SET_Z(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_Z) : ((gflags) & ~G1FLAG_Z))
#define G1FLAGS_SET_M(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_M) : ((gflags) & ~G1FLAG_M))
#define G1FLAGS_SET_BBOX(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_BBOX) : ((gflags) & ~G1FLAG_BBOX))
#define G1FLAGS_SET_GEODETIC(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_GEODETIC) : ((gflags) & ~G1FLAG_GEODETIC))
#define G1FLAGS_SET_SOLID(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_SOLID) : ((gflags) & ~G1FLAG_SOLID))

#define G1FLAGS_NDIMS(gflags) (2 + G1FLAGS_GET_Z(gflags) + G1FLAGS_GET_M(gflags))
#define G1FLAGS_GET_ZM(gflags) (G1FLAGS_GET_M(gflags) + G1FLAGS_GET_Z(gflags) * 2)
#define G1FLAGS_NDIMS_BOX(gflags) (G1FLAGS_GET_GEODETIC(gflags) ? 3 : G1FLAGS_NDIMS(gflags))

uint8_t g1flags(int has_z, int has_m, int is_geodetic);
uint8_t lwflags_get_g1flags(lwflags_t lwflags);

/*
* GSERIALIZED PUBLIC API
*/

/**
* Read the flags from a #GSERIALIZED and return a standard lwflag
* integer
*/
lwflags_t gserialized1_get_lwflags(const GSERIALIZED *g);

/**
* Copy a new bounding box into an existing gserialized.
* If necessary a new #GSERIALIZED will be allocated. Test
* that input != output before freeing input.
*/
GSERIALIZED *gserialized1_set_gbox(GSERIALIZED *g, GBOX *gbox);

/**
* Remove the bounding box from a #GSERIALIZED. Returns a freshly
* allocated #GSERIALIZED every time.
*/
GSERIALIZED* gserialized1_drop_gbox(GSERIALIZED *g);

/**
* Read the box from the #GSERIALIZED or calculate it if necessary.
* Return #LWFAILURE if box cannot be calculated (NULL or EMPTY
* input).
*/
int gserialized1_get_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/**
* Read the box from the #GSERIALIZED or return #LWFAILURE if
* box is unavailable.
*/
int gserialized1_fast_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/**
* Extract the geometry type from the serialized form (it hides in
* the anonymous data area, so this is a handy function).
*/
uint32_t gserialized1_get_type(const GSERIALIZED *g);

/**
* Returns the size in bytes to read from toast to get the basic
* information from a geometry: GSERIALIZED struct, bbox and type
*/
uint32_t gserialized1_max_header_size(void);

/**
* Returns a hash code for the srid/type/geometry information
* in the GSERIALIZED. Ignores metadata like flags and optional
* boxes, etc.
*/
int32_t gserialized1_hash(const GSERIALIZED *g);

/**
* Extract the SRID from the serialized form (it is packed into
* three bytes so this is a handy function).
*/
int32_t gserialized1_get_srid(const GSERIALIZED *g);

/**
* Write the SRID into the serialized form (it is packed into
* three bytes so this is a handy function).
*/
void gserialized1_set_srid(GSERIALIZED *g, int32_t srid);

/**
* Check if a #GSERIALIZED is empty without deserializing first.
* Only checks if the number of elements of the parent geometry
* is zero, will not catch collections of empty, eg:
* GEOMETRYCOLLECTION(POINT EMPTY)
*/
int gserialized1_is_empty(const GSERIALIZED *g);

/**
* Check if a #GSERIALIZED has a bounding box without deserializing first.
*/
int gserialized1_has_bbox(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED has a Z ordinate.
*/
int gserialized1_has_z(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED has an M ordinate.
*/
int gserialized1_has_m(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED is a geography.
*/
int gserialized1_is_geodetic(const GSERIALIZED *gser);

/**
* Return the number of dimensions (2, 3, 4) in a geometry
*/
int gserialized1_ndims(const GSERIALIZED *gser);

/**
* Allocate a new #GSERIALIZED from an #LWGEOM. For all non-point types, a bounding
* box will be calculated and embedded in the serialization. The geodetic flag is used
* to control the box calculation (cartesian or geocentric). If set, the size pointer
* will contain the size of the final output, which is useful for setting the PgSQL
* VARSIZE information.
*/
GSERIALIZED* gserialized1_from_lwgeom(LWGEOM *geom, size_t *size);

/**
* Return the memory size a GSERIALIZED will occupy for a given LWGEOM.
*/
size_t gserialized1_from_lwgeom_size(const LWGEOM *geom);

/**
* Allocate a new #LWGEOM from a #GSERIALIZED. The resulting #LWGEOM will have coordinates
* that are double aligned and suitable for direct reading using getPoint2d_p_ro
*/
LWGEOM* lwgeom_from_gserialized1(const GSERIALIZED *g);

/**
* Point into the float box area of the serialization
*/
const float * gserialized1_get_float_box_p(const GSERIALIZED *g, size_t *ndims);

int gserialized1_peek_gbox_p(const GSERIALIZED *g, GBOX *gbox);

int gserialized1_peek_first_point(const GSERIALIZED *g, POINT4D *out_point);