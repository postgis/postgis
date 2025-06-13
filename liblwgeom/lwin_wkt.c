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
 * Copyright (C) 2010 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/


#include <stdlib.h>
#include <ctype.h> /* for isspace */

#include "lwin_wkt.h"
#include "lwin_wkt_parse.h"
#include "lwgeom_log.h"


/*
* Error messages for failures in the parser.
*/
const char *parser_error_messages[] =
{
	"",
	"geometry requires more points",
	"geometry must have an odd number of points",
	"geometry contains non-closed rings",
	"can not mix dimensionality in a geometry",
	"parse error - invalid geometry",
	"invalid WKB type",
	"incontinuous compound curve",
	"triangle must have exactly 4 points",
	"geometry has too many points",
	"parse error - invalid geometry"
};

#define SET_PARSER_ERROR(errno) { \
		global_parser_result.message = parser_error_messages[(errno)]; \
		global_parser_result.errcode = (errno); \
		global_parser_result.errlocation = wkt_yylloc.last_column; \
	}

/**
* Read the SRID number from an SRID=<> string
*/
int wkt_lexer_read_srid(char *str)
{
	char *c = str;
	long i = 0;
	int32_t srid;

	if( ! str ) return SRID_UNKNOWN;
	c += 5; /* Advance past "SRID=" */
	i = strtol(c, NULL, 10);
	srid = clamp_srid((int32_t)i);
	/* TODO: warn on explicit UNKNOWN srid ? */
	return srid;
}

static lwflags_t wkt_dimensionality(char *dimensionality)
{
	size_t i = 0;
	lwflags_t flags = 0;

	if( ! dimensionality )
		return flags;

	/* If there's an explicit dimensionality, we use that */
	for( i = 0; i < strlen(dimensionality); i++ )
	{
		if( (dimensionality[i] == 'Z') || (dimensionality[i] == 'z') )
			FLAGS_SET_Z(flags,1);
		else if( (dimensionality[i] == 'M') || (dimensionality[i] == 'm') )
			FLAGS_SET_M(flags,1);
		/* only a space is accepted in between */
		else if( ! isspace(dimensionality[i]) ) break;
	}
	return flags;
}


/**
* Force the dimensionality of a geometry to match the dimensionality
* of a set of flags (usually derived from a ZM WKT tag).
*/
static int wkt_parser_set_dims(LWGEOM *geom, lwflags_t flags)
{
	int hasz = FLAGS_GET_Z(flags);
	int hasm = FLAGS_GET_M(flags);
	uint32_t i = 0;

	/* Error on junk */
	if( ! geom )
		return LW_FAILURE;

	FLAGS_SET_Z(geom->flags, hasz);
	FLAGS_SET_M(geom->flags, hasm);

	switch( geom->type )
	{
		case POINTTYPE:
		{
			LWPOINT *pt = (LWPOINT*)geom;
			if ( pt->point )
			{
				FLAGS_SET_Z(pt->point->flags, hasz);
				FLAGS_SET_M(pt->point->flags, hasm);
			}
			break;
		}
		case TRIANGLETYPE:
		case CIRCSTRINGTYPE:
		case LINETYPE:
		{
			LWLINE *ln = (LWLINE*)geom;
			if ( ln->points )
			{
				FLAGS_SET_Z(ln->points->flags, hasz);
				FLAGS_SET_M(ln->points->flags, hasm);
			}
			break;
		}
		case POLYGONTYPE:
		{
			LWPOLY *poly = (LWPOLY*)geom;
			for ( i = 0; i < poly->nrings; i++ )
			{
				if( poly->rings[i] )
				{
					FLAGS_SET_Z(poly->rings[i]->flags, hasz);
					FLAGS_SET_M(poly->rings[i]->flags, hasm);
				}
			}
			break;
		}
		case CURVEPOLYTYPE:
		{
			LWCURVEPOLY *poly = (LWCURVEPOLY*)geom;
			for ( i = 0; i < poly->nrings; i++ )
				wkt_parser_set_dims(poly->rings[i], flags);
			break;
		}
		default:
		{
			if ( lwtype_is_collection(geom->type) )
			{
				LWCOLLECTION *col = (LWCOLLECTION*)geom;
				for ( i = 0; i < col->ngeoms; i++ )
					wkt_parser_set_dims(col->geoms[i], flags);
				return LW_SUCCESS;
			}
			else
			{
				LWDEBUGF(2,"Unknown geometry type: %d", geom->type);
				return LW_FAILURE;
			}
		}
	}

	return LW_SUCCESS;
}

