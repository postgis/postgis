#ifndef _LWPGIS_DEFINES
#define _LWPGIS_DEFINES

#include "../postgis_svn_revision.h"

/*
 * Define just the version numbers; otherwise we get some strange substitutions in postgis.sql.in
 */
#define POSTGIS_PGSQL_VERSION 91
#define POSTGIS_GEOS_VERSION 33
#define POSTGIS_PROJ_VERSION 48
#define POSTGIS_LIB_VERSION 2.1.0
#define POSTGIS_LIBXML2_VERSION 2.9.1

/*
 * Define the build date and the version number
 * (these substitiutions are done with extra quotes sinces CPP
 * won't substitute within apostrophes)
 */
#define _POSTGIS_SQL_SELECT_POSTGIS_VERSION 'SELECT ''2.1 USE_GEOS=1 USE_PROJ=1 USE_STATS=1''::text AS version'
#define _POSTGIS_SQL_SELECT_POSTGIS_BUILD_DATE 'SELECT ''2013-10-30 17:44:27''::text AS version'

#if POSTGIS_SVN_REVISION
#define _POSTGIS_SQL_SELECT_POSTGIS_SCRIPTS_VERSION $$ SELECT '2.1.0'::text || ' r' || POSTGIS_SVN_REVISION::text AS version $$
#else
#define _POSTGIS_SQL_SELECT_POSTGIS_SCRIPTS_VERSION $$ SELECT '2.1.0'::text AS version $$
#endif

#define SRID_USR_MAX 998999

#endif /* _LWPGIS_DEFINES */


