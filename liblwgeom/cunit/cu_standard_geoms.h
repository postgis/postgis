/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _CU_STANDARD_GEOMS_H
#define _CU_STANDARD_GEOMS_H 1

#include <stddef.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define CU_UNUSED __attribute__((unused))
#else
#define CU_UNUSED
#endif

#define CU_STANDARD_GEOM_EMPTY        (1U << 0)
#define CU_STANDARD_GEOM_Z            (1U << 1)
#define CU_STANDARD_GEOM_M            (1U << 2)
#define CU_STANDARD_GEOM_CURVE        (1U << 3)
#define CU_STANDARD_GEOM_SURFACE      (1U << 4)
#define CU_STANDARD_GEOM_COLLECTION   (1U << 5)
#define CU_STANDARD_GEOM_GEOS_NOOP    (1U << 6)
#define CU_STANDARD_GEOM_GSERIALIZED1 (1U << 7)
#define CU_STANDARD_GEOM_GSERIALIZED2 (1U << 8)

#define CU_STANDARD_GEOM_SERIALIZED (CU_STANDARD_GEOM_GSERIALIZED1 | CU_STANDARD_GEOM_GSERIALIZED2)

typedef struct cu_standard_geom
{
	const char *id;
	const char *ewkt;
	unsigned int flags;
} cu_standard_geom;

static const cu_standard_geom cu_standard_geoms[] CU_UNUSED = {
    {"point-empty", "POINT EMPTY", CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_SERIALIZED},
    {"point-2d", "POINT(0 0.2)", CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"linestring-empty", "LINESTRING EMPTY", CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_SERIALIZED},
    {"linestring-2d", "LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
     CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"multipoint-empty", "MULTIPOINT EMPTY", CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_SERIALIZED},
    {"multipoint-repeated", "MULTIPOINT(0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9)",
     CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"multilinestring-empty-srid1", "SRID=1;MULTILINESTRING EMPTY",
     CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_SERIALIZED},
    {"multilinestring-2d-srid1",
     "SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
     CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"polygon-2d-with-hole", "POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
     CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"polygon-empty", "POLYGON EMPTY", CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_SERIALIZED},
    {"polygon-srid4326", "SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
     CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"polygon-empty-srid4326", "SRID=4326;POLYGON EMPTY", CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_SERIALIZED},
    {"polygon-multi-hole-srid4326",
     "SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))",
     CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"polygon-z-srid100000",
     "SRID=100000;POLYGON((-1 -1 3,-1 2.5 3,2 2 3,2 -1 3,-1 -1 3),(0 0 3,0 1 3,1 1 3,1 0 3,0 0 3),(-0.5 -0.5 3,-0.5 -0.4 3,-0.4 -0.4 3,-0.4 -0.5 3,-0.5 -0.5 3))",
     CU_STANDARD_GEOM_Z | CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"multipolygon-srid4326",
     "SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
     CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"multipolygon-empty-srid4326", "SRID=4326;MULTIPOLYGON EMPTY",
     CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_SERIALIZED},
    {"collection-mixed-srid4326",
     "SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
     CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_GEOS_NOOP | CU_STANDARD_GEOM_SERIALIZED},
    {"collection-empty-srid4326", "SRID=4326;GEOMETRYCOLLECTION EMPTY",
     CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_SERIALIZED},
    {"collection-empty-members-srid4326", "SRID=4326;GEOMETRYCOLLECTION(POINT EMPTY,MULTIPOLYGON EMPTY)",
     CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_SERIALIZED},
    {"multicurve-zm",
     "MULTICURVE((5 5 1 3,3 5 2 2,3 3 3 1,0 3 1 1),CIRCULARSTRING(0 0 0 0,0.26794 1 3 -2,0.5857864 1.414213 1 2))",
     CU_STANDARD_GEOM_Z | CU_STANDARD_GEOM_M | CU_STANDARD_GEOM_CURVE | CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_SERIALIZED},
    {"multisurface-curve",
     "MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
     CU_STANDARD_GEOM_CURVE | CU_STANDARD_GEOM_SURFACE | CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_SERIALIZED},
    {"multisurface-empty-curve", "MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING EMPTY))",
     CU_STANDARD_GEOM_EMPTY | CU_STANDARD_GEOM_CURVE | CU_STANDARD_GEOM_SURFACE | CU_STANDARD_GEOM_COLLECTION | CU_STANDARD_GEOM_SERIALIZED},
};

static const size_t cu_standard_geoms_count CU_UNUSED =
    sizeof(cu_standard_geoms) / sizeof(cu_standard_geoms[0]);

static CU_UNUSED int
cu_standard_geom_matches(const cu_standard_geom *geom, unsigned int include_flags, unsigned int exclude_flags)
{
	return ((geom->flags & include_flags) == include_flags) && ((geom->flags & exclude_flags) == 0);
}

static CU_UNUSED const cu_standard_geom *
cu_standard_geom_by_id(const char *id)
{
	size_t i;

	for (i = 0; i < cu_standard_geoms_count; i++)
	{
		if (strcmp(cu_standard_geoms[i].id, id) == 0)
			return &cu_standard_geoms[i];
	}

	return NULL;
}

static CU_UNUSED size_t
cu_standard_geoms_select(const cu_standard_geom **out, size_t out_size, unsigned int include_flags, unsigned int exclude_flags)
{
	size_t i;
	size_t count = 0;

	for (i = 0; i < cu_standard_geoms_count; i++)
	{
		if (!cu_standard_geom_matches(&cu_standard_geoms[i], include_flags, exclude_flags))
			continue;

		if (count < out_size)
			out[count] = &cu_standard_geoms[i];
		count++;
	}

	return count;
}

#undef CU_UNUSED

#endif
