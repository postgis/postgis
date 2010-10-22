#include "lwin_wkt.h"
#include "lwin_wkt_parse.h"


/*
* Error messages for failures in the parser. 
*/
static const char *parser_error_messages[] =
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
	"unknown parse error",
	"geometry has too many points"
};

#define SET_PARSER_ERROR(errno) { \
		global_parser_result.message = parser_error_messages[(errno)]; \
		global_parser_result.errcode = (errno); \
	}
		

int wkt_lexer_read_srid(char *str)
{
	char *c = str;
	long i = 0;
	
	if( ! str ) return 0;
	c += 5; /* Advance past "SRID=" */
	i = strtol(c, NULL, 10);
	return (int)i;
};

static uchar wkt_dimensionality(char *dimensionality)
{
	int i = 0;
	uchar flags = 0;
	
	if( ! dimensionality ) 
		return flags;
	
	/* If there's an explicit dimensionality, we use that */
	for( i = 0; i < strlen(dimensionality); i++ )
	{
		if( dimensionality[i] == 'Z' || dimensionality[i] == 'z' )
			FLAGS_SET_Z(flags,1);
		if( dimensionality[i] == 'M' || dimensionality[i] == 'm' )
			FLAGS_SET_M(flags,1);
	}
	return flags;
}


/**
* Force the dimensionality of a geometry to match the dimensionality
* of a set of flags (usually derived from a ZM WKT tag).
*/
static int wkt_parser_set_dims(LWGEOM *geom, uchar flags)
{
	int hasz = FLAGS_GET_Z(flags);
	int hasm = FLAGS_GET_M(flags);
	int i = 0;
	
	/* Error on junk */
	if( ! geom ) 
		return LW_FALSE;

	TYPE_SETZM(geom->type, hasz, hasm);
		
	if( ! lwgeom_is_empty(geom) )
	{
		int lwtype = TYPE_GETTYPE(geom->type);
		if( lwtype == POINTTYPE )
		{
			LWPOINT *pt = (LWPOINT*)geom;
			TYPE_SETZM(pt->point->dims, hasz, hasm);
			return LW_TRUE;
		}
		else if ( lwtype == TRIANGLETYPE || 
		          lwtype == CIRCSTRINGTYPE || 
		          lwtype == LINETYPE )
		{
			LWLINE *ln = (LWLINE*)geom;
			TYPE_SETZM(ln->points->dims, hasz, hasm);
			return LW_TRUE;
		}
		else if ( lwtype == POLYGONTYPE )
		{
			LWPOLY *poly = (LWPOLY*)geom;
			for ( i = 0; i < poly->nrings; i++ )
				TYPE_SETZM(poly->rings[i]->dims, hasz, hasm);
			return LW_TRUE;
		}
		else if ( lwtype == CURVEPOLYTYPE )
		{
			LWCURVEPOLY *poly = (LWCURVEPOLY*)geom;
			for ( i = 0; i < poly->nrings; i++ )
				wkt_parser_set_dims(poly->rings[i], flags);
			return LW_TRUE;
		}
		else if ( lwgeom_is_collection(lwtype) )
		{
			LWCOLLECTION *col = (LWCOLLECTION*)geom;
			for ( i = 0; i < col->ngeoms; i++ )
				wkt_parser_set_dims(col->geoms[i], flags);			
			return LW_TRUE;
		}
		else
		{
			LWDEBUGF(2,"Unknown geometry type: %d", type);
			return LW_FALSE;
		}	
	}
	return LW_TRUE;				
}

/**
* Read the dimensionality from a "ZM" string, if provided. Then check that the
* dimensionality matches that of the pointarray. If the dimension counts
* match, ensure the pointarray is using the right "Z" or "M".
*/
static int wkt_pointarray_dimensionality(POINTARRAY *pa, uchar flags)
{	
	int hasz = FLAGS_GET_Z(flags);
	int hasm = FLAGS_GET_M(flags);
	int ndims = 2 + hasz + hasm;

	/* No dimensionality or array means we go with what we have */
	if( ! flags || ! pa )
		return LW_TRUE;
		
	LWDEBUGF(5,"dimensionality ndims == %d", ndims);
	LWDEBUGF(5,"TYPE_NDIMS(pa->dims) == %d", TYPE_NDIMS(pa->dims));
	
	/* Our number of dimensions don't match, that's a failure */
	if( TYPE_NDIMS(pa->dims) != ndims )
		return LW_FALSE;
		
	/* Set the dimensionality of the array to match the requested ZM */
	TYPE_SETZM(pa->dims, hasz, hasm);
	
	return LW_TRUE;
}


