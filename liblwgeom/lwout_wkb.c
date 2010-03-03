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

static char* lwgeom_to_wkb_buf(const LWGEOM *geom, char *buf, uchar variant);
static size_t lwgeom_to_wkb_size(const LWGEOM *geom, uchar variant);

#define WKB_DOUBLE_SIZE 8
#define WKB_INT_SIZE 4
#define WKB_BYTE_SIZE 1

/**
* Look-up table for hex writer
*/
static char *hexchr = "0123456789ABCDEF";


/**
* Optional SRID
*/
static int lwgeom_wkb_needs_srid(const LWGEOM *geom, uchar variant)
{
	if ( (variant & WKB_EXTENDED) && lwgeom_has_srid(geom) )
		return LW_TRUE;
	return LW_FALSE;
}

/**
* GeometryType
*/
static unsigned int lwgeom_wkb_type(const LWGEOM *geom, uchar variant)
{
	unsigned int wkb_type = 0;

	uchar type = geom->type;
	
	switch( TYPE_GETTYPE(type) )
	{
		case POINTTYPE:
			wkb_type = 1;
			break;
		case LINETYPE:
			wkb_type = 2;
			break;
		case POLYGONTYPE:
			wkb_type = 3;
			break;
		case MULTIPOINTTYPE:
			wkb_type = 4;
			break;
		case MULTILINETYPE:
			wkb_type = 5;
			break;
		case MULTIPOLYGONTYPE:
			wkb_type = 6;
			break;
		case COLLECTIONTYPE:
			wkb_type = 7;
			break;
		case CIRCSTRINGTYPE:
			wkb_type = 8;
			break;
		case COMPOUNDTYPE:
			wkb_type = 9;
			break;
		case CURVEPOLYTYPE:
			wkb_type = 10;
			break;
		case MULTICURVETYPE:
			wkb_type = 11;
			break;
		case MULTISURFACETYPE:
			wkb_type = 12;
			break;
		default:
			lwerror("Unsupported geometry type: %s [%d]", lwgeom_typename(type), type);
	}
	
	if( variant & WKB_EXTENDED )
	{
		if ( TYPE_HASZ(type) )
			wkb_type |= WKBZOFFSET;
		if ( TYPE_HASM(type) )
			wkb_type |= WKBMOFFSET;
		if ( lwgeom_has_srid(geom) )
			wkb_type |= WKBSRIDFLAG;
	}
	else if( variant & WKB_ISO )
	{
		/* Z types are in the 1000 range */
		if ( TYPE_HASZ(type) )
			wkb_type += 1000;
		/* M types are in the 2000 range */
		if ( TYPE_HASM(type) )
			wkb_type += 2000;
		/* ZM types are in the 1000 + 2000 = 3000 range */
	}	
	return wkb_type;
}

/**
* Endian
*/
static char* endian_to_wkb_buf(char *buf, uchar variant)
{
	if( variant & WKB_HEX )
	{
		buf[0] = '0';
		buf[1] = ((variant & WKB_NDR) ? '1' : '0');
		return buf + 2;
	}
	else 
	{
		buf[0] = ((variant & WKB_NDR) ? 1 : 0);
		return buf + 1;
	}
}

/**
* SwapBytes?
*/
static int wkb_swap_bytes(uchar variant)
{
	/* If requested variant matches machine arch, we don't have to swap! */
	if( ((variant & WKB_NDR) && (BYTE_ORDER == LITTLE_ENDIAN)) ||
	    ((! (variant & WKB_NDR)) && (BYTE_ORDER == BIG_ENDIAN)) )
	{
		return LW_FALSE;
	}
	return LW_TRUE;
}

