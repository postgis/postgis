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

#if defined(HAVE_LIBJSON)

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

/* Prototype */
static LWGEOM *parse_geojson(json_object *geojson, int *hasz);

static inline json_object *
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
			lwerror("invalid GeoJSON representation");
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

static inline json_object *
parse_coordinates(json_object *geojson)
{
	json_object *coordinates = findMemberByName(geojson, "coordinates");
	if (!coordinates)
	{
		lwerror("Unable to find 'coordinates' in GeoJSON string");
		return NULL;
	}

	if (json_type_array != json_object_get_type(coordinates))
	{
		lwerror("The 'coordinates' in GeoJSON are not an array");
		return NULL;
	}
	return coordinates;
}


static inline int
parse_geojson_coord(json_object *poObj, int *hasz, POINTARRAY *pa)
{
	POINT4D pt = {0, 0, 0, 0};

	if (json_object_get_type(poObj) == json_type_array)
	{
		json_object *poObjCoord = NULL;
		const int nSize = json_object_array_length(poObj);
		if (nSize < 2)
		{
			lwerror("Too few ordinates in GeoJSON");
			return LW_FAILURE;
		}

		/* Read X coordinate */
		poObjCoord = json_object_array_get_idx(poObj, 0);
		pt.x = json_object_get_double(poObjCoord);

		/* Read Y coordinate */
		poObjCoord = json_object_array_get_idx(poObj, 1);
		pt.y = json_object_get_double(poObjCoord);

		if (nSize > 2) /* should this be >= 3 ? */
		{
			/* Read Z coordinate */
			poObjCoord = json_object_array_get_idx(poObj, 2);
			pt.z = json_object_get_double(poObjCoord);
			*hasz = LW_TRUE;
		}
	}
	else
	{
		/* If it's not an array, just don't handle it */
		lwerror("The 'coordinates' in GeoJSON are not sufficiently nested");
		return LW_FAILURE;
	}

	return ptarray_append_point(pa, &pt, LW_TRUE);
}

static inline LWGEOM *
parse_geojson_point(json_object *geojson, int *hasz)
{
	json_object *coords = parse_coordinates(geojson);
	if (!coords)
		return NULL;
	POINTARRAY *pa = ptarray_construct_empty(1, 0, 1);
	parse_geojson_coord(coords, hasz, pa);
	return (LWGEOM *)lwpoint_construct(0, NULL, pa);
}

static inline LWGEOM *
parse_geojson_linestring(json_object *geojson, int *hasz)
{
	json_object *points = parse_coordinates(geojson);
	if (!points)
		return NULL;
	POINTARRAY *pa = ptarray_construct_empty(1, 0, 1);
	const int nPoints = json_object_array_length(points);
	for (int i = 0; i < nPoints; i++)
	{
		json_object *coords = json_object_array_get_idx(points, i);
		parse_geojson_coord(coords, hasz, pa);
	}
	return (LWGEOM *)lwline_construct(0, NULL, pa);
}

