#include "lwin_wkt.h"

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

static uchar wkt_parser_dimensionality(POINTARRAY *pa, char *dimensionality)
{	
	uchar flags = 0;
	int i;

	/* If there's an explicit dimensionality, we use that */
	if( dimensionality )
	{
		for( i = 0; i < strlen(dimensionality); i++ )
		{
			if( dimensionality[i] == 'Z' )
				FLAGS_SET_Z(flags,1);
			if( dimensionality[i] == 'M' )
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
* Create a new linestring. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT. Check for 
*/
LWGEOM* wkt_parser_linestring_new(POINTARRAY *pa, char *dimensionality)
{
	uchar flags;
	
	/* TODO apply the parser checks? (not enough points, etc) */	
	
	/* If there's an explicit dimensionality, we use that */
	flags = wkt_parser_dimensionality(pa, dimensionality);
	
	/* No pointarray means it is empty */
	if( ! pa )
		return lwline_as_lwgeom(lwline_construct_empty(0, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));
	
	/* If the number of dimensions is not consistent, we have a problem. */
	if( FLAGS_NDIMS(flags) != TYPE_NDIMS(pa->dims) )
	{
		/* TODO: Error out of the parse. */		
		printf("________ ndims of array != ndims of dimensionalty tokens \n\n>>>>>> ERROR <<<<<<<\n\n");
	}
	
	TYPE_SETZM(pa->dims, FLAGS_GET_Z(flags), FLAGS_GET_M(flags));

	return lwline_as_lwgeom(lwline_construct(0, NULL, pa));
}

/* Circular strings are just like linestrings! */
LWGEOM* wkt_parser_circularstring_new(POINTARRAY *pa, char *dimensionality)
{
	/* TODO apply the parser checks? */
	LWCIRCSTRING *circ = (LWCIRCSTRING*)wkt_parser_linestring_new(pa, dimensionality);
	TYPE_SETTYPE(circ->type, CIRCSTRINGTYPE);
	return lwcircstring_as_lwgeom(circ);
}