/**
* Integer32
*/
static char* int32_to_wkb_buf(const int ival, char *buf, uchar variant)
{
	char *iptr = (char*)(&ival);
	int i = 0;
	
	if( sizeof(int) != WKB_INT_SIZE )
	{
		lwerror("Machine int size is not %d bytes!", WKB_INT_SIZE);
	}
	LWDEBUGF(4, "Writing value '%d'", ival);
	if( variant & WKB_HEX )
	{
		int swap =  wkb_swap_bytes(variant);
		/* Machine/request arch mismatch, so flip byte order */
		for( i = 0; i < WKB_INT_SIZE; i++ )
		{
			int j = (swap ? WKB_INT_SIZE - 1 - i : i);
			uchar b = iptr[j];
			/* Top four bits to 0-F */
			buf[2*i] = hexchr[b >> 4];
			/* Bottom four bits to 0-F */
			buf[2*i+1] = hexchr[b & 0x0F];			
		}
		return buf + (2 * WKB_INT_SIZE);
	}
	else
	{
		/* Machine/request arch mismatch, so flip byte order */
		if( wkb_swap_bytes(variant) )
		{
			for ( i = 0; i < WKB_INT_SIZE; i++ )
			{
				buf[i] = iptr[WKB_INT_SIZE - 1 - i];
			}
		}
		/* If machine arch and requested arch match, don't flip byte order */
		else
		{
			memcpy(buf, iptr, WKB_INT_SIZE);
		}
		return buf + WKB_INT_SIZE;
	}
}

/**
* Float64
*/
static char* double_to_wkb_buf(const double d, char *buf, uchar variant)
{
	char *dptr = (char*)(&d);
	int i = 0;
	
	if( sizeof(double) != WKB_DOUBLE_SIZE )
	{
		lwerror("Machine double size is not %d bytes!", WKB_DOUBLE_SIZE);
	}
	
	if( variant & WKB_HEX )
	{
		int swap =  wkb_swap_bytes(variant);
		/* Machine/request arch mismatch, so flip byte order */
		for( i = 0; i < WKB_DOUBLE_SIZE; i++ )
		{
			int j = (swap ? WKB_DOUBLE_SIZE - 1 - i : i);
			uchar b = dptr[j];
			/* Top four bits to 0-F */
			buf[2*i] = hexchr[b >> 4];
			/* Bottom four bits to 0-F */
			buf[2*i+1] = hexchr[b & 0x0F];			
		}
		return buf + (2 * WKB_DOUBLE_SIZE);
	}
	else
	{
		/* Machine/request arch mismatch, so flip byte order */
		if( wkb_swap_bytes(variant) )
		{
			for ( i = 0; i < WKB_DOUBLE_SIZE; i++ )
			{
				buf[i] = dptr[WKB_DOUBLE_SIZE - 1 - i];
			}
		}
		/* If machine arch and requested arch match, don't flip byte order */
		else
		{
			memcpy(buf, dptr, WKB_DOUBLE_SIZE);
		}
		return buf + WKB_DOUBLE_SIZE;
	}
}


/**
* Empty
*/
static size_t empty_to_wkb_size(const LWGEOM *geom, uchar variant)
{
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE + WKB_INT_SIZE;

	if( lwgeom_wkb_needs_srid(geom, variant) )
		size += WKB_INT_SIZE;
		
	return size;
}

static char* empty_to_wkb_buf(const LWGEOM *geom, char *buf, uchar variant)
{
	unsigned int wkb_type = lwgeom_wkb_type(geom, variant);

	if( TYPE_GETTYPE(geom->type) == POINTTYPE )
		wkb_type += 3; /* Change POINT to MULTIPOINT */

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);

	/* Set the geometry type */
	buf = int32_to_wkb_buf(wkb_type, buf, variant);

	/* Set the SRID if necessary */
	if( lwgeom_wkb_needs_srid(geom, variant) )
		buf = int32_to_wkb_buf(geom->SRID, buf, variant);

	/* Set nrings/npoints/ngeoms to zero */
	buf = int32_to_wkb_buf(0, buf, variant);
	return buf;
}

