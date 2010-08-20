/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "libgeom.h"

/**********************************************************************/

static char hex2char[256] = {
    /* not Hex characters */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    /* 0-9 */
    0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
    /* A-F */
    -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    /* not Hex characters */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* a-f */
    -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    /* not Hex characters (upper 128 characters) */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };


static char* bytes_from_hexbytes(const char *hexbuf, size_t hexsize)
{
	char *buf = NULL;
	register char h1, h2;
	int i;
	
	if( hexsize % 2 )
		lwerror("Hex string size has to be a multiple of two.");

	buf = lwalloc(hexsize/2);
	
	if( ! buf )
		lwerror("Unable to allocate memory buffer.");
		
	for( i = 0; i < hexsize/2; i++ )
	{
		h1 = hex2char[hexbuf[2*i]];
		h2 = hex2char[hexbuf[2*i+1]];
		if( h1 < 0 )
			lwerror("Invalid hex character (%c) encountered", hexbuf[i]);
		if( h2 < 0 )
			lwerror("Invalid hex character (%c) encountered", hexbuf[i+1]);
		/* First character is high bits, second is low bits */
		buf[i] = ((h1 & 0x0F) << 4) | (h2 & 0x0F);
	}
	return buf;
}

/**********************************************************************/



/**
* Used for passing the parse state between the parsing functions.
*/
typedef struct 
{
	const char *wkb; /* Points to start of WKB */
	const size_t wkb_size; /* Expected size of WKB */
	int swap_bytes; /* Do an endian flip? */
	int check; /* Simple validity checks on geometries */
	uint32 lwtype; /* Current type we are handling */
	uint32 srid; /* Current SRID we are handling */
	int has_z; /* Z? */
	int has_m; /* M? */
	int has_srid; /* SRID? */
	const char *pos; /* Current parse position */
} wkb_parse_state;

/**
* Check that we are not about to read off the end of the WKB 
* array.
*/
static inline void wkb_parse_state_check(wkb_parse_state *s, size_t next)
{
	if( (s->pos + next) > (s->wkb + s->wkb_size) )
		lwerror("WKB structure does not match expected size!");
} 

/**
* Take in an unknown kind of wkb type number and ensure it comes out
* as an extended WKB type number (with Z/M/SRID flags masked onto the 
* high bits).
*/
static void lwtype_from_wkb_state(wkb_parse_state *s, uint32 wkb_type)
{
	uint32 wkb_simple_type;
	
	s->has_z = LW_FALSE;
	s->has_m = LW_FALSE;
	s->has_srid = LW_FALSE;

	/* If any of the higher bits are set, this is probably an extended type. */
	if( wkb_type & 0xF0000000 )
	{
		s->has_z = wkb_type & WKBZOFFSET;
		s->has_m = wkb_type & WKBMOFFSET;
		s->has_srid = wkb_type & WKBSRIDFLAG;
	}
	
	/* Mask off the flags */
	wkb_type = wkb_type & 0x0FFFFFFF;
	/* Strip out just the type number (1-12) from the ISO number (eg 3001-3012) */
	wkb_simple_type = wkb_type % 1000;
	
	/* Only the extended form has an SRID */
	if( wkb_type >= 3000 && wkb_type < 4000 )
	{
		s->has_z = LW_TRUE;
		s->has_m = LW_TRUE;
	}
	else if ( wkb_type >= 2000 && wkb_type < 3000 )
	{
		s->has_m = LW_TRUE;
	}
	else if ( wkb_type >= 1000 && wkb_type < 2000 )
	{
		s->has_z = LW_TRUE;
	}

	switch (wkb_simple_type)
	{
		case 1: /* POINT */
			s->lwtype = POINTTYPE;
			break;
		case 2: /* LINESTRING */
			s->lwtype = LINETYPE;
			break;
		case 3: /* POLYGON */
			s->lwtype = POLYGONTYPE;
			break;
		case 4: /* MULTIPOINT */
			s->lwtype = MULTIPOINTTYPE;
			break;
		case 5: /* MULTILINESTRING */
			s->lwtype = MULTILINETYPE;
			break;
		case 6: /* MULTIPOLYGON */
			s->lwtype = MULTIPOLYGONTYPE;
			break;
		case 7: /* GEOMETRYCOLLECTION */
			s->lwtype = COLLECTIONTYPE;
			break;
		case 8: /* CIRCULARSTRING */
			s->lwtype = CIRCSTRINGTYPE;
			break;
		case 9: /* COMPOUNDCURVE */
			s->lwtype = COMPOUNDTYPE;
			break;
		case 10: /* CURVEPOLYGON */
			s->lwtype = CURVEPOLYTYPE;
			break;
		case 11: /* MULTICURVE */
			s->lwtype = MULTICURVETYPE;
			break;
		case 12: /* MULTISURFACE */
			s->lwtype = MULTISURFACETYPE;
			break;
		default: /* Error! */
			lwerror("Unknown WKB type! %d", wkb_simple_type);
			break;	
	}
	return;
}

