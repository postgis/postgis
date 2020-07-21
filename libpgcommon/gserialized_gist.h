#include <float.h>
/**********************************************************************
**  GIDX structure.
**
**  This is an n-dimensional (practically, the
**  implementation is 2-4 dimensions) box used for index keys. The
**  coordinates are anonymous, so we can have any number of dimensions.
**  The sizeof a GIDX is 1 + 2 * ndims * sizeof(float).
**  The order of values in a GIDX is
**  xmin,xmax,ymin,ymax,zmin,zmax...
*/
typedef struct
{
	int32 varsize;
	float c[1];
} GIDX;

/*
** For some GiST support functions, it is more efficient to allocate
** memory for our GIDX off the stack and cast that memory into a GIDX.
** But, GIDX is variable length, what to do? We'll bake in the assumption
** that no GIDX is more than 4-dimensional for now, and ensure that much
** space is available.
** 4 bytes varsize + 4 dimensions * 2 ordinates * 4 bytes float size = 36 bytes
*/
#define GIDX_MAX_SIZE 36
#define GIDX_MAX_DIM 4

/**********************************************************************
**  BOX2DF structure.
**
**  This is a 2-dimensional key for simple cartesian indexes,
**  with backwards compatible behavior to previous indexes in
**  PostGIS
*/

typedef struct
{
	float xmin, xmax, ymin, ymax;
} BOX2DF;

/*********************************************************************************
** GIDX support functions.
**
** We put the GIDX support here rather than libgeom because it is a specialized
** type only used for indexing purposes. It also makes use of some PgSQL
** infrastructure like the VARSIZE() macros.
*/

/* allocate a new gidx object on the heap */
GIDX *gidx_new(int ndims);

/* Note empty GIDX */
bool gidx_is_unknown(const GIDX *a);

/* Generate human readable form for GIDX. */
char *gidx_to_string(GIDX *a);

/* typedef to correct array-bounds checking for casts to GIDX - do not
   use this ANYWHERE except in the casts below */
typedef float _gidx_float_array[sizeof(float) * 2 * 4];

/* Returns number of dimensions for this GIDX */
#define GIDX_NDIMS(gidx) ((VARSIZE((gidx)) - VARHDRSZ) / (2 * sizeof(float)))
/* Minimum accessor. */
#define GIDX_GET_MIN(gidx, dimension) (*((_gidx_float_array *)(&(gidx)->c)))[2 * (dimension)]
/* Maximum accessor. */
#define GIDX_GET_MAX(gidx, dimension) (*((_gidx_float_array *)(&(gidx)->c)))[2 * (dimension) + 1]
/* Minimum setter. */
#define GIDX_SET_MIN(gidx, dimension, value) ((gidx)->c[2 * (dimension)] = (value))
/* Maximum setter. */
#define GIDX_SET_MAX(gidx, dimension, value) ((gidx)->c[2 * (dimension) + 1] = (value))
/* Returns the size required to store a GIDX of requested dimension */
#define GIDX_SIZE(dimensions) (sizeof(int32) + 2 * (dimensions) * sizeof(float))

/* Allocate a copy of the box */
BOX2DF *box2df_copy(BOX2DF *b);

/* Grow the first argument to contain the second */
void box2df_merge(BOX2DF *b_union, BOX2DF *b_new);

/* Allocate a copy of the box */
GIDX *gidx_copy(GIDX *b);

/* Grow the first argument to contain the second */
void gidx_merge(GIDX **b_union, GIDX *b_new);

/* Note empty BOX2DF */
bool box2df_is_empty(const BOX2DF *a);

/* Fill in a gbox from a GIDX */
void gbox_from_gidx(GIDX *a, GBOX *gbox, int flags);

/* Fill in a gbox from a BOX2DF */
int box2df_to_gbox_p(BOX2DF *a, GBOX *box);

/*********************************************************************************
** GSERIALIZED support functions.
**
** Fast functions for pulling boxes out of serializations.
*/

/* Pull out the #GIDX bounding box and flags with a absolute minimum system overhead */
int gserialized_datum_get_gidx_p(Datum gserialized_datum, GIDX *gidx);
int gserialized_datum_get_box2df_p(Datum gsdatum, BOX2DF *box2df);

bool box2df_contains(const BOX2DF *a, const BOX2DF *b);
void box2df_set_empty(BOX2DF *a);
void box2df_set_finite(BOX2DF *a);
void box2df_validate(BOX2DF *b);
bool box2df_overlaps(const BOX2DF *a, const BOX2DF *b);
bool box2df_overleft(const BOX2DF *a, const BOX2DF *b);
bool box2df_equals(const BOX2DF *a, const BOX2DF *b);
bool box2df_left(const BOX2DF *a, const BOX2DF *b);
bool box2df_right(const BOX2DF *a, const BOX2DF *b);
bool box2df_overright(const BOX2DF *a, const BOX2DF *b);
bool box2df_overbelow(const BOX2DF *a, const BOX2DF *b);
bool box2df_below(const BOX2DF *a, const BOX2DF *b);
bool box2df_above(const BOX2DF *a, const BOX2DF *b);
bool box2df_overabove(const BOX2DF *a, const BOX2DF *b);

void gidx_validate(GIDX *b);
void gidx_set_unknown(GIDX *a);
bool gidx_overlaps(GIDX *a, GIDX *b);
bool gidx_equals(GIDX *a, GIDX *b);
bool gidx_contains(GIDX *a, GIDX *b);