/**
* POINTARRAY
*/
static size_t ptarray_to_wkb_size(const POINTARRAY *pa, uchar variant)
{
	int dims = 2;
	size_t size = 0;
	if( pa->npoints < 1 )
		return 0;
	if( variant & (WKB_ISO | WKB_EXTENDED) )
		dims = TYPE_NDIMS(pa->dims);

	/* Include the npoints if it's not a POINT type) */
	if( ! ( variant & WKB_NO_NPOINTS ) )
		size += WKB_INT_SIZE;

	/* size of the double list */
	size += pa->npoints * dims * WKB_DOUBLE_SIZE;

	return size;
}

static char* ptarray_to_wkb_buf(const POINTARRAY *pa, char *buf, uchar variant)
{
	int dims = 2;
	int i, j;
	double *dbl_ptr;
	
	if( pa->npoints < 1 )
		return buf;
	if( (variant & WKB_ISO) || (variant & WKB_EXTENDED) )
		dims = TYPE_NDIMS(pa->dims);

	/* Set the number of points (if it's not a POINT type) */
	if( ! ( variant & WKB_NO_NPOINTS ) )
		buf = int32_to_wkb_buf(pa->npoints, buf, variant);
	
	/* Set the ordinates. */
	/* TODO: Ensure that getPoint_internal is always aligned so 
	         this doesn't fail on RiSC architectures */
	/* TODO: Make this faster by bulk copying the coordinates when
	         the output endian/dims match the internal endian/dims */
	for( i = 0; i < pa->npoints; i++ )
	{
		LWDEBUGF(4, "Writing point #%d", i);
		dbl_ptr = (double*)getPoint_internal(pa, i);
		for( j = 0; j < dims; j++ )
		{
			LWDEBUGF(4, "Writing dimension #%d (buf = %p)", j, buf);
			buf = double_to_wkb_buf(dbl_ptr[j], buf, variant);
		}
	}
	LWDEBUGF(4, "Done (buf = %p)", buf);	
	return buf;
}

/**
* POINT
*/
static size_t lwpoint_to_wkb_size(const LWPOINT *pt, uchar variant)
{
	/* Endian flag + type number */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE;

	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)pt, variant) )
		size += WKB_INT_SIZE;

	/* Points */
	size += ptarray_to_wkb_size(pt->point, variant | WKB_NO_NPOINTS);
	return size;
}

static char* lwpoint_to_wkb_buf(const LWPOINT *pt, char *buf, uchar variant)
{
	/* Set the endian flag */
	LWDEBUGF(4, "Entering function, buf = %p", buf);
	buf = endian_to_wkb_buf(buf, variant);
	LWDEBUGF(4, "Endian set, buf = %p", buf);
	/* Set the geometry type */
	buf = int32_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)pt, variant), buf, variant);
	LWDEBUGF(4, "Type set, buf = %p", buf);
	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)pt, variant) )
	{
		buf = int32_to_wkb_buf(pt->SRID, buf, variant);
		LWDEBUGF(4, "SRID set, buf = %p", buf);
	}
	/* Set the coordinates */
	buf = ptarray_to_wkb_buf(pt->point, buf, variant | WKB_NO_NPOINTS);
	LWDEBUGF(4, "Pointarray set, buf = %p", buf);
	return buf;
}

/**
* LINESTRING, CIRCULARSTRING
*/
static size_t lwline_to_wkb_size(const LWLINE *line, uchar variant)
{
	/* Endian flag + type number */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE;
	
	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)line, variant) )
		size += WKB_INT_SIZE;
			
	/* Size of point array */
	size += ptarray_to_wkb_size(line->points, variant);
	return size;
}

static char* lwline_to_wkb_buf(const LWLINE *line, char *buf, uchar variant)
{
	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = int32_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)line, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)line, variant) )
		buf = int32_to_wkb_buf(line->SRID, buf, variant);
	/* Set the coordinates */
	buf = ptarray_to_wkb_buf(line->points, buf, variant);
	return buf;
}

