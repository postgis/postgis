/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 *
 * http://postgis.refractions.net
 *
 * Copyright (C) 2011      Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2009-2010 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2008      Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright (C) 2004-2007 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _LWGEOM_PG_H
#define _LWGEOM_PG_H 1

#include "postgres.h"
#include "utils/geo_decls.h"
#include "fmgr.h"

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "pgsql_compat.h"

void *pg_alloc(size_t size);
void *pg_realloc(void *ptr, size_t size);
void pg_free(void *ptr);
void pg_error(const char *msg, va_list vp);
void pg_notice(const char *msg, va_list vp);

/*
 * This is the binary representation of lwgeom compatible
 * with postgresql varlena struct
 */
typedef struct
{
	uint32 size;        /* varlena header (do not touch directly!) */
	uchar type;         /* encodes ndims, type, bbox presence,
			                srid presence */
	uchar data[1];
}
PG_LWGEOM;


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


/*
* Temporary changeover defines for PG_LWGEOM and GSERIALIZED
*/
#include "gserialized.h"
#ifdef GSERIALIZED_ON
#define PG_LWGEOM GSERIALIZED
#define BOX2DFLOAT4 GBOX
#endif

/*
** GSERIALIED prototypes used outside the index functions
*/

/* Remove the embedded bounding box */
GSERIALIZED* gserialized_drop_gidx(GSERIALIZED *g);




/* Serialize/deserialize a PG_LWGEOM (postgis datatype) */
extern PG_LWGEOM *pglwgeom_serialize(LWGEOM *lwgeom);
extern LWGEOM *pglwgeom_deserialize(PG_LWGEOM *pglwgeom);

/*
 * Construct a full PG_LWGEOM type (including size header)
 * from a serialized form.
 * The constructed PG_LWGEOM object will be allocated using palloc
 * and the serialized form will be copied.
 * If you specify a SRID other then -1 it will be set.
 * If you request bbox (wantbbox=1) it will be extracted or computed
 * from the serialized form.
 * 
 * NOTE: only available when GSERIALIZED_ON is undefined
 * TODO: wrap in #ifndef GSERIALIZED_ON
 */
extern PG_LWGEOM *PG_LWGEOM_construct(uchar *serialized, int srid, int wantbbox);

/* PG_LWGEOM SRID get/set */
extern PG_LWGEOM *pglwgeom_set_srid(PG_LWGEOM *pglwgeom, int32 newSRID);
extern int pglwgeom_get_srid(PG_LWGEOM *pglwgeom);
extern int pglwgeom_get_type(const PG_LWGEOM *lwgeom);
extern int pglwgeom_get_zm(const PG_LWGEOM *lwgeom);
extern PG_LWGEOM* pglwgeom_drop_bbox(PG_LWGEOM *geom);
extern size_t pglwgeom_size(const PG_LWGEOM *geom);
extern int pglwgeom_ndims(const PG_LWGEOM *geom);
extern bool pglwgeom_has_bbox(const PG_LWGEOM *lwgeom);
extern bool pglwgeom_has_z(const PG_LWGEOM *lwgeom);
extern bool pglwgeom_has_m(const PG_LWGEOM *lwgeom);
extern int pglwgeom_is_empty(const PG_LWGEOM *geom);
/*
 * Get the 2d bounding box of the given geometry, in FLOAT4 format.
 * Use a cached bbox if available, compute it otherwise.
 * Return LW_FALSE if the geometry has no bounding box (ie: is empty).
 */
extern int pglwgeom_getbox2d_p(const PG_LWGEOM *geom, BOX2DFLOAT4 *box);
extern BOX3D *pglwgeom_compute_serialized_box3d(const PG_LWGEOM *geom);
extern int pglwgeom_compute_serialized_box3d_p(const PG_LWGEOM *geom, BOX3D *box3d);
extern char is_worth_caching_pglwgeom_bbox(const PG_LWGEOM *);

/* PG-dependant */

/**
* Utility to convert cstrings to textp pointers 
*/
text* cstring2text(const char *cstring);
char* text2cstring(const text *textptr);

/*
 * Use this macro to extract the char * required
 * by most functions from an PG_LWGEOM struct.
 * (which is an PG_LWGEOM w/out int32 size casted to char *)
 */
#define SERIALIZED_FORM(x) ((uchar *)VARDATA((x)))

/* 
 * For PostgreSQL >= 8.5 redefine the STATRELATT macro to its
 * new value of STATRELATTINH 
 */
#if POSTGIS_PGSQL_VERSION >= 85
	#define STATRELATT STATRELATTINH
#endif

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