/**
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
};

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
};

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
};

void wkt_parser_ptarray_add_coord(POINTARRAY *pa, POINT p)
{
	POINT4D pt;
	
	/* Error on trouble */
	if( ! pa ) 
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return;	
	}
	
	/* Check that the coordinate has the same dimesionality as the array */
	if( FLAGS_NDIMS(p.flags) != TYPE_NDIMS(pa->dims) )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_MIXDIMS];
		global_parser_result.errcode = PARSER_ERROR_MIXDIMS;
		return;
	}
	
	/* While parsing the point arrays, XYM and XMZ points are both treated as XYZ */
	pt.x = p.x;
	pt.y = p.y;
	if( TYPE_HASZ(pa->dims) )
		pt.z = p.z;
	if( TYPE_HASM(pa->dims) )
		pt.m = p.m;
	/* If the destination is XYM, we'll write the third coordinate to m */
	if( TYPE_HASM(pa->dims) && ! TYPE_HASZ(pa->dims) )
		pt.m = p.z;
		
	ptarray_add_point(pa, &pt);
}

/**
* Start a point array from the first coordinate.
*/
POINTARRAY* wkt_parser_ptarray_new(POINT p)
{
	int ndims = FLAGS_NDIMS(p.flags);
	POINTARRAY *pa = ptarray_construct_empty((ndims>2), (ndims>3));
	if ( ! pa )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	wkt_parser_ptarray_add_coord(pa, p);
	return pa;
}

/**
* Create a new point. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT.
*/
LWGEOM* wkt_parser_point_new(POINTARRAY *pa, char *dimensionality)
{
	uchar flags = wkt_dimensionality(dimensionality);
	
	/* No pointarray means it is empty */
	if( ! pa )
		return lwpoint_as_lwgeom(lwpoint_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == LW_FALSE )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Only one point allowed in our point array! */	
	if( pa->npoints != 1 )
	{
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
	uchar flags = wkt_dimensionality(dimensionality);

	/* No pointarray means it is empty */
	if( ! pa )
		return lwline_as_lwgeom(lwline_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == LW_FALSE )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}
	
	/* Apply check for not enough points, if requested. */	
	if( (global_parser_result.parser_check_flags & PARSER_CHECK_MINPOINTS) && (pa->npoints < 2) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}

	return lwline_as_lwgeom(lwline_construct(SRID_UNKNOWN, NULL, pa));
}

/**
* Create a new circularstring. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT. 
* Circular strings are just like linestrings, except with slighty different
* validity rules (minpoint == 3, numpoints % 2 == 1). 
*/
LWGEOM* wkt_parser_circularstring_new(POINTARRAY *pa, char *dimensionality)
{
	uchar flags = wkt_dimensionality(dimensionality);

	/* No pointarray means it is empty */
	if( ! pa )
		return lwcircstring_as_lwgeom(lwcircstring_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == LW_FALSE )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}
	
	/* Apply check for not enough points, if requested. */	
	if( (global_parser_result.parser_check_flags & PARSER_CHECK_MINPOINTS) && (pa->npoints < 3) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}	

	/* Apply check for odd number of points, if requested. */	
	if( (global_parser_result.parser_check_flags & PARSER_CHECK_ODD) && ((pa->npoints % 2) == 1) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_ODDPOINTS);
		return NULL;
	}
	
	return lwcircstring_as_lwgeom(lwcircstring_construct(SRID_UNKNOWN, NULL, pa));	
}

LWGEOM* wkt_parser_triangle_new(POINTARRAY *pa, char *dimensionality)
{
	uchar flags = wkt_dimensionality(dimensionality);

	/* No pointarray means it is empty */
	if( ! pa )
		return lwtriangle_as_lwgeom(lwtriangle_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == LW_FALSE )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Triangles need four points. */	
	if( (pa->npoints != 4) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_TRIANGLEPOINTS);
		return NULL;
	}	
	
	/* Triangles need closure. */	
	if( ! ptarray_isclosed(pa) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_UNCLOSED);
		return NULL;
	}	

	return lwtriangle_as_lwgeom(lwtriangle_construct(SRID_UNKNOWN, NULL, pa));
}

LWGEOM* wkt_parser_polygon_new(POINTARRAY *pa)
{
	POINTARRAY **pas = NULL;
	LWPOLY *poly = NULL;
	static int nrings = 1;
	
	/* No pointarray is a problem */
	if( ! pa )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;	
	}

	pas = lwalloc(nrings * sizeof(POINTARRAY*));
	pas[0] = pa;
	poly = lwpoly_construct(SRID_UNKNOWN, NULL, nrings, pas);
	
	/* Error out if we can't build this polygon. */
	if( ! poly )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	
	return lwpoly_as_lwgeom(poly);
}