static inline LWPOLY *
parse_geojson_poly_rings(json_object *rings, int *hasz)
{
	if (!rings || json_object_get_type(rings) != json_type_array)
		return NULL;

	int nRings = json_object_array_length(rings);

	/* No rings => POLYGON EMPTY */
	if (!nRings)
		return lwpoly_construct_empty(0, 1, 0);

	/* Expecting up to nRings otherwise */
	POINTARRAY **ppa = (POINTARRAY **)lwalloc(sizeof(POINTARRAY *) * nRings);
	int o = 0;

	for (int i = 0; i < nRings; i++)
	{
		json_object *points = json_object_array_get_idx(rings, i);
		if (!points || json_object_get_type(points) != json_type_array)
		{
			for (int k = 0; k < o; k++)
				ptarray_free(ppa[k]);
			lwfree(ppa);
			lwerror("The 'coordinates' in GeoJSON ring are not an array");
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

		ppa[o] = ptarray_construct_empty(1, 0, 1);
		for (int j = 0; j < nPoints; j++)
		{
			json_object *coords = NULL;
			coords = json_object_array_get_idx(points, j);
			if (LW_FAILURE == parse_geojson_coord(coords, hasz, ppa[o]))
			{
				for (int k = 0; k <= o; k++)
					ptarray_free(ppa[k]);
				lwfree(ppa);
				lwerror("The 'coordinates' in GeoJSON are not sufficiently nested");
				return NULL;
			}
		}
		o++;
	}

	/* All the rings were empty! */
	if (!o)
	{
		lwfree(ppa);
		return lwpoly_construct_empty(0, 1, 0);
	}

	return lwpoly_construct(0, NULL, o, ppa);
}

static inline LWGEOM *
parse_geojson_polygon(json_object *geojson, int *hasz)
{
	return (LWGEOM *)parse_geojson_poly_rings(parse_coordinates(geojson), hasz);
}

static inline LWGEOM *
parse_geojson_multipoint(json_object *geojson, int *hasz)
{
	json_object *points = parse_coordinates(geojson);
	if (!points)
		return NULL;
	LWMPOINT *geom = (LWMPOINT *)lwcollection_construct_empty(MULTIPOINTTYPE, 0, 1, 0);

	const int nPoints = json_object_array_length(points);
	for (int i = 0; i < nPoints; ++i)
	{
		POINTARRAY *pa = ptarray_construct_empty(1, 0, 1);
		json_object *coord = json_object_array_get_idx(points, i);
		if (parse_geojson_coord(coord, hasz, pa))
			geom = lwmpoint_add_lwpoint(geom, lwpoint_construct(0, NULL, pa));
		else
		{
			lwmpoint_free(geom);
			ptarray_free(pa);
			return NULL;
		}
	}

	return (LWGEOM *)geom;
}

static inline LWGEOM *
parse_geojson_multilinestring(json_object *geojson, int *hasz)
{
	json_object *mls = parse_coordinates(geojson);
	if (!mls)
		return NULL;
	LWMLINE *geom = (LWMLINE *)lwcollection_construct_empty(MULTILINETYPE, 0, 1, 0);
	const int nLines = json_object_array_length(mls);
	for (int i = 0; i < nLines; ++i)
	{
		POINTARRAY *pa = ptarray_construct_empty(1, 0, 1);
		json_object *coords = json_object_array_get_idx(mls, i);

		if (json_type_array == json_object_get_type(coords))
		{
			const int nPoints = json_object_array_length(coords);
			for (int j = 0; j < nPoints; ++j)
			{
				json_object *coord = json_object_array_get_idx(coords, j);
				if (!parse_geojson_coord(coord, hasz, pa))
				{
					lwmline_free(geom);
					ptarray_free(pa);
					return NULL;
				}
			}
			geom = lwmline_add_lwline(geom, lwline_construct(0, NULL, pa));
		}
		else
		{
			lwmline_free(geom);
			ptarray_free(pa);
			return NULL;
		}
	}
	return (LWGEOM *)geom;
}

static inline LWGEOM *
parse_geojson_multipolygon(json_object *geojson, int *hasz)
{
	json_object *polys = parse_coordinates(geojson);
	if (!polys)
		return NULL;
	LWGEOM *geom = (LWGEOM *)lwcollection_construct_empty(MULTIPOLYGONTYPE, 0, 1, 0);
	int nPolys = json_object_array_length(polys);

	for (int i = 0; i < nPolys; ++i)
	{
		json_object *rings = json_object_array_get_idx(polys, i);
		LWPOLY *poly = parse_geojson_poly_rings(rings, hasz);
		if (poly)
			geom = (LWGEOM *)lwmpoly_add_lwpoly((LWMPOLY *)geom, poly);
	}

	return geom;
}

static inline LWGEOM *
parse_geojson_geometrycollection(json_object *geojson, int *hasz)
{
	json_object *poObjGeoms = findMemberByName(geojson, "geometries");
	if (!poObjGeoms)
	{
		lwerror("Unable to find 'geometries' in GeoJSON string");
		return NULL;
	}
	LWGEOM *geom = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, 0, 1, 0);

	if (json_type_array == json_object_get_type(poObjGeoms))
	{
		const int nGeoms = json_object_array_length(poObjGeoms);
		for (int i = 0; i < nGeoms; ++i)
		{
			json_object *poObjGeom = json_object_array_get_idx(poObjGeoms, i);
			LWGEOM *t = parse_geojson(poObjGeom, hasz);
			if (t)
				geom = (LWGEOM *)lwcollection_add_lwgeom((LWCOLLECTION *)geom, t);
			else
			{
				lwgeom_free(geom);
				return NULL;
			}
		}
	}

	return geom;
}

static inline LWGEOM *
parse_geojson(json_object *geojson, int *hasz)
{
	json_object *type = NULL;
	const char *name;

	if (!geojson)
	{
		lwerror("invalid GeoJSON representation");
		return NULL;
	}

	type = findMemberByName(geojson, "type");
	if (!type)
	{
		lwerror("unknown GeoJSON type");
		return NULL;
	}

	name = json_object_get_string(type);

	if (strcasecmp(name, "Point") == 0)
		return parse_geojson_point(geojson, hasz);

	if (strcasecmp(name, "LineString") == 0)
		return parse_geojson_linestring(geojson, hasz);

	if (strcasecmp(name, "Polygon") == 0)
		return parse_geojson_polygon(geojson, hasz);

	if (strcasecmp(name, "MultiPoint") == 0)
		return parse_geojson_multipoint(geojson, hasz);

	if (strcasecmp(name, "MultiLineString") == 0)
		return parse_geojson_multilinestring(geojson, hasz);

	if (strcasecmp(name, "MultiPolygon") == 0)
		return parse_geojson_multipolygon(geojson, hasz);

	if (strcasecmp(name, "GeometryCollection") == 0)
		return parse_geojson_geometrycollection(geojson, hasz);

	lwerror("invalid GeoJson representation");
	return NULL; /* Never reach */
}

#endif /* HAVE_LIBJSON */

LWGEOM *
lwgeom_from_geojson(const char *geojson, char **srs)
{
#ifndef HAVE_LIBJSON
	*srs = NULL;
	lwerror("You need JSON-C for lwgeom_from_geojson");
	return NULL;
#else  /* HAVE_LIBJSON */

	/* Begin to Parse json */
	json_tokener *jstok = json_tokener_new();
	json_object *poObj = json_tokener_parse_ex(jstok, geojson, -1);
	if (jstok->err != json_tokener_success)
	{
		char err[256];
		snprintf(err, 256, "%s (at offset %d)", json_tokener_error_desc(jstok->err), jstok->char_offset);
		json_tokener_free(jstok);
		json_object_put(poObj);
		lwerror(err);
		return NULL;
	}
	json_tokener_free(jstok);

	*srs = NULL;
	json_object *poObjSrs = findMemberByName(poObj, "crs");
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

	int hasz = LW_FALSE;
	LWGEOM *lwgeom = parse_geojson(poObj, &hasz);
	json_object_put(poObj);
	if (!lwgeom)
		return NULL;

	if (!hasz)
	{
		LWGEOM *tmp = lwgeom_force_2d(lwgeom);
		lwgeom_free(lwgeom);
		lwgeom = tmp;
	}
	lwgeom_add_bbox(lwgeom);
	return lwgeom;
#endif /* HAVE_LIBJSON */
}
