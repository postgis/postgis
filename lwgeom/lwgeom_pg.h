#ifndef _LWGEOM_PG_H
#define _LWGEOM_PG_H 1

#include "postgres.h"
#include "utils/geo_decls.h"
#include "fmgr.h"
#include "liblwgeom.h"

void *pg_alloc(size_t size);
void *pg_realloc(void *ptr, size_t size);
void pg_free(void *ptr);
void pg_error(const char *msg, ...);
void pg_notice(const char *msg, ...);

// call this as first thing of any PG function
void init_pg_func(void);

// PG-dependant
extern BOX2DFLOAT4 *box_to_box2df(BOX *box);  // postgresql standard type
extern BOX box2df_to_box(BOX2DFLOAT4 *box);
extern void box2df_to_box_p(BOX2DFLOAT4 *box, BOX *out); // postgresql standard type
// PG-exposed
Datum box2d_same(PG_FUNCTION_ARGS);
Datum box2d_overlap(PG_FUNCTION_ARGS);
Datum box2d_overleft(PG_FUNCTION_ARGS);
Datum box2d_left(PG_FUNCTION_ARGS);
Datum box2d_right(PG_FUNCTION_ARGS);
Datum box2d_overright(PG_FUNCTION_ARGS);
Datum box2d_contained(PG_FUNCTION_ARGS);
Datum box2d_contain(PG_FUNCTION_ARGS);
Datum box2d_inter(PG_FUNCTION_ARGS);
Datum BOX2D_union(PG_FUNCTION_ARGS);

Datum gist_lwgeom_compress(PG_FUNCTION_ARGS);
Datum gist_lwgeom_consistent(PG_FUNCTION_ARGS);
Datum gist_rtree_decompress(PG_FUNCTION_ARGS);
Datum lwgeom_box_union(PG_FUNCTION_ARGS);
Datum lwgeom_box_penalty(PG_FUNCTION_ARGS);
Datum lwgeom_gbox_same(PG_FUNCTION_ARGS);
Datum lwgeom_gbox_picksplit(PG_FUNCTION_ARGS);


#endif // !defined _LWGEOM_PG_H 1