/**
* Read the dimensionality from a flag, if provided. Then check that the
* dimensionality matches that of the pointarray. If the dimension counts
* match, ensure the pointarray is using the right "Z" or "M".
*/
static int wkt_pointarray_dimensionality(POINTARRAY *pa, lwflags_t flags)
{
	int hasz = FLAGS_GET_Z(flags);
	int hasm = FLAGS_GET_M(flags);
	int ndims = 2 + hasz + hasm;

	/* No dimensionality or array means we go with what we have */
	if( ! (flags && pa) )
		return LW_TRUE;

	LWDEBUGF(5,"dimensionality ndims == %d", ndims);
	LWDEBUGF(5,"FLAGS_NDIMS(pa->flags) == %d", FLAGS_NDIMS(pa->flags));

	/*
	* ndims > 2 implies that the flags have something useful to add,
	* that there is a 'Z' or an 'M' or both.
	*/
	if( ndims > 2 )
	{
		/* Mismatch implies a problem */
		if ( FLAGS_NDIMS(pa->flags) != ndims )
			return LW_FALSE;
		/* Match means use the explicit dimensionality */
		else
		{
			FLAGS_SET_Z(pa->flags, hasz);
			FLAGS_SET_M(pa->flags, hasm);
		}
	}

	return LW_TRUE;
}



/**
* Build a 2d coordinate.
*/
POINT wkt_parser_coord_2(double c1, double c2)
{
	POINT p;
	p.flags = 0;
	p.x = c1;
	p.y = c2;
	p.z = p.m = 0.0;
	FLAGS_SET_Z(p.flags, 0);
	FLAGS_SET_M(p.flags, 0);
	return p;
}

/**
* Note, if this is an XYM coordinate we'll have to fix it later when we build
* the object itself and have access to the dimensionality token.
*/
POINT wkt_parser_coord_3(double c1, double c2, double c3)
{
		POINT p;
		p.flags = 0;
		p.x = c1;
		p.y = c2;
		p.z = c3;
		p.m = 0;
		FLAGS_SET_Z(p.flags, 1);
		FLAGS_SET_M(p.flags, 0);
		return p;
}

/**
*/
POINT wkt_parser_coord_4(double c1, double c2, double c3, double c4)
{
	POINT p;
	p.flags = 0;
	p.x = c1;
	p.y = c2;
	p.z = c3;
	p.m = c4;
	FLAGS_SET_Z(p.flags, 1);
	FLAGS_SET_M(p.flags, 1);
	return p;
}

POINTARRAY* wkt_parser_ptarray_add_coord(POINTARRAY *pa, POINT p)
{
	POINT4D pt;
	LWDEBUG(4,"entered");

	/* Error on trouble */
	if( ! pa )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* Check that the coordinate has the same dimensionality as the array */
	if( FLAGS_NDIMS(p.flags) != FLAGS_NDIMS(pa->flags) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* While parsing the point arrays, XYM and XMZ points are both treated as XYZ */
	pt.x = p.x;
	pt.y = p.y;
	if( FLAGS_GET_Z(pa->flags) )
		pt.z = p.z;
	if( FLAGS_GET_M(pa->flags) )
		pt.m = p.m;
	/* If the destination is XYM, we'll write the third coordinate to m */
	if( FLAGS_GET_M(pa->flags) && ! FLAGS_GET_Z(pa->flags) )
		pt.m = p.z;

	ptarray_append_point(pa, &pt, LW_TRUE); /* Allow duplicate points in array */
	return pa;
}

/**
* Start a point array from the first coordinate.
*/
POINTARRAY* wkt_parser_ptarray_new(POINT p)
{
	int ndims = FLAGS_NDIMS(p.flags);
	POINTARRAY *pa = ptarray_construct_empty((ndims>2), (ndims>3), 4);
	LWDEBUG(4,"entered");
	if ( ! pa )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	return wkt_parser_ptarray_add_coord(pa, p);
}

/**
* Create a new point. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT.
*/
LWGEOM* wkt_parser_point_new(POINTARRAY *pa, char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	LWDEBUG(4,"entered");

	/* No pointarray means it is empty */
	if( ! pa )
		return lwpoint_as_lwgeom(lwpoint_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == LW_FALSE )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Only one point allowed in our point array! */
	if( pa->npoints != 1 )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_LESSPOINTS);
		return NULL;
	}

	return lwpoint_as_lwgeom(lwpoint_construct(SRID_UNKNOWN, NULL, pa));
}


