/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2019 Darafei Praliaskouski <me@komzpa.net>
 * Copyright 2013 Sandro Santilli <strk@kbt.io>
 * Copyright 2011 Kashif Rasul <kashif.rasul@gmail.com>
 *
 **********************************************************************/

#include "liblwgeom.h"
#include "lwgeom_log.h"
#include "../postgis_config.h"

#if defined(HAVE_LIBJSON) || defined(HAVE_LIBJSON_C) /* --{ */

#define JSON_C_VERSION_013 (13 << 8)

#include <json.h>

#if !defined(JSON_C_VERSION_NUM) || JSON_C_VERSION_NUM < JSON_C_VERSION_013
#include <json_object_private.h>
#endif

#ifndef JSON_C_VERSION
/* Adds support for libjson < 0.10 */
#define json_tokener_error_desc(x) json_tokener_errors[(x)]
#endif

#include <string.h>

static void
geojson_lwerror(char *msg, __attribute__((__unused__)) int error_code)
{
	LWDEBUGF(3, "lwgeom_from_geojson ERROR %i", error_code);
	lwerror("%s", msg);
}

/* Prototype */
static LWGEOM *parse_geojson(json_object *geojson, int *hasz, int root_srid);

static json_object *
findMemberByName(json_object *poObj, const char *pszName)
{
	json_object *poTmp;
	json_object_iter it;

	poTmp = poObj;

	if (!pszName || !poObj)
		return NULL;

	it.key = NULL;
	it.val = NULL;
	it.entry = NULL;

	if (json_object_get_object(poTmp))
	{
		if (!json_object_get_object(poTmp)->head)
		{
			geojson_lwerror("invalid GeoJSON representation", 2);
			return NULL;
		}

		for (it.entry = json_object_get_object(poTmp)->head;
		     (it.entry ? (it.key = (char *)it.entry->k, it.val = (json_object *)it.entry->v, it.entry) : 0);
		     it.entry = it.entry->next)
		{
			if (strcasecmp((char *)it.key, pszName) == 0)
				return it.val;
		}
	}

	return NULL;
}

static int
parse_geojson_coord(json_object *poObj, int *hasz, POINTARRAY *pa)
{
	POINT4D pt = {0, 0, 0, 0};

	LWDEBUGF(3, "parse_geojson_coord called for object %s.", json_object_to_json_string(poObj));

	if (json_type_array == json_object_get_type(poObj))
	{

		json_object *poObjCoord = NULL;
		const int nSize = json_object_array_length(poObj);
		LWDEBUGF(3, "parse_geojson_coord called for array size %d.", nSize);

		if (nSize < 2)
		{
			geojson_lwerror("Too few ordinates in GeoJSON", 4);
			return LW_FAILURE;
		}

		/* Read X coordinate */
		poObjCoord = json_object_array_get_idx(poObj, 0);
		pt.x = json_object_get_double(poObjCoord);
		LWDEBUGF(3, "parse_geojson_coord pt.x = %f.", pt.x);

		/* Read Y coordinate */
		poObjCoord = json_object_array_get_idx(poObj, 1);
		pt.y = json_object_get_double(poObjCoord);
		LWDEBUGF(3, "parse_geojson_coord pt.y = %f.", pt.y);

		if (nSize > 2) /* should this be >= 3 ? */
		{
			/* Read Z coordinate */
			poObjCoord = json_object_array_get_idx(poObj, 2);
			pt.z = json_object_get_double(poObjCoord);
			LWDEBUGF(3, "parse_geojson_coord pt.z = %f.", pt.z);
			*hasz = LW_TRUE;
		}
	}
	else
	{
		/* If it's not an array, just don't handle it */
		return LW_FAILURE;
	}

	return ptarray_append_point(pa, &pt, LW_TRUE);
}

static LWGEOM *
parse_geojson_point(json_object *geojson, int *hasz, int root_srid)
{
	LWGEOM *geom;
	POINTARRAY *pa;
	json_object *coords = NULL;

	LWDEBUGF(3, "parse_geojson_point called with root_srid = %d.", root_srid);

	coords = findMemberByName(geojson, "coordinates");
	if (!coords)
	{
		geojson_lwerror("Unable to find 'coordinates' in GeoJSON string", 4);
		return NULL;
	}

	pa = ptarray_construct_empty(1, 0, 1);
	parse_geojson_coord(coords, hasz, pa);

	geom = (LWGEOM *)lwpoint_construct(root_srid, NULL, pa);
	LWDEBUG(2, "parse_geojson_point finished.");
	return geom;
}

static LWGEOM *
parse_geojson_linestring(json_object *geojson, int *hasz, int root_srid)
{
	LWGEOM *geom;
	POINTARRAY *pa;
	json_object *points = NULL;
	int i = 0;

	LWDEBUG(2, "parse_geojson_linestring called.");

	points = findMemberByName(geojson, "coordinates");
	if (!points)
	{
		geojson_lwerror("Unable to find 'coordinates' in GeoJSON string", 4);
		return NULL;
	}

	pa = ptarray_construct_empty(1, 0, 1);

	if (json_type_array == json_object_get_type(points))
	{
		const int nPoints = json_object_array_length(points);
		for (i = 0; i < nPoints; ++i)
		{
			json_object *coords = NULL;
			coords = json_object_array_get_idx(points, i);
			parse_geojson_coord(coords, hasz, pa);
		}
	}

	geom = (LWGEOM *)lwline_construct(root_srid, NULL, pa);

	LWDEBUG(2, "parse_geojson_linestring finished.");
	return geom;
}

