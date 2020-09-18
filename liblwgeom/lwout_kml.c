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
 * Copyright 2006 Corporacion Autonoma Regional de Santander
 * Copyright 2010 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/


#include "liblwgeom_internal.h"
#include "stringbuffer.h"

static int lwgeom_to_kml2_sb(const LWGEOM *geom, int precision, const char *prefix, stringbuffer_t *sb);
static int lwpoint_to_kml2_sb(const LWPOINT *point, int precision, const char *prefix, stringbuffer_t *sb);
static int lwline_to_kml2_sb(const LWLINE *line, int precision, const char *prefix, stringbuffer_t *sb);
static int lwtriangle_to_kml2_sb(const LWTRIANGLE *tri, int precision, const char *prefix, stringbuffer_t *sb);
static int lwpoly_to_kml2_sb(const LWPOLY *poly, int precision, const char *prefix, stringbuffer_t *sb);
static int lwcollection_to_kml2_sb(const LWCOLLECTION *col, int precision, const char *prefix, stringbuffer_t *sb);
static int ptarray_to_kml2_sb(const POINTARRAY *pa, int precision, stringbuffer_t *sb);

/*
* KML 2.2.0
*/

/* takes a GEOMETRY and returns a KML representation */
lwvarlena_t *
lwgeom_to_kml2(const LWGEOM *geom, int precision, const char *prefix)
{
	stringbuffer_t *sb;
	int rv;

	/* Can't do anything with empty */
	if( lwgeom_is_empty(geom) )
		return NULL;

	sb = stringbuffer_create();
	rv = lwgeom_to_kml2_sb(geom, precision, prefix, sb);

	if ( rv == LW_FAILURE )
	{
		stringbuffer_destroy(sb);
		return NULL;
	}

	lwvarlena_t *v = stringbuffer_getvarlenacopy(sb);
	stringbuffer_destroy(sb);

	return v;
}

static int
lwgeom_to_kml2_sb(const LWGEOM *geom, int precision, const char *prefix, stringbuffer_t *sb)
{
	switch (geom->type)
	{
	case POINTTYPE:
		return lwpoint_to_kml2_sb((LWPOINT*)geom, precision, prefix, sb);

	case LINETYPE:
		return lwline_to_kml2_sb((LWLINE*)geom, precision, prefix, sb);

	case TRIANGLETYPE:
		return lwtriangle_to_kml2_sb((LWTRIANGLE *)geom, precision, prefix, sb);

	case POLYGONTYPE:
		return lwpoly_to_kml2_sb((LWPOLY*)geom, precision, prefix, sb);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case TINTYPE:
		return lwcollection_to_kml2_sb((LWCOLLECTION*)geom, precision, prefix, sb);

	default:
		lwerror("lwgeom_to_kml2: '%s' geometry type not supported", lwtype_name(geom->type));
		return LW_FAILURE;
	}
}

static int
ptarray_to_kml2_sb(const POINTARRAY *pa, int precision, stringbuffer_t *sb)
{
	uint32_t i, j;
	uint32_t dims = FLAGS_GET_Z(pa->flags) ? 3 : 2;
	POINT4D pt;
	double *d;

	for ( i = 0; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &pt);
		d = (double*)(&pt);
		if ( i ) stringbuffer_append_len(sb," ",1);
		for (j = 0; j < dims; j++)
		{
			if ( j ) stringbuffer_append_len(sb,",",1);
			stringbuffer_append_double(sb, d[j], precision);
		}
	}
	return LW_SUCCESS;
}


static int
lwpoint_to_kml2_sb(const LWPOINT *point, int precision, const char *prefix, stringbuffer_t *sb)
{
	/* Open point */
	if ( stringbuffer_aprintf(sb, "<%sPoint><%scoordinates>", prefix, prefix) < 0 ) return LW_FAILURE;
	/* Coordinate array */
	if ( ptarray_to_kml2_sb(point->point, precision, sb) == LW_FAILURE ) return LW_FAILURE;
	/* Close point */
	if ( stringbuffer_aprintf(sb, "</%scoordinates></%sPoint>", prefix, prefix) < 0 ) return LW_FAILURE;
	return LW_SUCCESS;
}