/**
* Create a new linestring. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT. Check for numpoints >= 2 if
* requested.
*/
LWGEOM* wkt_parser_linestring_new(POINTARRAY *pa, char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	LWDEBUG(4,"entered");

	/* No pointarray means it is empty */
	if( ! pa )
		return lwline_as_lwgeom(lwline_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == LW_FALSE )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Apply check for not enough points, if requested. */
	if( (global_parser_result.parser_check_flags & LW_PARSER_CHECK_MINPOINTS) && (pa->npoints < 2) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}

	return lwline_as_lwgeom(lwline_construct(SRID_UNKNOWN, NULL, pa));
}

/**
* Create a new circularstring. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT.
* Circular strings are just like linestrings, except with slightly different
* validity rules (minpoint == 3, numpoints % 2 == 1).
*/
LWGEOM* wkt_parser_circularstring_new(POINTARRAY *pa, char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	LWDEBUG(4,"entered");

	/* No pointarray means it is empty */
	if( ! pa )
		return lwcircstring_as_lwgeom(lwcircstring_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == LW_FALSE )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Apply check for not enough points, if requested. */
	if( (global_parser_result.parser_check_flags & LW_PARSER_CHECK_MINPOINTS) && (pa->npoints < 3) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}

	/* Apply check for odd number of points, if requested. */
	if( (global_parser_result.parser_check_flags & LW_PARSER_CHECK_ODD) && ((pa->npoints % 2) == 0) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_ODDPOINTS);
		return NULL;
	}

	return lwcircstring_as_lwgeom(lwcircstring_construct(SRID_UNKNOWN, NULL, pa));
}

LWGEOM* wkt_parser_triangle_new(POINTARRAY *pa, char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	LWDEBUG(4,"entered");

	/* No pointarray means it is empty */
	if( ! pa )
		return lwtriangle_as_lwgeom(lwtriangle_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == LW_FALSE )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Triangles need four points. */
	if( (pa->npoints != 4) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_TRIANGLEPOINTS);
		return NULL;
	}

	/* Triangles need closure. */
	if( ! ptarray_is_closed_z(pa) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_UNCLOSED);
		return NULL;
	}

	return lwtriangle_as_lwgeom(lwtriangle_construct(SRID_UNKNOWN, NULL, pa));
}

LWGEOM* wkt_parser_polygon_new(POINTARRAY *pa, char dimcheck)
{
	LWPOLY *poly = NULL;
	LWDEBUG(4,"entered");

	/* No pointarray is a problem */
	if( ! pa )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	poly = lwpoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(pa->flags), FLAGS_GET_M(pa->flags));

	/* Error out if we can't build this polygon. */
	if( ! poly )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	wkt_parser_polygon_add_ring(lwpoly_as_lwgeom(poly), pa, dimcheck);
	return lwpoly_as_lwgeom(poly);
}

