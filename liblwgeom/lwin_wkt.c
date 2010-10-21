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

/**
* Given flags and a string ("Z", "M" or "ZM") determine if the flags and 
* string describe the same number of dimensions. If they do, update the flags
* to ensure they are using the correct higher dimension in the 3D case ("M" or "Z").
static int wkt_parser_dimensionality_check(uchar flags, uchar type)
{
	if( ! ( FLAGS_GET_M(flags) == TYPE_HASM(type) ) )
		return LW_FALSE;

	if( ! ( FLAGS_GET_Z(flags) == TYPE_HASZ(type) ) )
		return LW_FALSE;
	
	return LW_TRUE;
}
*/

int wkt_lexer_read_srid(char *str)
{
	char *c = str;
	long i = 0;
	
	if( ! str ) return 0;
	c += 5; /* Advance past "SRID=" */
	i = strtol(c, NULL, 10);
	return (int)i;
};


static uchar wkt_parser_dimensionality(POINTARRAY *pa, char *dimensionality)
{	
	uchar flags = 0;
	int i;

	/* If there's an explicit dimensionality, we use that */
	if( dimensionality )
	{
		for( i = 0; i < strlen(dimensionality); i++ )
		{
			if( dimensionality[i] == 'Z' || dimensionality[i] == 'z' )
				FLAGS_SET_Z(flags,1);
			if( dimensionality[i] == 'M' || dimensionality[i] == 'm' )
				FLAGS_SET_M(flags,1);
		}
	}
	/* Otherwise we use the implicit dimensionality in the number of coordinate dimensions */
	else if ( pa )
	{
		FLAGS_SET_Z(flags,TYPE_HASZ(pa->dims));
		FLAGS_SET_M(flags,TYPE_HASM(pa->dims));
	}
	
	return flags;
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
	
	/* Avoid trouble */
	if( ! pa ) return;
	
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

POINTARRAY* wkt_parser_ptarray_new(int ndims)
{
	POINTARRAY *pa = ptarray_construct_empty((ndims>2), (ndims>3));
	return pa;
}

/**
* Create a new point. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT.
*/
LWGEOM* wkt_parser_point_new(POINTARRAY *pa, char *dimensionality)
{
	/* If there's an explicit dimensionality, we use that, otherwise
	   use the implicit dimensionality of the array. */
	uchar flags = wkt_parser_dimensionality(pa, dimensionality);
	
	/* No pointarray means it is empty */
	if( ! pa )
		return lwpoint_as_lwgeom(lwpoint_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* What are the numbers of dimensions? */
	LWDEBUGF(5,"FLAGS_NDIMS(flags) == %d", FLAGS_NDIMS(flags));
	LWDEBUGF(5,"TYPE_NDIMS(pa->dims) == %d", TYPE_NDIMS(pa->dims));
	
	/* If the number of dimensions is not consistent, we have a problem. */
	if( FLAGS_NDIMS(flags) != TYPE_NDIMS(pa->dims) )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_MIXDIMS];
		global_parser_result.errcode = PARSER_ERROR_MIXDIMS;
		return NULL;
	}
	
	TYPE_SETZM(pa->dims, FLAGS_GET_Z(flags), FLAGS_GET_M(flags));

	/* Only one point allowed in our point array! */	
	if( pa->npoints != 1 )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_LESSPOINTS];
		global_parser_result.errcode = PARSER_ERROR_LESSPOINTS;
		return NULL;
	}		

	return lwpoint_as_lwgeom(lwpoint_construct(SRID_UNKNOWN, NULL, pa));
}