static int
lwline_to_kml2_sb(const LWLINE *line, int precision, const char *prefix, stringbuffer_t *sb)
{
	/* Open linestring */
	if ( stringbuffer_aprintf(sb, "<%sLineString><%scoordinates>", prefix, prefix) < 0 ) return LW_FAILURE;
	/* Coordinate array */
	if ( ptarray_to_kml2_sb(line->points, precision, sb) == LW_FAILURE ) return LW_FAILURE;
	/* Close linestring */
	if ( stringbuffer_aprintf(sb, "</%scoordinates></%sLineString>", prefix, prefix) < 0 ) return LW_FAILURE;

	return LW_SUCCESS;
}

static int
lwtriangle_to_kml2_sb(const LWTRIANGLE *tri, int precision, const char *prefix, stringbuffer_t *sb)
{
	/* Open polygon */
	if (stringbuffer_aprintf(
		sb, "<%sPolygon><%souterBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix, prefix) < 0)
		return LW_FAILURE;
	/* Coordinate array */
	if (ptarray_to_kml2_sb(tri->points, precision, sb) == LW_FAILURE)
		return LW_FAILURE;
	/* Close polygon */
	if (stringbuffer_aprintf(
		sb, "</%scoordinates></%sLinearRing></%souterBoundaryIs></%sPolygon>", prefix, prefix, prefix, prefix) <
	    0)
		return LW_FAILURE;

	return LW_SUCCESS;
}

static int
lwpoly_to_kml2_sb(const LWPOLY *poly, int precision, const char *prefix, stringbuffer_t *sb)
{
	uint32_t i;
	int rv;

	/* Open polygon */
	if ( stringbuffer_aprintf(sb, "<%sPolygon>", prefix) < 0 ) return LW_FAILURE;
	for ( i = 0; i < poly->nrings; i++ )
	{
		/* Inner or outer ring opening tags */
		if( i )
			rv = stringbuffer_aprintf(sb, "<%sinnerBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix);
		else
			rv = stringbuffer_aprintf(sb, "<%souterBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix);
		if ( rv < 0 ) return LW_FAILURE;

		/* Coordinate array */
		if ( ptarray_to_kml2_sb(poly->rings[i], precision, sb) == LW_FAILURE ) return LW_FAILURE;

		/* Inner or outer ring closing tags */
		if( i )
			rv = stringbuffer_aprintf(sb, "</%scoordinates></%sLinearRing></%sinnerBoundaryIs>", prefix, prefix, prefix);
		else
			rv = stringbuffer_aprintf(sb, "</%scoordinates></%sLinearRing></%souterBoundaryIs>", prefix, prefix, prefix);
		if ( rv < 0 ) return LW_FAILURE;
	}
	/* Close polygon */
	if ( stringbuffer_aprintf(sb, "</%sPolygon>", prefix) < 0 ) return LW_FAILURE;

	return LW_SUCCESS;
}

static int
lwcollection_to_kml2_sb(const LWCOLLECTION *col, int precision, const char *prefix, stringbuffer_t *sb)
{
	uint32_t i;
	int rv;

	/* Open geometry */
	if ( stringbuffer_aprintf(sb, "<%sMultiGeometry>", prefix) < 0 ) return LW_FAILURE;
	for ( i = 0; i < col->ngeoms; i++ )
	{
		rv = lwgeom_to_kml2_sb(col->geoms[i], precision, prefix, sb);
		if ( rv == LW_FAILURE ) return LW_FAILURE;
	}
	/* Close geometry */
	if ( stringbuffer_aprintf(sb, "</%sMultiGeometry>", prefix) < 0 ) return LW_FAILURE;

	return LW_SUCCESS;
}
