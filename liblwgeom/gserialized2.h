/**
* Macros for manipulating the core 'flags' byte. A uint8_t used as follows:
*/
#define G2FLAG_Z         0x01
#define G2FLAG_M         0x02
#define G2FLAG_BBOX      0x04
#define G2FLAG_GEODETIC  0x08
#define G2FLAG_EXTENDED  0x10
#define G2FLAG_RESERVED1 0x20 /* RESERVED FOR FUTURE USES */
#define G2FLAG_VER_0     0x40
#define G2FLAG_RESERVED2 0x80 /* RESERVED FOR FUTURE VERSIONS */

/**
* Macros for the extended 'flags' uint64_t.
*/
#define G2FLAG_X_SOLID            0x00000001
#define G2FLAG_X_CHECKED_VALID    0x00000002 // To Be Implemented?
#define G2FLAG_X_IS_VALID         0x00000004 // To Be Implemented?
#define G2FLAG_X_HAS_HASH         0x00000008 // To Be Implemented?

#define G2FLAGS_GET_VERSION(gflags)  (((gflags) & G2FLAG_VER_0)>>6)
#define G2FLAGS_GET_Z(gflags)         ((gflags) & G2FLAG_Z)
#define G2FLAGS_GET_M(gflags)        (((gflags) & G2FLAG_M)>>1)
#define G2FLAGS_GET_BBOX(gflags)     (((gflags) & G2FLAG_BBOX)>>2)
#define G2FLAGS_GET_GEODETIC(gflags) (((gflags) & G2FLAG_GEODETIC)>>3)
#define G2FLAGS_GET_EXTENDED(gflags) (((gflags) & G2FLAG_EXTENDED)>>4)
#define G2FLAGS_GET_UNUSED(gflags)   (((gflags) & G2FLAG_UNUSED)>>5)

#define G2FLAGS_SET_Z(gflags, value) ((gflags) = (value) ? ((gflags) | G2FLAG_Z) : ((gflags) & ~G2FLAG_Z))
#define G2FLAGS_SET_M(gflags, value) ((gflags) = (value) ? ((gflags) | G2FLAG_M) : ((gflags) & ~G2FLAG_M))
#define G2FLAGS_SET_BBOX(gflags, value) ((gflags) = (value) ? ((gflags) | G2FLAG_BBOX) : ((gflags) & ~G2FLAG_BBOX))
#define G2FLAGS_SET_GEODETIC(gflags, value) ((gflags) = (value) ? ((gflags) | G2FLAG_GEODETIC) : ((gflags) & ~G2FLAG_GEODETIC))
#define G2FLAGS_SET_EXTENDED(gflags, value) ((gflags) = (value) ? ((gflags) | G2FLAG_EXTENDED) : ((gflags) & ~G2FLAG_EXTENDED))
#define G2FLAGS_SET_VERSION(gflags, value) ((gflags) = (value) ? ((gflags) | G2FLAG_VER_0) : ((gflags) & ~G2FLAG_VER_0))

#define G2FLAGS_NDIMS(gflags) (2 + G2FLAGS_GET_Z(gflags) + G2FLAGS_GET_M(gflags))
#define G2FLAGS_GET_ZM(gflags) (G2FLAGS_GET_M(gflags) + G2FLAGS_GET_Z(gflags) * 2)
#define G2FLAGS_NDIMS_BOX(gflags) (G2FLAGS_GET_GEODETIC(gflags) ? 3 : G2FLAGS_NDIMS(gflags))

uint8_t g2flags(int has_z, int has_m, int is_geodetic);
uint8_t lwflags_get_g2flags(lwflags_t lwflags);

/*
* GSERIALIZED PUBLIC API
*/

/**
* Read the flags from a #GSERIALIZED and return a standard lwflag
* integer
*/
lwflags_t gserialized2_get_lwflags(const GSERIALIZED *g);

