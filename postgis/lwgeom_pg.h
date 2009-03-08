#ifndef _LWGEOM_PG_H
#define _LWGEOM_PG_H 1

#include "../postgis_config.h"
#include "postgres.h"
#include "utils/geo_decls.h"
#include "fmgr.h"
#include "liblwgeom.h"
#include "pgsql_compat.h"

void *pg_alloc(size_t size);
void *pg_realloc(void *ptr, size_t size);
void pg_free(void *ptr);
void pg_error(const char *msg, va_list vp);
void pg_notice(const char *msg, va_list vp);


/* Debugging macros */
#if POSTGIS_DEBUG_LEVEL > 0

/* Display a simple message at NOTICE level */
#define POSTGIS_DEBUG(level, msg) \
        do { \
                if (POSTGIS_DEBUG_LEVEL >= level) \
                        ereport(NOTICE, (errmsg_internal("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__))); \
        } while (0);

/* Display a formatted message at NOTICE level (like printf, with variadic arguments) */
#define POSTGIS_DEBUGF(level, msg, ...) \
        do { \
                if (POSTGIS_DEBUG_LEVEL >= level) \
                        ereport(NOTICE, (errmsg_internal("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__, __VA_ARGS__))); \
        } while (0);

#else

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define POSTGIS_DEBUG(level, msg) \
        ((void) 0)

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define POSTGIS_DEBUGF(level, msg, ...) \
        ((void) 0)

#endif


/*
 * Standard macro for reporting parser errors to PostgreSQL
 */

extern void pg_parser_errhint(LWGEOM_PARSER_RESULT *lwg_parser_result);
extern void pg_unparser_errhint(LWGEOM_UNPARSER_RESULT *lwg_unparser_result);

#define PG_PARSER_ERROR(lwg_parser_result) \
        do { \
                pg_parser_errhint(&lwg_parser_result); \
        } while(0);

/*
 * Standard macro for reporting unparser errors to PostgreSQL
 */
#define PG_UNPARSER_ERROR(lwg_unparser_result) \
        do { \
                pg_unparser_errhint(&lwg_unparser_result); \
        } while(0);


/* Serialize/deserialize a PG_LWGEOM (postgis datatype) */
extern PG_LWGEOM *pglwgeom_serialize(LWGEOM *lwgeom);
extern LWGEOM *pglwgeom_deserialize(PG_LWGEOM *pglwgeom);

/* PG_LWGEOM WKB IO */
extern PG_LWGEOM *pglwgeom_from_ewkb(uchar *ewkb, int flags, size_t ewkblen);
extern char *pglwgeom_to_ewkb(PG_LWGEOM *geom, int flags, char byteorder, size_t *ewkblen);

/* PG_LWGEOM SRID get/set */
extern PG_LWGEOM *pglwgeom_setSRID(PG_LWGEOM *pglwgeom, int32 newSRID);
extern int pglwgeom_getSRID(PG_LWGEOM *pglwgeom);

extern Oid getGeometryOID(void);

/* PG-dependant */
/* BOX is postgresql standard type */
extern void box_to_box3d_p(BOX *box, BOX3D *out);  
extern void box3d_to_box_p(BOX3D *box, BOX *out);

/* PG-exposed */
Datum BOX2D_same(PG_FUNCTION_ARGS);
Datum BOX2D_overlap(PG_FUNCTION_ARGS);
Datum BOX2D_overleft(PG_FUNCTION_ARGS);
Datum BOX2D_left(PG_FUNCTION_ARGS);
Datum BOX2D_right(PG_FUNCTION_ARGS);
Datum BOX2D_overright(PG_FUNCTION_ARGS);
Datum BOX2D_overbelow(PG_FUNCTION_ARGS);
Datum BOX2D_below(PG_FUNCTION_ARGS);
Datum BOX2D_above(PG_FUNCTION_ARGS);
Datum BOX2D_overabove(PG_FUNCTION_ARGS);
Datum BOX2D_contained(PG_FUNCTION_ARGS);
Datum BOX2D_contain(PG_FUNCTION_ARGS);
Datum BOX2D_intersects(PG_FUNCTION_ARGS);
Datum BOX2D_union(PG_FUNCTION_ARGS);

Datum LWGEOM_same(PG_FUNCTION_ARGS);
Datum BOX3D_construct(PG_FUNCTION_ARGS);
Datum BOX2DFLOAT4_ymin(PG_FUNCTION_ARGS);

Datum LWGEOM_force_2d(PG_FUNCTION_ARGS);
Datum LWGEOM_force_3dm(PG_FUNCTION_ARGS);
Datum LWGEOM_force_3dz(PG_FUNCTION_ARGS);
Datum LWGEOM_force_4d(PG_FUNCTION_ARGS);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS);
Datum LWGEOM_force_multi(PG_FUNCTION_ARGS);

Datum LWGEOMFromWKB(PG_FUNCTION_ARGS);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS);

Datum LWGEOM_getBBOX(PG_FUNCTION_ARGS);
Datum LWGEOM_addBBOX(PG_FUNCTION_ARGS);
Datum LWGEOM_dropBBOX(PG_FUNCTION_ARGS);

#endif /* !defined _LWGEOM_PG_H 1 */