/**
* Create a new point. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT.
*/
LWGEOM* wkt_parser_multipoint_new(POINTARRAY *pa, char *dimensionality)
{
	/* If there's an explicit dimensionality, we use that, otherwise
	   use the implicit dimensionality of the array. */
	uchar flags = wkt_parser_dimensionality(pa, dimensionality);
	
	/* No pointarray means it is empty */
	if( ! pa )
		return lwmpoint_as_lwgeom(lwmpoint_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* What are the numbers of dimensions? */
	LWDEBUGF(5,"FLAGS_NDIMS(flags) == %d", FLAGS_NDIMS(flags));
	LWDEBUGF(5,"TYPE_NDIMS(pa->dims) == %d", TYPE_NDIMS(pa->dims));
	
	/* If the number of dimensions is not consistent, we have a problem. */
	if( FLAGS_NDIMS(flags) != TYPE_NDIMS(pa->dims) )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_MIXDIMS];
		global_parser_result.errcode = PARSER_ERROR_MIXDIMS;
		return NULL;
	}
	
	TYPE_SETZM(pa->dims, FLAGS_GET_Z(flags), FLAGS_GET_M(flags));

	return lwmpoint_as_lwgeom(lwmpoint_construct(SRID_UNKNOWN, NULL, pa));
}




/**
* Create a new linestring. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT. Check for numpoints >= 2 if
* requested.
*/
LWGEOM* wkt_parser_linestring_new(POINTARRAY *pa, char *dimensionality)
{
	/* If there's an explicit dimensionality, we use that, otherwise
	   use the implicit dimensionality of the array. */
	uchar flags = wkt_parser_dimensionality(pa, dimensionality);
	
	/* No pointarray means it is empty */
	if( ! pa )
		return lwline_as_lwgeom(lwline_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* What are the numbers of dimensions? */
	LWDEBUGF(5,"FLAGS_NDIMS(flags) == %d", FLAGS_NDIMS(flags));
	LWDEBUGF(5,"TYPE_NDIMS(pa->dims) == %d", TYPE_NDIMS(pa->dims));
	
	/* If the number of dimensions is not consistent, we have a problem. */
	if( FLAGS_NDIMS(flags) != TYPE_NDIMS(pa->dims) )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_MIXDIMS];
		global_parser_result.errcode = PARSER_ERROR_MIXDIMS;
		return NULL;
	}
	
	TYPE_SETZM(pa->dims, FLAGS_GET_Z(flags), FLAGS_GET_M(flags));

	/* Apply check for not enough points, if requested. */	
	if( (global_parser_result.parser_check_flags & PARSER_CHECK_MINPOINTS) && (pa->npoints < 2) )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_MOREPOINTS];
		global_parser_result.errcode = PARSER_ERROR_MOREPOINTS;
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
	/* If there's an explicit dimensionality, we use that, otherwise
	   use the implicit dimensionality of the array. */
	uchar flags = wkt_parser_dimensionality(pa, dimensionality);
	
	/* No pointarray means it is empty */
	if( ! pa )
		return lwcircstring_as_lwgeom(lwcircstring_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* What are the numbers of dimensions? */
	LWDEBUGF(5,"FLAGS_NDIMS(flags) == %d", FLAGS_NDIMS(flags));
	LWDEBUGF(5,"TYPE_NDIMS(pa->dims) == %d", TYPE_NDIMS(pa->dims));
	
	/* If the number of dimensions is not consistent, we have a problem. */
	if( FLAGS_NDIMS(flags) != TYPE_NDIMS(pa->dims) )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_MIXDIMS];
		global_parser_result.errcode = PARSER_ERROR_MIXDIMS;
		return NULL;
	}
	
	TYPE_SETZM(pa->dims, FLAGS_GET_Z(flags), FLAGS_GET_M(flags));

	/* Apply check for not enough points, if requested. */	
	if( (global_parser_result.parser_check_flags & PARSER_CHECK_MINPOINTS) && (pa->npoints < 3) )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_MOREPOINTS];
		global_parser_result.errcode = PARSER_ERROR_MOREPOINTS;
		return NULL;
	}	

	/* Apply check for odd number of points, if requested. */	
	if( (global_parser_result.parser_check_flags & PARSER_CHECK_ODD) && ((pa->npoints % 2) == 1) )
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_ODDPOINTS];
		global_parser_result.errcode = PARSER_ERROR_ODDPOINTS;
		return NULL;
	}
	
	return lwcircstring_as_lwgeom(lwcircstring_construct(SRID_UNKNOWN, NULL, pa));	
	
}


void wkt_parser_geometry_new(LWGEOM *geom, int srid)
{
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



