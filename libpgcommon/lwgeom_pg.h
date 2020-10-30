/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 *
 * Copyright (C) 2011      Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2009-2011 Paul Ramsey <pramsey@cleverelephant.ca>
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
#include "fmgr.h"
#include "catalog/namespace.h" /* For TypenameGetTypid */
#if POSTGIS_PGSQL_VERSION > 100
#include "catalog/pg_type_d.h" /* For TypenameGetTypid */
#endif
#include "utils/geo_decls.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/syscache.h"

#include "liblwgeom.h"
#include "pgsql_compat.h"


/****************************************************************************************/

typedef enum
{
	GEOMETRYOID = 1,
	GEOGRAPHYOID,
	BOX3DOID,
	BOX2DFOID,
	GIDXOID,
	RASTEROID,
	POSTGISNSPOID
} postgisType;

typedef struct
{
	Oid geometry_oid;
	Oid geography_oid;
	Oid box2df_oid;
	Oid box3d_oid;
	Oid gidx_oid;
	Oid raster_oid;
	Oid install_nsp_oid;
	char *install_nsp;
	char *spatial_ref_sys;
} postgisConstants;

/* Global to hold all the run-time constants */
extern postgisConstants *POSTGIS_CONSTANTS;

/* Infer the install location of postgis, and thus the namespace to use
 * when looking up the type name, and cache oids */
void postgis_initialize_cache(FunctionCallInfo fcinfo);

/* Useful if postgis_initialize_cache() has been called before.
 * Otherwise it tries to do a bare lookup */
Oid postgis_oid(postgisType typ);

/* Returns the fully qualified, and properly quoted, identifier of spatial_ref_sys
 * Note that it's length can be up to strlen(schema) + "." + strlen("spatial_ref_sys") + NULL, i.e: 80 bytes
 * Only useful if postgis_initialize_cache has been called before. Otherwise returns "spatial_ref_sys"
 */
const char *postgis_spatial_ref_sys();

/****************************************************************************************/

/* Globals to hold GEOMETRYOID, GEOGRAPHYOID */
// extern Oid GEOMETRYOID;
// extern Oid GEOGRAPHYOID;
// Oid postgis_geometry_oid(void);
// Oid postgis_geography_oid(void);

/****************************************************************************************/

/* Install PosgreSQL handlers for liblwgeom use */
void pg_install_lwgeom_handlers(void);

/* Argument handling macros */
#define PG_GETARG_GSERIALIZED_P(varno) ((GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_DATUM(varno)))
#define PG_GETARG_GSERIALIZED_P_COPY(varno) ((GSERIALIZED *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(varno)))
#define PG_GSERIALIZED_DATUM_NEEDS_DETOAST(datum) \
	(VARATT_IS_EXTENDED((datum)) || VARATT_IS_EXTERNAL((datum)) || VARATT_IS_COMPRESSED((datum)))
#define PG_GETARG_GSERIALIZED_HEADER(varno) \
	PG_GSERIALIZED_DATUM_NEEDS_DETOAST(PG_GETARG_DATUM(varno)) \
	? ((GSERIALIZED *)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(varno), 0, gserialized_max_header_size())) \
	: ((GSERIALIZED *)(PG_GETARG_DATUM(varno)))

/* Debugging macros */
#if POSTGIS_DEBUG_LEVEL > 0