/**
* Byte
*/
static char byte_from_wkb_state(wkb_parse_state *s)
{
	char char_value = 0;

	wkb_parse_state_check(s, WKB_BYTE_SIZE);
	
	char_value = s->pos[0];
	s->pos += WKB_BYTE_SIZE;
	
	return char_value;
}

/**
* Int32
*/
static uint32 integer_from_wkb_state(wkb_parse_state *s)
{
	uint32 i = 0;

	wkb_parse_state_check(s, WKB_INT_SIZE);
	
	i = *((uint32*)(s->pos));
	
	/* Swap? Copy into a stack-allocated integer. */
	if( s->swap_bytes )
	{
		int j = 0;
		uchar tmp;
		
		for( j = 0; j < WKB_INT_SIZE/2; j++ )
		{
			tmp = ((uchar*)(&i))[j];
			((uchar*)(&i))[j] = ((uchar*)(&i))[WKB_INT_SIZE - j - 1];
			((uchar*)(&i))[WKB_INT_SIZE - j - 1] = tmp;
		}
	}

	s->pos += WKB_INT_SIZE;
	return i;
}

/**
* Double
*/
static double double_from_wkb_state(wkb_parse_state *s)
{
	double d = 0;

	wkb_parse_state_check(s, WKB_DOUBLE_SIZE);

	d = *((double*)(s->pos));

	/* Swap? Copy into a stack-allocated integer. */
	if( s->swap_bytes )
	{
		int i = 0;
		uchar tmp;
		
		for( i = 0; i < WKB_DOUBLE_SIZE/2; i++ )
		{
			tmp = ((uchar*)(&d))[i];
			((uchar*)(&d))[i] = ((uchar*)(&d))[WKB_DOUBLE_SIZE - i - 1];
			((uchar*)(&d))[WKB_DOUBLE_SIZE - i - 1] = tmp;
		}

	}

	s->pos += WKB_DOUBLE_SIZE;
	return d;
}

static POINTARRAY* ptarray_from_wkb_state(wkb_parse_state *s)
{
	POINTARRAY *pa = NULL;
	size_t pa_size;
	uint32 ndims = 2;
	uint32 npoints = 0;

	npoints = integer_from_wkb_state(s);
	if( s->has_z ) ndims++;
	if( s->has_m ) ndims++;

	/* Empty! */
	if( npoints == 0 )
		return NULL;

	/* Does the data we want to read exist? */
	wkb_parse_state_check(s, npoints * ndims * WKB_DOUBLE_SIZE);
	
	/* If we're in a native endianness, we can just copy the data directly! */
	if( ! s->flip_bytes )
	{
		pa = ptarray_construct_copy_data(s->has_z, s->has_m, npoints, s->pos);
		s->pos += pa->npoints * TYPE_NDIMS(pa->dims);
	}
	/* Otherwise we have to read each double, separately */
	else
	{
		int i = 0;
		double d;
		double *dlist;
		pa = ptarray_construct(s->has_z, s->has_m, npoints);
		dlist = (double*)(pa->serialized_pointlist);
		for( i = 0; i < npoints * ndims; i++ )
		{
			dlist[i] = double_from_wkb_state(s);
		}
	}
		
	return pa;
}

/**
* LINESTRING
*/
static LWLINE* lwline_from_wkb_state(wkb_parse_state *s)
{
	POINTARRAY pa = ptarray_from_wkb_state(s);

	if( pa == NULL )
		return lwline_construct_empty(s->has_z, s->has_m, s->srid);

	if( s->check & PARSER_CHECK_MINPOINTS && pa->npoints < 2 )
	{
		lwerror("%s must have at least two points", lwtype_name(s->lwtype));
		return NULL;
	}

	return lwline_construct(s->srid, NULL, pa);
}