/**
* POLYGON
*/
static size_t lwpoly_to_wkb_size(const LWPOLY *poly, uchar variant)
{
	/* endian flag + type number + number of rings */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE + WKB_INT_SIZE;
	int i = 0;
	
	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)poly, variant) )
		size += WKB_INT_SIZE;
			
	for( i = 0; i < poly->nrings; i++ )
	{
		/* Size of ring point array */
		size += ptarray_to_wkb_size(poly->rings[i], variant);
	}
	
	return size;
}

static char* lwpoly_to_wkb_buf(const LWPOLY *poly, char *buf, uchar variant)
{
	int i;

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = int32_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)poly, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)poly, variant) )
		buf = int32_to_wkb_buf(poly->SRID, buf, variant);
	/* Set the number of rings */
	buf = int32_to_wkb_buf(poly->nrings, buf, variant);
	
	for( i = 0; i < poly->nrings; i++ )
	{
		buf = ptarray_to_wkb_buf(poly->rings[i], buf, variant);
	}
	
	return buf;
}

/**
* MULTIPOINT, MULTILINESTRING, MULTIPOLYGON, GEOMETRYCOLLECTION
* MULTICURVE, COMPOUNDCURVE, MULTISURFACE, CURVEPOLYGON
*/
static size_t lwcollection_to_wkb_size(const LWCOLLECTION *col, uchar variant)
{
	/* Endian flag + type number + number of subgeoms */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE + WKB_INT_SIZE;
	int i = 0;
	
	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)col, variant) )
		size += WKB_INT_SIZE;
		
	for( i = 0; i < col->ngeoms; i++ )
	{
		/* size of subgeom */
		size += lwgeom_to_wkb_size((LWGEOM*)col->geoms[i], variant);
	}
	
	return size;
}

static char* lwcollection_to_wkb_buf(const LWCOLLECTION *col, char *buf, uchar variant)
{
	int i;

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = int32_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)col, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)col, variant) )
		buf = int32_to_wkb_buf(col->SRID, buf, variant);
	/* Set the number of sub-geometries */
	buf = int32_to_wkb_buf(col->ngeoms, buf, variant);
	
	for( i = 0; i < col->ngeoms; i++ )
	{
		buf = lwgeom_to_wkb_buf(col->geoms[i], buf, variant);
	}
	
	return buf;
}

/**
* GEOMETRY
*/
static size_t lwgeom_to_wkb_size(const LWGEOM *geom, uchar variant)
{
	size_t size = 0;
	
	if ( geom == NULL ) 
		return 0;

	/* Short circuit out empty geometries */
	if ( lwgeom_is_empty(geom) )
	{
		return empty_to_wkb_size(geom, variant);
	}
			
	switch( TYPE_GETTYPE(geom->type) )
	{
		case POINTTYPE:
			size += lwpoint_to_wkb_size((LWPOINT*)geom, variant);
			break;

		/* LineString and CircularString both have points elements */
		case CIRCSTRINGTYPE:
		case LINETYPE:
			size += lwline_to_wkb_size((LWLINE*)geom, variant);
			break;

		/* Polygon has nrings and rings elements */
		case POLYGONTYPE:
			size += lwpoly_to_wkb_size((LWPOLY*)geom, variant);
			break;

		/* All these Collection types have ngeoms and geoms elements */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COMPOUNDTYPE:
		case CURVEPOLYTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case COLLECTIONTYPE:
			size += lwcollection_to_wkb_size((LWCOLLECTION*)geom, variant);
			break;

		/* Unknown type! */
		default:
			lwerror("Unsupported geometry type: %s [%d]", lwgeom_typename(geom->type), TYPE_GETTYPE(geom->type));
	}

	return size;
}