/* Display a simple message at NOTICE level */
/* from PgSQL utils/elog.h, LOG is 15, and DEBUG5 is 10 and everything else is in between */
#define POSTGIS_DEBUG(level, msg) \
		do { \
				if (POSTGIS_DEBUG_LEVEL >= level) \
						ereport((level < 1 || level > 5) ? DEBUG5 : (LOG - level), (errmsg_internal("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__))); \
		} while (0);

/* Display a formatted message at NOTICE level (like printf, with variadic arguments) */
#define POSTGIS_DEBUGF(level, msg, ...) \
		do { \
				if (POSTGIS_DEBUG_LEVEL >= level) \
						ereport((level < 1 || level > 5) ? DEBUG5 : (LOG - level), (errmsg_internal("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__, __VA_ARGS__))); \
		} while (0);

#else /* POSTGIS_DEBUG_LEVEL */

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define POSTGIS_DEBUG(level, msg) \
		((void) 0)

/* Empty prototype that can be optimised away by the compiler for non-debug builds */
#define POSTGIS_DEBUGF(level, msg, ...) \
		((void) 0)

#endif /* POSTGIS_DEBUG_LEVEL */

/*
* GUC name search functions stolen from PostgreSQL to
* support searching for already-defined GUC variables
*/
int postgis_guc_name_compare(const char *namea, const char *nameb);
int postgis_guc_var_compare(const void *a, const void *b);
int postgis_guc_find_option(const char *name);

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

/* TODO: only cancel the interrupt if inside an outer call ? */
#define LWGEOM_INIT() { \
  lwgeom_cancel_interrupt(); \
}

/**
* Utility method to call the serialization and then set the
* PgSQL varsize header appropriately with the serialized size.
*/
GSERIALIZED *geometry_serialize(LWGEOM *lwgeom);

/**
* Utility method to call the serialization and then set the
* PgSQL varsize header appropriately with the serialized size.
*/
GSERIALIZED* geography_serialize(LWGEOM *lwgeom);

/**
 * Compare SRIDs of two GSERIALIZEDs and print informative error message if they differ.
 */
void gserialized_error_if_srid_mismatch(const GSERIALIZED *g1, const GSERIALIZED *g2, const char *funcname);

/**
 * Compare SRIDs of GSERIALIZEDs to reference and print informative error message if they differ.
 */
void gserialized_error_if_srid_mismatch_reference(const GSERIALIZED *g1, const int32_t srid, const char *funcname);

/**
* Pull out a gbox bounding box as fast as possible.
* Tries to read cached box from front of serialized vardata.
* If no cached box, calculates box from scratch.
* Fails on empty.
*/
int gserialized_datum_get_gbox_p(Datum gsdatum, GBOX *gbox);
int gserialized_datum_get_internals_p(Datum gsdatum, GBOX *gbox, lwflags_t *flags, uint8_t *type, int32_t *srid);

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

/** needed for sp-gist support PostgreSQL 11+ **/
Datum BOX3D_construct(PG_FUNCTION_ARGS);

Datum LWGEOM_to_BOX2DF(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS);
Datum BOX3D_contains(PG_FUNCTION_ARGS);
Datum BOX3D_contained(PG_FUNCTION_ARGS);
Datum BOX3D_overlaps(PG_FUNCTION_ARGS);
Datum BOX3D_same(PG_FUNCTION_ARGS);
Datum BOX3D_left(PG_FUNCTION_ARGS);
Datum BOX3D_overleft(PG_FUNCTION_ARGS);
Datum BOX3D_right(PG_FUNCTION_ARGS);
Datum BOX3D_overright(PG_FUNCTION_ARGS);
Datum BOX3D_below(PG_FUNCTION_ARGS);
Datum BOX3D_overbelow(PG_FUNCTION_ARGS);
Datum BOX3D_above(PG_FUNCTION_ARGS);
Datum BOX3D_overabove(PG_FUNCTION_ARGS);
Datum BOX3D_front(PG_FUNCTION_ARGS);
Datum BOX3D_overfront(PG_FUNCTION_ARGS);
Datum BOX3D_back(PG_FUNCTION_ARGS);
Datum BOX3D_overback(PG_FUNCTION_ARGS);
Datum BOX3D_distance(PG_FUNCTION_ARGS);

#define DatumGetBox3DP(X) ((BOX3D *)DatumGetPointer(X))
#define Box3DPGetDatum(X) PointerGetDatum(X)
#define PG_GETARG_BOX3D_P(n) DatumGetBox3DP(PG_GETARG_DATUM(n))
#define PG_RETURN_BOX3D_P(x) return Box3DPGetDatum(x)

Datum LWGEOM_force_2d(PG_FUNCTION_ARGS);
Datum LWGEOM_force_3dm(PG_FUNCTION_ARGS);
Datum LWGEOM_force_3dz(PG_FUNCTION_ARGS);
Datum LWGEOM_force_4d(PG_FUNCTION_ARGS);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS);
Datum LWGEOM_force_multi(PG_FUNCTION_ARGS);
Datum LWGEOM_force_curve(PG_FUNCTION_ARGS);

Datum LWGEOMFromEWKB(PG_FUNCTION_ARGS);
Datum LWGEOMFromTWKB(PG_FUNCTION_ARGS);

Datum LWGEOM_getBBOX(PG_FUNCTION_ARGS);
Datum LWGEOM_addBBOX(PG_FUNCTION_ARGS);
Datum LWGEOM_dropBBOX(PG_FUNCTION_ARGS);

void lwpgerror(const char *fmt, ...);
void lwpgnotice(const char *fmt, ...);
void lwpgwarning(const char *fmt, ...);

#if POSTGIS_PGSQL_VERSION < 100
Datum CallerFInfoFunctionCall1(PGFunction func, FmgrInfo *flinfo, Oid collation, Datum arg1);
Datum CallerFInfoFunctionCall2(PGFunction func, FmgrInfo *flinfo, Oid collation, Datum arg1, Datum arg2);
#endif
Datum CallerFInfoFunctionCall3(PGFunction func, FmgrInfo *flinfo, Oid collation, Datum arg1, Datum arg2, Datum arg3);



#endif /* !defined _LWGEOM_PG_H */