/**
* CIRCULARSTRING
*/
static LWCIRCSTRING* lwcircstring_from_wkb_state(wkb_parse_state *s)
{
	POINTARRAY pa = ptarray_from_wkb_state(s);

	if( pa == NULL )
		return lwcircstring_construct_empty(s->has_z, s->has_m, s->srid);

	if( s->check & PARSER_CHECK_MINPOINTS && pa->npoints < 3 )
	{
		lwerror("%s must have at least three points", lwtype_name(s->lwtype));
		return NULL;
	}

	if( s->check & PARSER_CHECK_ODD && ! (pa->npoints % 2) )
	{
		lwerror("%s must have an odd number of points", lwtype_name(s->lwtype));
		return NULL;
	}

	return lwcircstring_construct(s->srid, NULL, pa);	
}

/**
* POLYGON
*/
static LWPOLY* lwpoly_from_wkb_state(wkb_parse_state *s)
{
	uint32 nrings = integer_from_wkb_state(s);
	int i = 0;
	LWPOLY *poly = lwpoly_construct_empty(s->has_z, s->has_m, s->srid);
	
	if( nrings == 0 )
		return poly;

	for( i = 0; i < nrings; i++ )
	{
		POINTARRAY pa = ptarray_from_wkb_state(s);
		if( pa == NULL )
			continue;
		lwpoly_add_ring(poly, pa);
	}
	return poly;
}

/**
* GEOMETRY
*/
static LWGEOM* lwgeom_from_wkb_state(wkb_parse_state *s)
{
	char wkb_little_endian;
	uint32 wkb_type;
	
	/* Fail when handed incorrect starting byte */
	wkb_little_endian = byte_from_wkb_state(s);
	if( wkb_little_endian != 1 && wkb_little_endian != 0 )
	{
		lwerror("Invalid endian flag value encountered.");
		return NULL;
	}

	/* Check the endianness of our input  */
	s->swap_bytes = LW_FALSE;
	if( BYTE_ORDER == LITTLE_ENDIAN ) /* Machine arch is little */
		if ( ! wkb_little_endian )    /* Data is big! */
			s->swap_bytes = LW_TRUE;
		else
			s->swap_bytes = LW_FALSE;
	else                           /* Machine arch is big */
		if ( wkb_little_endian )   /* Data is little! */
			s->swap_bytes = LW_TRUE;
		else
			s->swap_bytes = LW_FALSE;

	/* Read the type number */
	lwtype_from_wkb_state(s, integer_from_wkb_state(s));
	
	/* Read the SRID, if necessary */
	if( s->has_srid )
		s->srid = integer_from_wkb_state(s);
	
	/* Do the right thing */
	switch( s->lwtype )
	{
		case POINTTYPE:
			return (LWGEOM*)lwpoint_from_wkb_state(s);
			break;
		case LINETYPE:
			return (LWGEOM*)lwline_from_wkb_state(s);
			break;
		case POLYGONTYPE:
			return (LWGEOM*)lwpoly_from_wkb_state(s);
			break;

/** TODO TODO
* lw*_construct_empty() for all types
* add maxrings, maxpoints, maxgeoms to all lwtypes
* lwpoly_add_ring()
* lwcollection_add_geom()
* ptarray_add_point()
* reorganize header file for clarity
*/

	}
	
}


/**
* WKB inputs must have a declared size, to prevent malformed WKB from reading
* off the end of the memory segment (this stops a malevolent user from declaring
* a one-ring polygon to have 10 rings, causing the WKB reader to walk off the 
* end of the memory).
*
* PARSER_CHECK_MINPOINTS, PARSER_CHECK_ODD, PARSER_CHECK_CLOSURE, 
* PARSER_CHECK_NONE, PARSER_CHECK_ALL
*/
LWGEOM* lwgeom_from_wkb(const char *wkb, const size_t wkb_size, const char check)
{
	wkb_parse_state s;
	
	/* Initialize the state appropriately */
	s.wkb = wkb;
	s.wkb_size = wkb_size;
	s.swap_bytes = LW_FALSE;
	s.check = check;
	s.lwtype = 0;
	s.srid = 0;
	s.has_z = LW_FALSE;
	s.has_m = LW_FALSE;
	s.has_srid = LW_FALSE;
	s.pos = wkb;
	
	return lwgeom_from_wkb_state(&s);
}

