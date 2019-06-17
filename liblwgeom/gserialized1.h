
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

#define G1FLAGS_GET_Z(flags)         ((flags) & G1FLAG_Z)
#define G1FLAGS_GET_M(flags)        (((flags) & G1FLAG_M)>>1)
#define G1FLAGS_GET_BBOX(flags)     (((flags) & G1FLAG_BBOX)>>2)
#define G1FLAGS_GET_GEODETIC(flags) (((flags) & G1FLAG_GEODETIC)>>3)
#define G1FLAGS_GET_READONLY(flags) (((flags) & G1FLAG_READONLY)>>4)
#define G1FLAGS_GET_SOLID(flags)    (((flags) & G1FLAG_SOLID)>>5)

#define G1FLAGS_SET_Z(flags, value) ((flags) = (value) ? ((flags) | G1FLAG_Z) : ((flags) & ~G1FLAG_Z))
#define G1FLAGS_SET_M(flags, value) ((flags) = (value) ? ((flags) | G1FLAG_M) : ((flags) & ~G1FLAG_M))
#define G1FLAGS_SET_BBOX(flags, value) ((flags) = (value) ? ((flags) | G1FLAG_BBOX) : ((flags) & ~G1FLAG_BBOX))
#define G1FLAGS_SET_GEODETIC(flags, value) ((flags) = (value) ? ((flags) | G1FLAG_GEODETIC) : ((flags) & ~G1FLAG_GEODETIC))
#define G1FLAGS_SET_READONLY(flags, value) ((flags) = (value) ? ((flags) | G1FLAG_READONLY) : ((flags) & ~G1FLAG_READONLY))
#define G1FLAGS_SET_SOLID(flags, value) ((flags) = (value) ? ((flags) | G1FLAG_SOLID) : ((flags) & ~G1FLAG_SOLID))

#define G1FLAGS_NDIMS(flags) (2 + G1FLAGS_GET_Z(flags) + G1FLAGS_GET_M(flags))
#define G1FLAGS_GET_ZM(flags) (G1FLAGS_GET_M(flags) + G1FLAGS_GET_Z(flags) * 2)
#define G1FLAGS_NDIMS_BOX(flags) (G1FLAGS_GET_GEODETIC(flags) ? 3 : G1FLAGS_NDIMS(flags))

uint8_t g1flags(int has_z, int has_m, int is_geodetic);