/**
* Copy a new bounding box into an existing gserialized.
* If necessary a new #GSERIALIZED will be allocated. Test
* that input != output before freeing input.
*/
GSERIALIZED *gserialized2_set_gbox(GSERIALIZED *g, GBOX *gbox);

/**
* Remove the bounding box from a #GSERIALIZED. Returns a freshly
* allocated #GSERIALIZED every time.
*/
GSERIALIZED* gserialized2_drop_gbox(GSERIALIZED *g);

/**
* Read the box from the #GSERIALIZED or calculate it if necessary.
* Return #LWFAILURE if box cannot be calculated (NULL or EMPTY
* input).
*/
int gserialized2_get_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/**
* Read the box from the #GSERIALIZED or return #LWFAILURE if
* box is unavailable.
*/
int gserialized2_fast_gbox_p(const GSERIALIZED *g, GBOX *gbox);

/**
* Extract the geometry type from the serialized form (it hides in
* the anonymous data area, so this is a handy function).
*/
uint32_t gserialized2_get_type(const GSERIALIZED *g);

/**
* Returns the size in bytes to read from toast to get the basic
* information from a geometry: GSERIALIZED struct, bbox and type
*/
uint32_t gserialized2_max_header_size(void);

/**
* Returns a hash code for the srid/type/geometry information
* in the GSERIALIZED. Ignores metadata like flags and optional
* boxes, etc.
*/
int32_t gserialized2_hash(const GSERIALIZED *g);

/**
* Extract the SRID from the serialized form (it is packed into
* three bytes so this is a handy function).
*/
int32_t gserialized2_get_srid(const GSERIALIZED *g);

/**
* Write the SRID into the serialized form (it is packed into
* three bytes so this is a handy function).
*/
void gserialized2_set_srid(GSERIALIZED *g, int32_t srid);

/**
* Check if a #GSERIALIZED is empty without deserializing first.
* Only checks if the number of elements of the parent geometry
* is zero, will not catch collections of empty, eg:
* GEOMETRYCOLLECTION(POINT EMPTY)
*/
int gserialized2_is_empty(const GSERIALIZED *g);

/**
* Check if a #GSERIALIZED has a bounding box without deserializing first.
*/
int gserialized2_has_bbox(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED has an extended flags section.
*/
int gserialized2_has_extended(const GSERIALIZED *g);

/**
* Check if a #GSERIALIZED has a Z ordinate.
*/
int gserialized2_has_z(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED has an M ordinate.
*/
int gserialized2_has_m(const GSERIALIZED *gser);

/**
* Check if a #GSERIALIZED is a geography.
*/
int gserialized2_is_geodetic(const GSERIALIZED *gser);

/**
* Return the number of dimensions (2, 3, 4) in a geometry
*/
int gserialized2_ndims(const GSERIALIZED *gser);

/**
* Allocate a new #GSERIALIZED from an #LWGEOM. For all non-point types, a bounding
* box will be calculated and embedded in the serialization. The geodetic flag is used
* to control the box calculation (cartesian or geocentric). If set, the size pointer
* will contain the size of the final output, which is useful for setting the PgSQL
* VARSIZE information.
*/
GSERIALIZED* gserialized2_from_lwgeom(LWGEOM *geom, size_t *size);

/**
* Return the memory size a GSERIALIZED will occupy for a given LWGEOM.
*/
size_t gserialized2_from_lwgeom_size(const LWGEOM *geom);

/**
* Allocate a new #LWGEOM from a #GSERIALIZED. The resulting #LWGEOM will have coordinates
* that are double aligned and suitable for direct reading using getPoint2d_p_ro
*/
LWGEOM* lwgeom_from_gserialized2(const GSERIALIZED *g);

/**
* Point into the float box area of the serialization
*/
const float * gserialized2_get_float_box_p(const GSERIALIZED *g, size_t *ndims);

int gserialized2_peek_gbox_p(const GSERIALIZED *g, GBOX *gbox);

int gserialized2_peek_first_point(const GSERIALIZED *g, POINT4D *out_point);
