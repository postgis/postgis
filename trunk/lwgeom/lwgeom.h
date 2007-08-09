typedef unsigned char uchar;

/*****************************************************************
 * MEMORY MANAGEMENT
 ****************************************************************/
void *lwalloc(size_t size);
void *lwrealloc(void *mem, size_t size);
void lwfree(void *mem);

/*****************************************************************
 * POINT
 ****************************************************************/

typedef struct { double x; double y; } POINT2D;
typedef struct { double x; double y; double m; } POINT3DM;
typedef struct { double x; double y; double z; } POINT3DZ;
typedef struct { double x; double y; double z; double m; } POINT4D;

/*****************************************************************
 * POINTARRAY
 ****************************************************************/

typedef struct POINTARRAY_T *POINTARRAY;

// Constructs a POINTARRAY copying given 2d points
POINTARRAY ptarray_construct2d(unsigned int npoints, const POINT2D *pts);

/*****************************************************************
 * LWGEOM
 ****************************************************************/

typedef struct LWGEOM_T *LWGEOM;

// Conversions
extern char *lwgeom_to_wkt(LWGEOM lwgeom);
extern char *lwgeom_to_ewkt(LWGEOM lwgeom);
extern char *lwgeom_to_hexwkb(LWGEOM lwgeom, unsigned int byteorder);
extern uchar *lwgeom_to_ewkb(char *ewkb, size_t size);
extern LWGEOM lwgeom_from_ewkb(uchar *ewkb, size_t ewkblen);

// Construction
extern LWGEOM make_lwpoint2d(int SRID, double x, double y);
extern LWGEOM make_lwpoint3dz(int SRID, double x, double y, double z);
extern LWGEOM make_lwpoint3dm(int SRID, double x, double y, double m);
extern LWGEOM make_lwpoint4d(int SRID, double x, double y, double z, double m);

// Spatial functions
extern void lwgeom_reverse(LWGEOM lwgeom);
extern void lwgeom_forceRHR(LWGEOM lwgeom);
extern LWGEOM lwgeom_segmentize2d(LWGEOM lwgeom, double dist);
