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
POINTARRAY ptarray_construct2d(unsigned int npoints, POINT2D ** pts);

/*****************************************************************
 * LWGEOM
 ****************************************************************/

typedef struct LWGEOM_T *LWGEOM;

// Conversions
extern char *lwgeom_to_wkt(LWGEOM lwgeom);

// Construction
extern LWGEOM lwpoint_construct(int SRID, char wantbbox, POINTARRAY pa);

// Spatial functions
extern void lwgeom_reverse(LWGEOM lwgeom);
extern void lwgeom_forceRHR(LWGEOM lwgeom);
