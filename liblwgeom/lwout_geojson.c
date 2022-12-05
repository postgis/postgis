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
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2009-2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 **********************************************************************/


#include "liblwgeom_internal.h"
#include "stringbuffer.h"
#include <string.h>	/* strlen */
#include <assert.h>


typedef struct geojson_opts {
	const char *srs;
	GBOX       *bbox;
	int         precision;
	int         hasz;
	int         isCollectionElement;
} geojson_opts;

enum {
	geojson_tagged,
	geojson_untagged
};

static void asgeojson_geometry(stringbuffer_t *sb, const LWGEOM *geom, const geojson_opts *opts);



static void
coordinate_to_geojson(stringbuffer_t *sb, const POINTARRAY *pa, uint32_t i, const geojson_opts *opts)
{
	if (!FLAGS_GET_Z(pa->flags))
	{
		const POINT2D *pt = getPoint2d_cp(pa, i);
		stringbuffer_append_char(sb, '[');
		stringbuffer_append_double(sb, pt->x, opts->precision);
		stringbuffer_append_char(sb, ',');
		stringbuffer_append_double(sb, pt->y, opts->precision);
		stringbuffer_append_char(sb, ']');
	}
	else
	{
		const POINT3D *pt = getPoint3d_cp(pa, i);
		stringbuffer_append_char(sb, '[');
		stringbuffer_append_double(sb, pt->x, opts->precision);
		stringbuffer_append_char(sb, ',');
		stringbuffer_append_double(sb, pt->y, opts->precision);
		stringbuffer_append_char(sb, ',');
		stringbuffer_append_double(sb, pt->z, opts->precision);
		stringbuffer_append_char(sb, ']');
	}
}

static void
pointArray_to_geojson(stringbuffer_t *sb, const POINTARRAY *pa, const geojson_opts *opts)
{
	if (!pa || pa->npoints == 0)
	{
		stringbuffer_append_len(sb, "[]", 2);
		return;
	}

	stringbuffer_append_char(sb, '[');
	for (uint32_t i = 0; i < pa->npoints; i++)
	{
		if (i) stringbuffer_append_char(sb, ',');
		coordinate_to_geojson(sb, pa, i, opts);
	}
	stringbuffer_append_char(sb, ']');
	return;
}

static void
asgeojson_srs(stringbuffer_t *sb, const geojson_opts *opts)
{
	if (!opts->srs) return;
	stringbuffer_append_len(sb, "\"crs\":{\"type\":\"name\",", 21);
	stringbuffer_aprintf(sb, "\"properties\":{\"name\":\"%s\"}},", opts->srs);
	return;
}


static void
asgeojson_bbox(stringbuffer_t *sb, const geojson_opts *opts)
{
	if (!opts->bbox) return;
	if (!opts->hasz)
		stringbuffer_aprintf(sb, "\"bbox\":[%.*f,%.*f,%.*f,%.*f],",
			opts->precision, opts->bbox->xmin,
			opts->precision, opts->bbox->ymin,
			opts->precision, opts->bbox->xmax,
			opts->precision, opts->bbox->ymax);
	else
		stringbuffer_aprintf(sb, "\"bbox\":[%.*f,%.*f,%.*f,%.*f,%.*f,%.*f],",
			opts->precision, opts->bbox->xmin,
			opts->precision, opts->bbox->ymin,
			opts->precision, opts->bbox->zmin,
			opts->precision, opts->bbox->xmax,
			opts->precision, opts->bbox->ymax,
			opts->precision, opts->bbox->zmax);
	return;
}


/**
 * Point Geometry
 */
static void
asgeojson_point_coords(stringbuffer_t *sb, const LWPOINT *point, const geojson_opts *opts, int tagged)
{
	if (tagged == geojson_tagged) stringbuffer_append_len(sb, "\"coordinates\":", 14);
	if (lwgeom_is_empty((LWGEOM*)point))
		stringbuffer_append_len(sb, "[]", 2);
	else
		coordinate_to_geojson(sb, point->point, 0, opts);
	return;
}

static void
asgeojson_line_coords(stringbuffer_t *sb, const LWLINE *line, const geojson_opts *opts, int tagged)
{
	if (tagged == geojson_tagged) stringbuffer_append_len(sb, "\"coordinates\":", 14);
	if (lwgeom_is_empty((LWGEOM*)line))
		stringbuffer_append_len(sb, "[]", 2);
	else
		pointArray_to_geojson(sb, line->points, opts);
	return;
}