static LWPOLY *
parse_geojson_poly_rings(json_object *rings, int *hasz, int root_srid)
{
	if (!rings)
		return NULL;
	int nRings = json_object_array_length(rings);

	/* No rings => POLYGON EMPTY */
	if (!nRings)
		return lwpoly_construct_empty(root_srid, 1, 0);

	/* Expecting up to nRings otherwise */
	POINTARRAY **ppa = (POINTARRAY **)lwalloc(sizeof(POINTARRAY *) * nRings);
	int o = 0;

	for (int i = 0; i < nRings; i++)
	{
		json_object *points = json_object_array_get_idx(rings, i);
		if (!points || json_object_get_type(points) != json_type_array)
		{
			geojson_lwerror("The 'coordinates' in GeoJSON ring are not an array", 4);
			return NULL;
		}
		int nPoints = json_object_array_length(points);

		/* Skip empty rings */
		if (!nPoints)
		{
			/* Empty outer? Don't promote first hole to outer, holes don't matter. */
			if (!i)
				break;
			else
				continue;
		}

		ppa[o++] = ptarray_construct_empty(1, 0, 1);
		for (int j = 0; j < nPoints; j++)
		{
			json_object *coords = NULL;
			coords = json_object_array_get_idx(points, j);
			if (LW_FAILURE == parse_geojson_coord(coords, hasz, ppa[i]))
			{
				int k;
				for (k = 0; k < o; k++)
					ptarray_free(ppa[k]);
				lwfree(ppa);
				geojson_lwerror("The 'coordinates' in GeoJSON are not sufficiently nested", 4);
				return NULL;
			}
		}
	}

	/* All the rings were empty! */
	if (!o)
	{
		lwfree(ppa);
		return lwpoly_construct_empty(root_srid, 1, 0);
	}

	return lwpoly_construct(root_srid, NULL, o, ppa);
}

static json_object *
parse_coordinates(json_object *geojson)
{
	json_object *coordinates = findMemberByName(geojson, "coordinates");
	if (!coordinates)
	{
		geojson_lwerror("Unable to find 'coordinates' in GeoJSON string", 4);
		return NULL;
	}

	if (json_type_array != json_object_get_type(coordinates))
	{
		geojson_lwerror("The 'coordinates' in GeoJSON are not an array", 4);
		return NULL;
	}
	return coordinates;
}

static LWGEOM *
parse_geojson_polygon(json_object *geojson, int *hasz, int root_srid)
{
	return (LWGEOM *)parse_geojson_poly_rings(parse_coordinates(geojson), hasz, root_srid);
}

static LWGEOM *
parse_geojson_multipoint(json_object *geojson, int *hasz, int root_srid)
{
	json_object *points = parse_coordinates(geojson);
	LWMPOINT *geom = (LWMPOINT *)lwcollection_construct_empty(MULTIPOINTTYPE, root_srid, 1, 0);

	const int nPoints = json_object_array_length(points);
	for (int i = 0; i < nPoints; ++i)
	{
		POINTARRAY *pa = ptarray_construct_empty(1, 0, 1);
		json_object *coord = json_object_array_get_idx(points, i);
		parse_geojson_coord(coord, hasz, pa);
		geom = lwmpoint_add_lwpoint(geom, lwpoint_construct(root_srid, NULL, pa));
	}

	return (LWGEOM *)geom;
}

static LWGEOM *
parse_geojson_multilinestring(json_object *geojson, int *hasz, int root_srid)
{
	json_object *poObjLines = parse_coordinates(geojson);
	LWMLINE *geom = (LWMLINE *)lwcollection_construct_empty(MULTILINETYPE, root_srid, 1, 0);
	const int nLines = json_object_array_length(poObjLines);
	for (int i = 0; i < nLines; ++i)
	{
		POINTARRAY *pa = ptarray_construct_empty(1, 0, 1);
		json_object *poObjLine = json_object_array_get_idx(poObjLines, i);

		if (json_type_array == json_object_get_type(poObjLine))
		{
			const int nPoints = json_object_array_length(poObjLine);
			for (int j = 0; j < nPoints; ++j)
			{
				json_object *coords = json_object_array_get_idx(poObjLine, j);
				parse_geojson_coord(coords, hasz, pa);
			}

			geom = lwmline_add_lwline(geom, lwline_construct(root_srid, NULL, pa));
		}
	}

	return (LWGEOM *)geom;
}