static char* lwgeom_to_wkb_buf(const LWGEOM *geom, char *buf, uchar variant)
{

	if ( lwgeom_is_empty(geom) )
		return empty_to_wkb_buf(geom, buf, variant);

	switch( TYPE_GETTYPE(geom->type) )
	{
		case POINTTYPE:
			return lwpoint_to_wkb_buf((LWPOINT*)geom, buf, variant);

		/* LineString and CircularString both have 'points' elements */
		case CIRCSTRINGTYPE:
		case LINETYPE:
			return lwline_to_wkb_buf((LWLINE*)geom, buf, variant);

		/* Polygon has 'nrings' and 'rings' elements */
		case POLYGONTYPE:
			return lwpoly_to_wkb_buf((LWPOLY*)geom, buf, variant);

		/* All these Collection types have 'ngeoms' and 'geoms' elements */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COMPOUNDTYPE:
		case CURVEPOLYTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case COLLECTIONTYPE:
			return lwcollection_to_wkb_buf((LWCOLLECTION*)geom, buf, variant);

		/* Unknown type! */
		default:
			lwerror("Unsupported geometry type: %s [%d]", lwgeom_typename(geom->type), TYPE_GETTYPE(geom->type));
	}
	/* Return value to keep compiler happy. */
	return 0;
}

/**
* Convert LWGEOM to a char* in WKB format. Caller is responsible for freeing
* the returned array.
*
* Accepts variants:
* One of: WKB_ISO, WKB_EXTENDED, WKB_SFSQL
* Any of: WKB_NDR
* For example, a variant = ( WKT_ISO | WKT_NDR ) would return the little-endian 
* ISO form of WKB.
*/
char* lwgeom_to_wkb(const LWGEOM *geom, uchar variant, size_t *size_out)
{
	size_t buf_size;
	char *buf = NULL;
	char *wkb_out = NULL;
	
	/* Initialize output size */
	if( size_out ) *size_out = 0;
	
	if( geom == NULL ) 
	{
		LWDEBUG(4,"Cannot convert NULL into WKB.");
		lwerror("Cannot convert NULL into WKB.");
		return NULL;
	}
	
	/* Calculate the required size of the output buffer */
	buf_size = lwgeom_to_wkb_size(geom, variant);
	LWDEBUGF(4, "WKB output size: %d", buf_size);

	if( buf_size == 0 )
	{
		LWDEBUG(4,"Error calculating output WKB buffer size.");
		lwerror("Error calculating output WKB buffer size.");
		return NULL;
	}

	/* Hex string takes twice as much space as binary + a null character */
	if( variant & WKB_HEX )
	{
		buf_size = 2 * buf_size + 1;
		LWDEBUGF(4, "Hex WKB output size: %d", buf_size);
	}

	/* Allocate the buffer */
	buf = lwalloc(buf_size);
	
	if( buf == NULL )
	{
		LWDEBUGF(4,"Unable to allocate %d bytes for WKB output buffer.", buf_size);
		lwerror("Unable to allocate %d bytes for WKB output buffer.", buf_size);
		return NULL;
	}
		
	/* Retain a pointer to the front of the buffer for later */
	wkb_out = buf;
	
	/* Write the WKB into the output buffer */
	buf = lwgeom_to_wkb_buf(geom, buf, variant);
	
	/* Null the last byte if this is a hex output */
	*buf = '\0';
	buf++;
	
	LWDEBUGF(4,"buf (%p) - wkb_out (%p) = %d", buf, wkb_out, buf - wkb_out);

	/* The buffer pointer should now land at the end of the allocated buffer space. Let's check. */
	if( buf_size != (buf - wkb_out) )
	{
		LWDEBUG(4,"Output WKB is not the same size as the allocated buffer.");
		lwerror("Output WKB is not the same size as the allocated buffer.");
		lwfree(wkb_out);
		return NULL;
	}
	
	return wkb_out;
}