static void
asgeojson_poly_coords(stringbuffer_t *sb, const LWPOLY *poly, const geojson_opts *opts, int tagged)
{
	uint32_t i;
	if (tagged == geojson_tagged) stringbuffer_append_len(sb, "\"coordinates\":", 14);
	if (lwgeom_is_empty((LWGEOM*)poly))
		stringbuffer_append_len(sb, "[]", 2);
	else
	{
		stringbuffer_append_char(sb, '[');
		for (i = 0; i < poly->nrings; i++)
		{
			if (i) stringbuffer_append_char(sb, ',');
			pointArray_to_geojson(sb, poly->rings[i], opts);
		}
		stringbuffer_append_char(sb, ']');
	}
	return;
}

/**
 * Point Geometry
 */
static void
asgeojson_point(stringbuffer_t *sb, const LWPOINT *point, const geojson_opts *opts)
{
	stringbuffer_append_len(sb, "{\"type\":\"Point\",", 16);
	asgeojson_srs(sb, opts);
	asgeojson_bbox(sb, opts);
	asgeojson_point_coords(sb, point, opts, geojson_tagged);
	stringbuffer_append_char(sb, '}');
	return;
}

/**
 * Triangle Geometry
 */
static void
asgeojson_triangle(stringbuffer_t *sb, const LWTRIANGLE *tri, const geojson_opts *opts)
{
	stringbuffer_append_len(sb, "{\"type\":\"Polygon\",", 18);
	asgeojson_srs(sb, opts);
	asgeojson_bbox(sb, opts);
	stringbuffer_append_len(sb, "\"coordinates\":[", 15);
	if (lwtriangle_is_empty(tri))
		stringbuffer_append_len(sb, "[]", 2);
	else
		pointArray_to_geojson(sb, tri->points, opts);
	stringbuffer_append_len(sb, "]}", 2);
	return;
}

/**
 * Line Geometry
 */
static void
asgeojson_line(stringbuffer_t *sb, const LWLINE *line, const geojson_opts *opts)
{
	const char tmpl[] = "{\"type\":\"LineString\",";
	stringbuffer_append_len(sb, tmpl, sizeof(tmpl)-1);
	asgeojson_srs(sb, opts);
	asgeojson_bbox(sb, opts);
	asgeojson_line_coords(sb, line, opts, geojson_tagged);
	stringbuffer_append_char(sb, '}');
	return;
}

/**
 * Polygon Geometry
 */
static void
asgeojson_poly(stringbuffer_t *sb, const LWPOLY *poly, const geojson_opts *opts)
{
	stringbuffer_append_len(sb, "{\"type\":\"Polygon\",", 18);
	asgeojson_srs(sb, opts);
	asgeojson_bbox(sb, opts);
	asgeojson_poly_coords(sb, poly, opts, geojson_tagged);
	stringbuffer_append_char(sb, '}');
	return;
}

/**
 * Multipoint Geometry
 */
static void
asgeojson_multipoint(stringbuffer_t *sb, const LWMPOINT *mpoint, const geojson_opts *opts)
{
	uint32_t i, ngeoms = mpoint->ngeoms;
	stringbuffer_append_len(sb, "{\"type\":\"MultiPoint\",", 21);
	asgeojson_srs(sb, opts);
	asgeojson_bbox(sb, opts);
	stringbuffer_append_len(sb, "\"coordinates\":[", 15);

	if (lwgeom_is_empty((LWGEOM*)mpoint))
		ngeoms = 0;

	for (i=0; i < ngeoms; i++)
	{
		if (i) stringbuffer_append_char(sb, ',');
		asgeojson_point_coords(sb, mpoint->geoms[i], opts, geojson_untagged);
	}
	stringbuffer_append_len(sb, "]}", 2);
	return;
}


/**
 * Multipoint Geometry
 */
static void
asgeojson_multiline(stringbuffer_t *sb, const LWMLINE *mline, const geojson_opts *opts)
{
	uint32_t i, ngeoms = mline->ngeoms;
	stringbuffer_append_len(sb, "{\"type\":\"MultiLineString\",", 26);
	asgeojson_srs(sb, opts);
	asgeojson_bbox(sb, opts);
	stringbuffer_append_len(sb, "\"coordinates\":[", 15);

	if (lwgeom_is_empty((LWGEOM*)mline))
		ngeoms = 0;

	for (i=0; i < ngeoms; i++)
	{
		if (i) stringbuffer_append_char(sb, ',');
		asgeojson_line_coords(sb, mline->geoms[i], opts, geojson_untagged);
	}
	stringbuffer_append_len(sb, "]}", 2);
	return;
}


