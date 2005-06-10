#include "postgres.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "fmgr.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"

Datum postgis_geos_version(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	//elog(ERROR,"JTSversion:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL();
}

Datum postgis_jts_version(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(postgis_jts_version);
Datum postgis_jts_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_NULL();
}

Datum intersection(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(intersection);
Datum intersection(PG_FUNCTION_ARGS)
{
	elog(ERROR,"intersection:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum convexhull(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(convexhull);
Datum convexhull(PG_FUNCTION_ARGS)
{
	elog(ERROR,"convexhull:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum difference(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(difference);
Datum difference(PG_FUNCTION_ARGS)
{
	elog(ERROR,"difference:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum boundary(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
	elog(ERROR,"boundary:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum symdifference(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
	elog(ERROR,"symdifference:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum geomunion(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
	elog(ERROR,"geomunion:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum buffer(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
	elog(ERROR,"buffer:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum relate_full(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(relate_full);
Datum relate_full(PG_FUNCTION_ARGS)
{
	elog(ERROR,"relate_full:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum relate_pattern(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(relate_pattern);
Datum relate_pattern(PG_FUNCTION_ARGS)
{
	elog(ERROR,"relate_pattern:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum disjoint(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(disjoint);
Datum disjoint(PG_FUNCTION_ARGS)
{
	elog(ERROR,"disjoint:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum intersects(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(intersects);
Datum intersects(PG_FUNCTION_ARGS)
{
	elog(ERROR,"intersects:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum touches(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	elog(ERROR,"touches:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum crosses(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(crosses);
Datum crosses(PG_FUNCTION_ARGS)
{
	elog(ERROR,"crosses:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum within(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	elog(ERROR,"within:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum contains(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(contains);
Datum contains(PG_FUNCTION_ARGS)
{
	elog(ERROR,"contains:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum overlaps(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	elog(ERROR,"overlaps:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum isvalid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(isvalid);
Datum isvalid(PG_FUNCTION_ARGS)
{
	elog(ERROR,"isvalid:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum issimple(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(issimple);
Datum issimple(PG_FUNCTION_ARGS)
{
	elog(ERROR,"issimple:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum geomequals(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(geomequals);
Datum geomequals(PG_FUNCTION_ARGS)
{
	elog(ERROR,"geomequals:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum isring(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(isring);
Datum isring(PG_FUNCTION_ARGS)
{
	elog(ERROR,"isring:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum pointonsurface(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(pointonsurface);
Datum pointonsurface(PG_FUNCTION_ARGS)
{
	elog(ERROR,"pointonsurface:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum unite_garray(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(unite_garray);
Datum unite_garray(PG_FUNCTION_ARGS)
{
	elog(ERROR,"unite_garray:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum GEOS_polygonize_garray(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(GEOS_polygonize_garray);
Datum GEOS_polygonize_garray(PG_FUNCTION_ARGS)
{
	elog(ERROR,"JTS_polygonize_garray:: operation not implemented - compile PostGIS with JTS or GEOS support");
	PG_RETURN_NULL(); // never get here
}

Datum JTSnoop(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(JTSnoop);
Datum JTSnoop(PG_FUNCTION_ARGS)
{
	elog(ERROR,"JTSnoop:: operation not implemented - compile PostGIS with JTS support");
	PG_RETURN_NULL(); // never get here
}

Datum GEOSnoop(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(GEOSnoop);
Datum GEOSnoop(PG_FUNCTION_ARGS)
{
	elog(ERROR,"GEOSnoop:: operation not implemented - compile PostGIS with GEOS support");
	PG_RETURN_NULL(); // never get here
}