void wkt_parser_polygon_add_ring(LWGEOM *poly, POINTARRAY *pa)
{
	/* Bad inputs are a problem */
	if( ! pa || ! poly )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return;	
	}

	/* Rings must agree on dimensionality */
	if( TYPE_NDIMS(poly->type) != TYPE_NDIMS(pa->dims) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return;
	}

	/* Apply check for minimum number of points, if requested. */	
	if( (global_parser_result.parser_check_flags & PARSER_CHECK_MINPOINTS) && (pa->npoints < 4) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return;
	}
	
	/* Apply check for not closed rings, if requested. */	
	if( (global_parser_result.parser_check_flags & PARSER_CHECK_CLOSURE) && ! ptarray_isclosed(pa) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_UNCLOSED);
		return;
	}

	/* If something goes wrong adding a ring, error out. */
	if ( LW_FALSE == lwpoly_add_ring(lwgeom_as_lwpoly(poly), pa) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return;	
	}
}

LWGEOM* wkt_parser_polygon_finalize(LWGEOM *poly, char *dimensionality)
{
	uchar flags = wkt_dimensionality(dimensionality);
	
	/* Null input implies empty return */
	if( ! poly )
		return lwpoly_as_lwgeom(lwpoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions are not consistent, we have a problem. */
	if( FLAGS_NDIMS(flags) != TYPE_NDIMS(poly->type) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Harmonize the flags in the sub-components with the wkt flags */
	if( LW_FALSE == wkt_parser_set_dims(poly, flags) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
			
	return poly;
}

LWGEOM* wkt_parser_collection_finalize(int lwtype, LWGEOM *col, char *dimensionality) 
{
	uchar flags = wkt_dimensionality(dimensionality);
	LWDEBUG(4,"entered");

	LWDEBUGF(4,"type1 %d",col->type);
	
	/* No geometry means it is empty */
	if( ! col )
	{
		LWCOLLECTION *col = lwcollection_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags));
		TYPE_SETTYPE(col->type, lwtype);
		return lwcollection_as_lwgeom(col);
	}

	/* If the number of dimensions are not consistent, we have a problem. */
	if( FLAGS_NDIMS(flags) != TYPE_NDIMS(col->type) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* For GEOMETRYCOLLECTION, the exact type of the dimensions must match too */
	if( lwtype == COLLECTIONTYPE &&
	    ( FLAGS_GET_Z(flags) != TYPE_HASZ(col->type) ||
	      FLAGS_GET_M(flags) != TYPE_HASM(col->type) ) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}
		
	/* Harmonize the collection dimensionality */
	if( LW_FALSE == wkt_parser_set_dims(col, flags) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	
	/* Set the collection type */
	TYPE_SETTYPE(col->type, lwtype);
			
	return col;
}


void wkt_parser_collection_add_geom(LWGEOM *col, LWGEOM *geom)
{
	LWCOLLECTION *c;
	LWDEBUG(4,"entered");

	/* No action if the geometry argument is null. */
	if ( ! geom ) return;
	
	/* Toss an error on a null collection input */
	if ( ! col )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return;
	}

	/* All the elements must agree on dimensionality */
	if( TYPE_HASZ(col->type) != TYPE_HASZ(geom->type) || 
	    TYPE_HASM(col->type) != TYPE_HASM(geom->type) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return;
	}
	c = lwcollection_add_lwgeom(lwgeom_as_lwcollection(col), geom);
	return;
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

void wkt_parser_geometry_new(LWGEOM *geom, int srid)
{
	LWDEBUG(4,"entered");
	LWDEBUGF(4,"geom %p",geom);

	if ( geom == NULL ) 
	{
		lwerror("Parsed geometry is null!");
		return;
	}
		
	if ( srid != SRID_UNKNOWN && srid < SRID_MAXIMUM )
		lwgeom_set_srid(geom, srid);
	else
		lwgeom_set_srid(geom, SRID_UNKNOWN);
	
	global_parser_result.geom = geom;
}


void lwgeom_parser_result_free(LWGEOM_PARSER_RESULT *parser_result)
{
	if ( parser_result->geom )
		lwgeom_free(parser_result->geom);
	if ( parser_result->serialized_lwgeom )
		lwfree(parser_result->serialized_lwgeom );
	/* We don't free parser_result->message because
	   it is a const *char */
}