static void
asgeojson_multipolygon(stringbuffer_t *sb, const LWMPOLY *mpoly, const geojson_opts *opts)
{
	uint32_t i, ngeoms = mpoly->ngeoms;

	stringbuffer_append_len(sb, "{\"type\":\"MultiPolygon\",", 23);
	asgeojson_srs(sb, opts);
	asgeojson_bbox(sb, opts);
	stringbuffer_append_len(sb, "\"coordinates\":[", 15);

	if (lwgeom_is_empty((LWGEOM*)mpoly))
		ngeoms = 0;

	for (i=0; i < ngeoms; i++)
	{
		if (i) stringbuffer_append_char(sb, ',');
		asgeojson_poly_coords(sb, mpoly->geoms[i], opts, geojson_untagged);
	}
	stringbuffer_append_len(sb, "]}", 2);
	return;
}

/**
 * Collection Geometry
 */
static void
asgeojson_collection(stringbuffer_t *sb, const LWCOLLECTION *col, const geojson_opts *opts)
{
	uint32_t i, ngeoms = col->ngeoms;

	/* subgeometries don't get boxes or srs */
	geojson_opts subopts = *opts;
	subopts.bbox = NULL;
	subopts.srs = NULL;
	subopts.isCollectionElement = LW_TRUE;

	stringbuffer_append_len(sb, "{\"type\":\"GeometryCollection\",", 29);
	asgeojson_srs(sb, opts);
	if (col->ngeoms) asgeojson_bbox(sb, opts);
	stringbuffer_append_len(sb, "\"geometries\":[", 14);

	if (lwgeom_is_empty((LWGEOM*)col))
		ngeoms = 0;

	for (i=0; i<ngeoms; i++)
	{
		if (i) stringbuffer_append_char(sb, ',');
		asgeojson_geometry(sb, col->geoms[i], &subopts);
	}

	stringbuffer_append_len(sb, "]}", 2);
	return;
}

static void
asgeojson_geometry(stringbuffer_t *sb, const LWGEOM *geom, const geojson_opts *opts)
{
	switch (geom->type)
	{
	case POINTTYPE:
		asgeojson_point(sb, (LWPOINT*)geom, opts);
		break;
	case LINETYPE:
		asgeojson_line(sb, (LWLINE*)geom, opts);
		break;
	case POLYGONTYPE:
		asgeojson_poly(sb, (LWPOLY*)geom, opts);
		break;
	case MULTIPOINTTYPE:
		asgeojson_multipoint(sb, (LWMPOINT*)geom, opts);
		break;
	case MULTILINETYPE:
		asgeojson_multiline(sb, (LWMLINE*)geom, opts);
		break;
	case MULTIPOLYGONTYPE:
		asgeojson_multipolygon(sb, (LWMPOLY*)geom, opts);
		break;
	case TRIANGLETYPE:
		asgeojson_triangle(sb, (LWTRIANGLE *)geom, opts);
		break;
	case TINTYPE:
	case COLLECTIONTYPE:
		if (opts->isCollectionElement) {
			lwerror("GeoJson: geometry not supported.");
		}
		asgeojson_collection(sb, (LWCOLLECTION*)geom, opts);
		break;
	default:
		lwerror("lwgeom_to_geojson: '%s' geometry type not supported", lwtype_name(geom->type));
	}
}

/**
 * Takes a GEOMETRY and returns a GeoJson representation
 */
lwvarlena_t *
lwgeom_to_geojson(const LWGEOM *geom, const char *srs, int precision, int has_bbox)
{
	GBOX static_bbox = {0};
	geojson_opts opts;
	stringbuffer_t sb;

	memset(&opts, 0, sizeof(opts));
	opts.precision = precision;
	opts.hasz = FLAGS_GET_Z(geom->flags);
	opts.srs = srs;

	if (has_bbox)
	{
		/* Whether these are geography or geometry,
		   the GeoJSON expects a cartesian bounding box */
		lwgeom_calculate_gbox_cartesian(geom, &static_bbox);
		opts.bbox = &static_bbox;
	}

	/* To avoid taking a copy of the output, we make */
	/* space for the VARLENA header before starting to */
	/* serialize the geom */
	stringbuffer_init_varlena(&sb);
	/* Now serialize the geometry */
	asgeojson_geometry(&sb, geom, &opts);
	/* Leave the initially allocated buffer in place */
	/* and write the varlena_t metadata into the slot we */
	/* left at the start */
	return stringbuffer_getvarlena(&sb);
}