LWGEOM* wkt_parser_polygon_add_ring(LWGEOM *poly, POINTARRAY *pa, char dimcheck)
{
	LWDEBUG(4,"entered");

	/* Bad inputs are a problem */
	if( ! (pa && poly) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* Rings must agree on dimensionality */
	if( FLAGS_NDIMS(poly->flags) != FLAGS_NDIMS(pa->flags) )
	{
		ptarray_free(pa);
		lwgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Apply check for minimum number of points, if requested. */
	if( (global_parser_result.parser_check_flags & LW_PARSER_CHECK_MINPOINTS) && (pa->npoints < 4) )
	{
		ptarray_free(pa);
		lwgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}

	/* Apply check for not closed rings, if requested. */
	if( (global_parser_result.parser_check_flags & LW_PARSER_CHECK_CLOSURE) &&
	    ! (dimcheck == 'Z' ? ptarray_is_closed_z(pa) : ptarray_is_closed_2d(pa)) )
	{
		ptarray_free(pa);
		lwgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_UNCLOSED);
		return NULL;
	}

	/* If something goes wrong adding a ring, error out. */
	if ( LW_FAILURE == lwpoly_add_ring(lwgeom_as_lwpoly(poly), pa) )
	{
		ptarray_free(pa);
		lwgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	return poly;
}

LWGEOM* wkt_parser_polygon_finalize(LWGEOM *poly, char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	int flagdims = FLAGS_NDIMS(flags);
	LWDEBUG(4,"entered");

	/* Null input implies empty return */
	if( ! poly )
		return lwpoly_as_lwgeom(lwpoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions are not consistent, we have a problem. */
	if( flagdims > 2 )
	{
		if ( flagdims != FLAGS_NDIMS(poly->flags) )
		{
			lwgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
			return NULL;
		}

		/* Harmonize the flags in the sub-components with the wkt flags */
		if( LW_FAILURE == wkt_parser_set_dims(poly, flags) )
		{
			lwgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}
	}

	return poly;
}

LWGEOM* wkt_parser_curvepolygon_new(LWGEOM *ring)
{
	LWGEOM *poly;
	LWDEBUG(4,"entered");

	/* Toss error on null geometry input */
	if( ! ring )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* Construct poly and add the ring. */
	poly = lwcurvepoly_as_lwgeom(lwcurvepoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(ring->flags), FLAGS_GET_M(ring->flags)));
	/* Return the result. */
	return wkt_parser_curvepolygon_add_ring(poly,ring);
}

LWGEOM* wkt_parser_curvepolygon_add_ring(LWGEOM *poly, LWGEOM *ring)
{
	LWDEBUG(4,"entered");

	/* Toss error on null input */
	if( ! (ring && poly) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		LWDEBUG(4,"inputs are null");
		return NULL;
	}

	/* All the elements must agree on dimensionality */
	if( FLAGS_NDIMS(poly->flags) != FLAGS_NDIMS(ring->flags) )
	{
		LWDEBUG(4,"dimensionality does not match");
		lwgeom_free(ring);
		lwgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Apply check for minimum number of points, if requested. */
	if( (global_parser_result.parser_check_flags & LW_PARSER_CHECK_MINPOINTS) )
	{
		uint32_t vertices_needed = 3;

		if ( ring->type == LINETYPE )
			vertices_needed = 4;

		if (lwgeom_count_vertices(ring) < vertices_needed)
		{
			LWDEBUG(4,"number of points is incorrect");
			lwgeom_free(ring);
			lwgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
			return NULL;
		}
	}

	/* Apply check for not closed rings, if requested. */
	if( (global_parser_result.parser_check_flags & LW_PARSER_CHECK_CLOSURE) )
	{
		int is_closed = 1;
		LWDEBUG(4,"checking ring closure");
		switch ( ring->type )
		{
			case LINETYPE:
			is_closed = lwline_is_closed(lwgeom_as_lwline(ring));
			break;

			case CIRCSTRINGTYPE:
			is_closed = lwcircstring_is_closed(lwgeom_as_lwcircstring(ring));
			break;

			case COMPOUNDTYPE:
			is_closed = lwcompound_is_closed(lwgeom_as_lwcompound(ring));
			break;
		}
		if ( ! is_closed )
		{
			LWDEBUG(4,"ring is not closed");
			lwgeom_free(ring);
			lwgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_UNCLOSED);
			return NULL;
		}
	}

	if( LW_FAILURE == lwcurvepoly_add_ring(lwgeom_as_lwcurvepoly(poly), ring) )
	{
		LWDEBUG(4,"failed to add ring");
		lwgeom_free(ring);
		lwgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	return poly;
}

LWGEOM* wkt_parser_curvepolygon_finalize(LWGEOM *poly, char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	int flagdims = FLAGS_NDIMS(flags);
	LWDEBUG(4,"entered");

	/* Null input implies empty return */
	if( ! poly )
		return lwcurvepoly_as_lwgeom(lwcurvepoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	if ( flagdims > 2 )
	{
		/* If the number of dimensions are not consistent, we have a problem. */
		if( flagdims != FLAGS_NDIMS(poly->flags) )
		{
			lwgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
			return NULL;
		}

		/* Harmonize the flags in the sub-components with the wkt flags */
		if( LW_FAILURE == wkt_parser_set_dims(poly, flags) )
		{
			lwgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}
	}

	return poly;
}

LWGEOM* wkt_parser_collection_new(LWGEOM *geom)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	static int ngeoms = 1;
	LWDEBUG(4,"entered");

	/* Toss error on null geometry input */
	if( ! geom )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* Create our geometry array */
	geoms = lwalloc(sizeof(LWGEOM*) * ngeoms);
	geoms[0] = geom;

	/* Make a new collection */
	col = lwcollection_construct(COLLECTIONTYPE, SRID_UNKNOWN, NULL, ngeoms, geoms);

	/* Return the result. */
	return lwcollection_as_lwgeom(col);
}


LWGEOM* wkt_parser_compound_new(LWGEOM *geom)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	static int ngeoms = 1;
	LWDEBUG(4,"entered");

	/* Toss error on null geometry input */
	if( ! geom )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* Elements of a compoundcurve cannot be empty, because */
	/* empty things can't join up and form a ring */
	if ( lwgeom_is_empty(geom) )
	{
		lwgeom_free(geom);
		SET_PARSER_ERROR(PARSER_ERROR_INCONTINUOUS);
		return NULL;
	}

	/* Create our geometry array */
	geoms = lwalloc(sizeof(LWGEOM*) * ngeoms);
	geoms[0] = geom;

	/* Make a new collection */
	col = lwcollection_construct(COLLECTIONTYPE, SRID_UNKNOWN, NULL, ngeoms, geoms);

	/* Return the result. */
	return lwcollection_as_lwgeom(col);
}


LWGEOM* wkt_parser_compound_add_geom(LWGEOM *col, LWGEOM *geom)
{
	LWDEBUG(4,"entered");

	/* Toss error on null geometry input */
	if( ! (geom && col) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* All the elements must agree on dimensionality */
	if( FLAGS_NDIMS(col->flags) != FLAGS_NDIMS(geom->flags) )
	{
		lwgeom_free(col);
		lwgeom_free(geom);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	if( LW_FAILURE == lwcompound_add_lwgeom((LWCOMPOUND*)col, geom) )
	{
		lwgeom_free(col);
		lwgeom_free(geom);
		SET_PARSER_ERROR(PARSER_ERROR_INCONTINUOUS);
		return NULL;
	}

	return col;
}


LWGEOM* wkt_parser_compound_finalize(LWGEOM *compound, char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	int flagdims = FLAGS_NDIMS(flags);

	/* No geometry means it is empty */
	if ( ! compound )
	{
		return lwcompound_as_lwgeom(lwcompound_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));
	}

	if ( flagdims > 2 )
	{
		/* If the number of dimensions are not consistent, we have a problem. */
		if( flagdims != FLAGS_NDIMS(compound->flags) )
		{
			lwgeom_free(compound);
			SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
			return NULL;
		}

		/* Harmonize the flags in the sub-components with the wkt flags */
		if( LW_FAILURE == wkt_parser_set_dims(compound, flags) )
		{
			lwgeom_free(compound);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}
	}
	compound->type = COMPOUNDTYPE;
	return compound;
}



LWGEOM* wkt_parser_collection_add_geom(LWGEOM *col, LWGEOM *geom)
{
	LWDEBUG(4,"entered");

	/* Toss error on null geometry input */
	if( ! (geom && col) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	return lwcollection_as_lwgeom(lwcollection_add_lwgeom(lwgeom_as_lwcollection(col), geom));
}

/**
 * Finalize a geometry collection by validating and harmonizing dimensionality, and set its collection type.
 *
 * Validates that the provided collection (geom) matches the explicit dimensionality tokens (Z/M) if present,
 * harmonizes sub-geometry dimensionality when needed, and sets geom->type to the requested collection type.
 * If geom is NULL an empty collection of the requested type is constructed (SRID_UNKNOWN) and returned.
 *
 * @param lwtype Target collection type (e.g., COLLECTIONTYPE).
 * @param geom Geometry to finalize; may be NULL to produce an empty collection. On error this function frees geom.
 * @param dimensionality Optional dimensionality token string (e.g. "Z", "M", "ZM"); used to enforce or harmonize Z/M presence.
 *
 * @return The finalized LWGEOM (possibly the input geom, modified), or NULL on error.
 *
 * Side effects:
 * - May free the input geom on error.
 * - May call wkt_parser_set_dims() to propagate dimensionality to sub-geometries.
 * - On error sets parser error codes: PARSER_ERROR_MIXDIMS (dimensionality mismatch) or PARSER_ERROR_OTHER.
 */
LWGEOM* wkt_parser_collection_finalize(int lwtype, LWGEOM *geom, char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	int flagdims = FLAGS_NDIMS(flags);

	/* No geometry means it is empty */
	if( ! geom )
	{
		return lwcollection_as_lwgeom(lwcollection_construct_empty(lwtype, SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));
	}

	/* There are 'Z' or 'M' tokens in the signature */
	if ( flagdims > 2 )
	{
		LWCOLLECTION *col = lwgeom_as_lwcollection(geom);
		uint32_t i;

		for ( i = 0 ; i < col->ngeoms; i++ )
		{
			LWGEOM *subgeom = col->geoms[i];
			if ( FLAGS_NDIMS(flags) != FLAGS_NDIMS(subgeom->flags) &&
				 ! lwgeom_is_empty(subgeom) )
			{
				lwgeom_free(geom);
				SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
				return NULL;
			}

			if ( lwtype == COLLECTIONTYPE &&
			   ( (FLAGS_GET_Z(flags) != FLAGS_GET_Z(subgeom->flags)) ||
			     (FLAGS_GET_M(flags) != FLAGS_GET_M(subgeom->flags)) ) &&
				! lwgeom_is_empty(subgeom) )
			{
				lwgeom_free(geom);
				SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
				return NULL;
			}
		}

		/* Harmonize the collection dimensionality */
		if( LW_FAILURE == wkt_parser_set_dims(geom, flags) )
		{
			lwgeom_free(geom);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}
	}

	/* Set the collection type */
	geom->type = lwtype;

	return geom;
}

/**
 * Create a 2D control point for NURBS with the weight stored in the M component (XYM).
 *
 * @param c1 X coordinate.
 * @param c2 Y coordinate.
 * @param weight Weight for the NURBS control point (stored in M).
 * @return POINT with X=c1, Y=c2, Z=0.0 and M=weight; flags indicate no Z and presence of M.
 */
POINT wkt_parser_nurbs_coord_3(double c1, double c2, double weight)
{
	POINT p;
	p.flags = 0;
	p.x = c1;
	p.y = c2;
	p.z = 0.0;
	p.m = weight;  /* Weight goes in M coordinate */
	FLAGS_SET_Z(p.flags, 0);
	FLAGS_SET_M(p.flags, 1);
	return p;
}

/**
 * Create a 3D control point for NURBS with the weight stored in the M component (XYZM).
 *
 * The returned POINT has X=c1, Y=c2, Z=c3, and M=weight, and has both Z and M flags enabled.
 *
 * @param c1 X coordinate.
 * @param c2 Y coordinate.
 * @param c3 Z coordinate.
 * @param weight Curve weight stored in the point's M component.
 * @return POINT initialized as an XYZM control point with appropriate flags set.
 */
POINT wkt_parser_nurbs_coord_4(double c1, double c2, double c3, double weight)
{
	POINT p;
	p.flags = 0;
	p.x = c1;
	p.y = c2;
	p.z = c3;
	p.m = weight;  /* Weight goes in M coordinate */
	FLAGS_SET_Z(p.flags, 1);
	FLAGS_SET_M(p.flags, 1);
	return p;
}

/**
 * Create a new NURBS curve from WKT parsing
 */

/**
 * Construct a NURBS curve geometry from parsed components.
 *
 * Builds and returns a LWNURBSCURVE (as LWGEOM) using the supplied polynomial
 * degree, control points, optional per-control-point weights, and optional
 * knot vector. If degree == 0, degree 2 is used for backward compatibility.
 *
 * The function validates:
 * - degree is in a supported range (1..10; 0 is treated as 2),
 * - points count is at least degree + 1,
 * - if weights are provided, their count equals the number of points and all
 *   weights are positive (weights are read from the X coordinate of the
 *   supplied weights POINTARRAY),
 * - if knots are provided, their count equals points_count + degree + 1 and
 *   the knot vector is non-decreasing (knot values are read from the X
 *   coordinate of the supplied knots POINTARRAY),
 * - dimensionality of the points is compatible with the optional
 *   dimensionality token.
 *
 * Ownership and side effects:
 * - The function takes ownership of the supplied POINTARRAY pointers: it will
 *   free them on error and will consume them on success (they are passed into
 *   the constructed NURBS curve). Caller must not use or free them after
 *   calling this function.
 * - On any validation or construction failure the function returns NULL and
 *   sets the parser error state.
 *
 * @param degree Polynomial degree for the NURBS curve (0 treated as 2).
 * @param points Control point array (must be non-NULL for a non-empty curve).
 * @param weights Optional weight array (one weight per control point; weight
 *                value is read from each point's X coordinate).
 * @param knots Optional knot array (knot values read from each point's X
 *              coordinate).
 * @param dimensionality Optional dimensionality token (e.g. "Z", "M", "ZM")
 *                        used to derive Z/M flags for the constructed curve.
 *
 * @return Pointer to the constructed LWGEOM representing the NURBS curve, or
 *         NULL on error.
 */
LWGEOM* wkt_parser_nurbscurve_new(double degree, POINTARRAY *points, POINTARRAY *weights, POINTARRAY *knots, char *dimensionality)
{
	LWNURBSCURVE *curve;
	double *weight_array = NULL;
	double *knot_array = NULL;
	uint32_t nweights = 0, nknots = 0;
	lwflags_t flags = wkt_dimensionality(dimensionality);
	int int_degree;

	LWDEBUG(4,"entered wkt_parser_nurbscurve_new");

	/* Validate degree is finite and not fractional */
	if (!isfinite(degree)) {
		if (points) ptarray_free(points);
		if (weights) ptarray_free(weights);
		if (knots) ptarray_free(knots);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* Check if degree has a fractional part */
	if (fabs(degree - round(degree)) >= 1e-10) {
		if (points) ptarray_free(points);
		if (weights) ptarray_free(weights);
		if (knots) ptarray_free(knots);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* Cast to int after validation */
	int_degree = (int)round(degree);

	/* Handle simple format compatibility - if degree is 0, use default degree 2 */
	if (int_degree == 0) {
		int_degree = 2;
	}

	/* Validate degree range */
	if (int_degree < 1 || int_degree > 10) {
		if (points) ptarray_free(points);
		if (weights) ptarray_free(weights);
		if (knots) ptarray_free(knots);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* Handle empty geometry */
	if (!points) {
		if (weights) ptarray_free(weights);
		if (knots) ptarray_free(knots);
		return lwnurbscurve_as_lwgeom(lwnurbscurve_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));
	}

	/* Validate minimum points for degree */
	if (points->npoints < (uint32_t)(int_degree + 1)) {
		ptarray_free(points);
		if (weights) ptarray_free(weights);
		if (knots) ptarray_free(knots);
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}

	/* Check dimensionality consistency */
	if (wkt_pointarray_dimensionality(points, flags) == LW_FALSE) {
		ptarray_free(points);
		if (weights) ptarray_free(weights);
		if (knots) ptarray_free(knots);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Extract weights if provided */
	if (weights && weights->npoints > 0) {
		nweights = weights->npoints;
		if (nweights != points->npoints) {
			ptarray_free(points);
			ptarray_free(weights);
			if (knots) ptarray_free(knots);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}

		weight_array = lwalloc(sizeof(double) * nweights);
		for (uint32_t i = 0; i < nweights; i++) {
			POINT2D p;
			if (!getPoint2d_p(weights, i, &p)) {
				ptarray_free(points);
				ptarray_free(weights);
				if (knots) ptarray_free(knots);
				lwfree(weight_array);
				SET_PARSER_ERROR(PARSER_ERROR_OTHER);
				return NULL;
			}
			weight_array[i] = p.x; /* Weight is stored in X coordinate */

			if (weight_array[i] <= 0.0) {
				ptarray_free(points);
				ptarray_free(weights);
				if (knots) ptarray_free(knots);
				lwfree(weight_array);
				SET_PARSER_ERROR(PARSER_ERROR_OTHER);
				return NULL;
			}
		}
		ptarray_free(weights);
	}

	/* Extract knots if provided */
	if (knots && knots->npoints > 0) {
		nknots = knots->npoints;
		uint32_t expected_knots = points->npoints + int_degree + 1;

		if (nknots != expected_knots) {
			ptarray_free(points);
			if (weight_array) lwfree(weight_array);
			ptarray_free(knots);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}

		knot_array = lwalloc(sizeof(double) * nknots);
		for (uint32_t i = 0; i < nknots; i++) {
			POINT2D p;
			if (!getPoint2d_p(knots, i, &p)) {
				ptarray_free(points);
				if (weight_array) lwfree(weight_array);
				ptarray_free(knots);
				lwfree(knot_array);
				SET_PARSER_ERROR(PARSER_ERROR_OTHER);
				return NULL;
			}
			knot_array[i] = p.x; /* Knot value is stored in X coordinate */
		}

		/* Validate knot vector is non-decreasing */
		for (uint32_t i = 1; i < nknots; i++) {
			if (knot_array[i] < knot_array[i-1]) {
				ptarray_free(points);
				if (weight_array) lwfree(weight_array);
				ptarray_free(knots);
				lwfree(knot_array);
				SET_PARSER_ERROR(PARSER_ERROR_OTHER);
				return NULL;
			}
		}

		/* Basic knot vector validation - ensure knots are in non-decreasing order */
		for (uint32_t i = 1; i < nknots; i++) {
			if (knot_array[i] < knot_array[i-1]) {
				ptarray_free(points);
				if (weight_array) lwfree(weight_array);
				ptarray_free(knots);
				lwfree(knot_array);
				SET_PARSER_ERROR(PARSER_ERROR_OTHER);
				return NULL;
			}
		}

		ptarray_free(knots);
	}

	/* Create the NURBS curve */
	curve = lwnurbscurve_construct(SRID_UNKNOWN, NULL, int_degree, points, weight_array, knot_array, nweights, nknots);

	if (!curve) {
		if (weight_array) lwfree(weight_array);
		if (knot_array) lwfree(knot_array);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	/* Deep-copied by constructor */
	if (weight_array) lwfree(weight_array);
	if (knot_array) lwfree(knot_array);
	return lwnurbscurve_as_lwgeom(curve);
}


/**
 * Create an empty NURBS curve with the given dimensionality token.
 *
 * The `dimensionality` token (e.g., "Z", "M", "ZM", or NULL) controls whether
 * the created empty NURBS geometry has Z and/or M components. The returned
 * geometry uses SRID_UNKNOWN.
 *
 * @param dimensionality Dimensionality token (case-insensitive) or NULL for 2D.
 * @return A newly allocated empty LWNURBSCURVE as an LWGEOM on success;
 *         NULL on failure (parser error set).
 */
LWGEOM* wkt_parser_nurbscurve_empty(char *dimensionality)
{
	lwflags_t flags = wkt_dimensionality(dimensionality);
	LWNURBSCURVE *curve;

	LWDEBUG(4,"entered wkt_parser_nurbscurve_empty");

	curve = lwnurbscurve_construct_empty(SRID_UNKNOWN,
		FLAGS_GET_Z(flags), FLAGS_GET_M(flags));

	if (!curve) {
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	return lwnurbscurve_as_lwgeom(curve);
}

/**
 * Register a parsed geometry and apply an SRID.
 *
 * If `geom` is non-NULL, sets its SRID to `srid` when `srid` is not SRID_UNKNOWN and
 * is within the valid range; otherwise sets the geometry SRID to SRID_UNKNOWN.
 * Stores the geometry pointer in the global parser result object.
 *
 * @param geom Parsed geometry to register (may be NULL — function returns without action).
 * @param srid Spatial reference identifier to assign to the geometry.
 */
void
wkt_parser_geometry_new(LWGEOM *geom, int32_t srid)
{
	LWDEBUG(4,"entered");
	LWDEBUGF(4,"geom %p",geom);
	LWDEBUGF(4,"srid %d",srid);

	if ( geom == NULL )
	{
		lwerror("Parsed geometry is null!");
		return;
	}

	if ( srid != SRID_UNKNOWN && srid <= SRID_MAXIMUM )
		lwgeom_set_srid(geom, srid);
	else
		lwgeom_set_srid(geom, SRID_UNKNOWN);

	global_parser_result.geom = geom;
}

void lwgeom_parser_result_init(LWGEOM_PARSER_RESULT *parser_result)
{
	memset(parser_result, 0, sizeof(LWGEOM_PARSER_RESULT));
}


void lwgeom_parser_result_free(LWGEOM_PARSER_RESULT *parser_result)
{
	if ( parser_result->geom )
	{
		lwgeom_free(parser_result->geom);
		parser_result->geom = 0;
	}
	if ( parser_result->serialized_lwgeom )
	{
		lwfree(parser_result->serialized_lwgeom );
		parser_result->serialized_lwgeom = 0;
	}
	/* We don't free parser_result->message because
	   it is a const *char */
}

/*
* Public function used for easy access to the parser.
*/
LWGEOM *lwgeom_from_wkt(const char *wkt, const char check)
{
	LWGEOM_PARSER_RESULT r;

	if( LW_FAILURE == lwgeom_parse_wkt(&r, (char*)wkt, check) )
	{
		lwerror("%s", r.message);
		return NULL;
	}

	return r.geom;
}


