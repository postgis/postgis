
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

#define G1FLAGS_GET_Z(gflags)         ((gflags) & G1FLAG_Z)
#define G1FLAGS_GET_M(gflags)        (((gflags) & G1FLAG_M)>>1)
#define G1FLAGS_GET_BBOX(gflags)     (((gflags) & G1FLAG_BBOX)>>2)
#define G1FLAGS_GET_GEODETIC(gflags) (((gflags) & G1FLAG_GEODETIC)>>3)
#define G1FLAGS_GET_READONLY(gflags) (((gflags) & G1FLAG_READONLY)>>4)
#define G1FLAGS_GET_SOLID(gflags)    (((gflags) & G1FLAG_SOLID)>>5)

#define G1FLAGS_SET_Z(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_Z) : ((gflags) & ~G1FLAG_Z))
#define G1FLAGS_SET_M(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_M) : ((gflags) & ~G1FLAG_M))
#define G1FLAGS_SET_BBOX(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_BBOX) : ((gflags) & ~G1FLAG_BBOX))
#define G1FLAGS_SET_GEODETIC(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_GEODETIC) : ((gflags) & ~G1FLAG_GEODETIC))
#define G1FLAGS_SET_READONLY(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_READONLY) : ((gflags) & ~G1FLAG_READONLY))
#define G1FLAGS_SET_SOLID(gflags, value) ((gflags) = (value) ? ((gflags) | G1FLAG_SOLID) : ((gflags) & ~G1FLAG_SOLID))

#define G1FLAGS_NDIMS(gflags) (2 + G1FLAGS_GET_Z(gflags) + G1FLAGS_GET_M(gflags))
#define G1FLAGS_GET_ZM(gflags) (G1FLAGS_GET_M(gflags) + G1FLAGS_GET_Z(gflags) * 2)
#define G1FLAGS_NDIMS_BOX(gflags) (G1FLAGS_GET_GEODETIC(gflags) ? 3 : G1FLAGS_NDIMS(gflags))

uint8_t g1flags(int has_z, int has_m, int is_geodetic);
uint8_t lwgeom_get_gflags(const LWGEOM *geom);