static LWGEOM *
parse_geojson_multipolygon(json_object *geojson, int *hasz, int root_srid)
{
	LWGEOM *geom = (LWGEOM *)lwcollection_construct_empty(MULTIPOLYGONTYPE, root_srid, 1, 0);
	json_object *polys = parse_coordinates(geojson);
	int nPolys = json_object_array_length(polys);

	for (int i = 0; i < nPolys; ++i)
	{
		json_object *rings = json_object_array_get_idx(polys, i);
		LWPOLY *poly = parse_geojson_poly_rings(rings, hasz, root_srid);
		geom = (LWGEOM *)lwmpoly_add_lwpoly((LWMPOLY *)geom, poly);
	}

	return geom;
}

static LWGEOM *
parse_geojson_geometrycollection(json_object *geojson, int *hasz, int root_srid)
{
	LWGEOM *geom = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, root_srid, 1, 0);
	json_object *poObjGeoms = findMemberByName(geojson, "geometries");
	if (!poObjGeoms)
	{
		geojson_lwerror("Unable to find 'geometries' in GeoJSON string", 4);
		return NULL;
	}

	if (json_type_array == json_object_get_type(poObjGeoms))
	{
		const int nGeoms = json_object_array_length(poObjGeoms);
		for (int i = 0; i < nGeoms; ++i)
		{
			json_object *poObjGeom = json_object_array_get_idx(poObjGeoms, i);
			geom = (LWGEOM *)lwcollection_add_lwgeom((LWCOLLECTION *)geom,
								 parse_geojson(poObjGeom, hasz, root_srid));
		}
	}

	return geom;
}

static LWGEOM *
parse_geojson(json_object *geojson, int *hasz, int root_srid)
{
	json_object *type = NULL;
	const char *name;

	if (!geojson)
	{
		geojson_lwerror("invalid GeoJSON representation", 2);
		return NULL;
	}

	type = findMemberByName(geojson, "type");
	if (!type)
	{
		geojson_lwerror("unknown GeoJSON type", 3);
		return NULL;
	}

	name = json_object_get_string(type);

	if (strcasecmp(name, "Point") == 0)
		return parse_geojson_point(geojson, hasz, root_srid);

	if (strcasecmp(name, "LineString") == 0)
		return parse_geojson_linestring(geojson, hasz, root_srid);

	if (strcasecmp(name, "Polygon") == 0)
		return parse_geojson_polygon(geojson, hasz, root_srid);

	if (strcasecmp(name, "MultiPoint") == 0)
		return parse_geojson_multipoint(geojson, hasz, root_srid);

	if (strcasecmp(name, "MultiLineString") == 0)
		return parse_geojson_multilinestring(geojson, hasz, root_srid);

	if (strcasecmp(name, "MultiPolygon") == 0)
		return parse_geojson_multipolygon(geojson, hasz, root_srid);

	if (strcasecmp(name, "GeometryCollection") == 0)
		return parse_geojson_geometrycollection(geojson, hasz, root_srid);

	lwerror("invalid GeoJson representation");
	return NULL; /* Never reach */
}

#endif /* HAVE_LIBJSON or HAVE_LIBJSON_C --} */

LWGEOM *
lwgeom_from_geojson(const char *geojson, char **srs)
{
#ifndef HAVE_LIBJSON
	*srs = NULL;
	lwerror("You need JSON-C for lwgeom_from_geojson");
	return NULL;
#else  /* HAVE_LIBJSON */

	/* size_t geojson_size = strlen(geojson); */

	LWGEOM *lwgeom;
	int hasz = LW_FALSE;
	json_tokener *jstok = NULL;
	json_object *poObj = NULL;
	json_object *poObjSrs = NULL;
	*srs = NULL;

	/* Begin to Parse json */
	jstok = json_tokener_new();
	poObj = json_tokener_parse_ex(jstok, geojson, -1);
	if (jstok->err != json_tokener_success)
	{
		char err[256];
		snprintf(err, 256, "%s (at offset %d)", json_tokener_error_desc(jstok->err), jstok->char_offset);
		json_tokener_free(jstok);
		json_object_put(poObj);
		geojson_lwerror(err, 1);
		return NULL;
	}
	json_tokener_free(jstok);

	poObjSrs = findMemberByName(poObj, "crs");
	if (poObjSrs != NULL)
	{
		json_object *poObjSrsType = findMemberByName(poObjSrs, "type");
		if (poObjSrsType != NULL)
		{
			json_object *poObjSrsProps = findMemberByName(poObjSrs, "properties");
			if (poObjSrsProps)
			{
				json_object *poNameURL = findMemberByName(poObjSrsProps, "name");
				if (poNameURL)
				{
					const char *pszName = json_object_get_string(poNameURL);
					if (pszName)
					{
						*srs = lwalloc(strlen(pszName) + 1);
						strcpy(*srs, pszName);
					}
				}
			}
		}
	}

	lwgeom = parse_geojson(poObj, &hasz, 0);
	json_object_put(poObj);

	lwgeom_add_bbox(lwgeom);

	if (!hasz)
	{
		LWGEOM *tmp = lwgeom_force_2d(lwgeom);
		lwgeom_free(lwgeom);
		lwgeom = tmp;
	}

	return lwgeom;
#endif /* HAVE_LIBJSON } */
}
