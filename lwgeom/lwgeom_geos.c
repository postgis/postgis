
#include "postgres.h"
#include "fmgr.h"

#if USE_GEOS

extern char *GEOSversion();
extern char *GEOSjtsport();

PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	char *ver = GEOSversion();
	text *result;
	result = (text *) palloc(VARHDRSZ  + strlen(ver));
	VARATT_SIZEP(result) = VARHDRSZ + strlen(ver) ;
	memcpy(VARDATA(result), ver, strlen(ver));
	free(ver);
	PG_RETURN_POINTER(result);
}

#else // ! defined USE_GEOS

PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	//elog(ERROR,"GEOSversion:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL();
}

#endif
