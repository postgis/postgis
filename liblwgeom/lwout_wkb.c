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
 * Copyright (C) 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/


#include <math.h>
#include <stddef.h> // for ptrdiff_t

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

static uint8_t* lwgeom_to_wkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant);
static size_t lwgeom_to_wkb_size(const LWGEOM *geom, uint8_t variant);

/*
* Look-up table for hex writer
*/
static char *hexchr = "0123456789ABCDEF";

char* hexbytes_from_bytes(const uint8_t *bytes, size_t size)
{
	char *hex;
	uint32_t i;
	if ( ! bytes || ! size )
	{
		lwerror("hexbutes_from_bytes: invalid input");
		return NULL;
	}
	hex = lwalloc(size * 2 + 1);
	hex[2*size] = '\0';
	for( i = 0; i < size; i++ )
	{
		/* Top four bits to 0-F */
		hex[2*i] = hexchr[bytes[i] >> 4];
		/* Bottom four bits to 0-F */
		hex[2*i+1] = hexchr[bytes[i] & 0x0F];
	}
	return hex;
}

/*
* Optional SRID
*/
static int lwgeom_wkb_needs_srid(const LWGEOM *geom, uint8_t variant)
{
	/* Sub-components of collections inherit their SRID from the parent.
	   We force that behavior with the WKB_NO_SRID flag */
	if ( variant & WKB_NO_SRID )
		return LW_FALSE;

	/* We can only add an SRID if the geometry has one, and the
	   WKB form is extended */
	if ( (variant & WKB_EXTENDED) && lwgeom_has_srid(geom) )
		return LW_TRUE;

	/* Everything else doesn't get an SRID */
	return LW_FALSE;
}

/**
 * Compute the WKB geometry type code for a given LWGEOM and WKB variant.
 *
 * Maps the internal geometry type to the appropriate WKB type code and
 * applies dimension and SRID encoding offsets/flags according to the
 * selected variant. NURBSCURVETYPE always uses ISO-style dimension offsets
 * (1000/2000/3000). For non-NURBS geometries:
 * - WKB_EXTENDED sets Z/M bit flags and may set the SRID flag (determined
 *   by lwgeom_wkb_needs_srid).
 * - WKB_ISO adds numeric dimension offsets (1000 for Z, 2000 for M).
 *
 * If the geometry type is unsupported the function logs an error and
 * returns 0.
 *
 * @param geom Geometry to encode (must be non-NULL).
 * @param variant Bitflags describing WKB variant/encoding (e.g. WKB_EXTENDED, WKB_ISO, WKB_NO_SRID).
 * @return WKB geometry type code with any applicable dimension/SRID modifiers.
 */
static uint32_t lwgeom_wkb_type(const LWGEOM *geom, uint8_t variant)
{
	uint32_t wkb_type = 0;

	switch ( geom->type )
	{
	case POINTTYPE:
		wkb_type = WKB_POINT_TYPE;
		break;
	case LINETYPE:
		wkb_type = WKB_LINESTRING_TYPE;
		break;
	case NURBSCURVETYPE:
		wkb_type = WKB_NURBSCURVE_TYPE;
		break;
	case POLYGONTYPE:
		wkb_type = WKB_POLYGON_TYPE;
		break;
	case MULTIPOINTTYPE:
		wkb_type = WKB_MULTIPOINT_TYPE;
		break;
	case MULTILINETYPE:
		wkb_type = WKB_MULTILINESTRING_TYPE;
		break;
	case MULTIPOLYGONTYPE:
		wkb_type = WKB_MULTIPOLYGON_TYPE;
		break;
	case COLLECTIONTYPE:
		wkb_type = WKB_GEOMETRYCOLLECTION_TYPE;
		break;
	case CIRCSTRINGTYPE:
		wkb_type = WKB_CIRCULARSTRING_TYPE;
		break;
	case COMPOUNDTYPE:
		wkb_type = WKB_COMPOUNDCURVE_TYPE;
		break;
	case CURVEPOLYTYPE:
		wkb_type = WKB_CURVEPOLYGON_TYPE;
		break;
	case MULTICURVETYPE:
		wkb_type = WKB_MULTICURVE_TYPE;
		break;
	case MULTISURFACETYPE:
		wkb_type = WKB_MULTISURFACE_TYPE;
		break;
	case POLYHEDRALSURFACETYPE:
		wkb_type = WKB_POLYHEDRALSURFACE_TYPE;
		break;
	case TINTYPE:
		wkb_type = WKB_TIN_TYPE;
		break;
	case TRIANGLETYPE:
		wkb_type = WKB_TRIANGLE_TYPE;
		break;
	default:
		lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(geom->type));
	}

	/* NURBS curves always use ISO dimension encoding */
	if ( geom->type == NURBSCURVETYPE )
	{
		if ( FLAGS_GET_Z(geom->flags) && !FLAGS_GET_M(geom->flags) )
			wkb_type += 1000;  /* Z: 1000 offset */
		else if ( FLAGS_GET_M(geom->flags) && !FLAGS_GET_Z(geom->flags) )
			wkb_type += 2000;  /* M: 2000 offset */
		else if ( FLAGS_GET_Z(geom->flags) && FLAGS_GET_M(geom->flags) )
			wkb_type += 3000;  /* ZM: 3000 offset */
	}
	else if ( variant & WKB_EXTENDED )
	{
		if ( FLAGS_GET_Z(geom->flags) )
			wkb_type |= WKBZOFFSET;
		if ( FLAGS_GET_M(geom->flags) )
			wkb_type |= WKBMOFFSET;
/*		if ( geom->srid != SRID_UNKNOWN && ! (variant & WKB_NO_SRID) ) */
		if ( lwgeom_wkb_needs_srid(geom, variant) )
			wkb_type |= WKBSRIDFLAG;
	}
	else if ( variant & WKB_ISO )
	{
		/* ISO encoding for other geometry types */
		if ( FLAGS_GET_Z(geom->flags) )
			wkb_type += 1000;
		if ( FLAGS_GET_M(geom->flags) )
			wkb_type += 2000;
	}
	return wkb_type;
}

/*
* Endian
*/
static uint8_t* endian_to_wkb_buf(uint8_t *buf, uint8_t variant)
{
	if ( variant & WKB_HEX )
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

/*
* SwapBytes?
*/
static inline int wkb_swap_bytes(uint8_t variant)
{
	/* If requested variant matches machine arch, we don't have to swap! */
	if (((variant & WKB_NDR) && !IS_BIG_ENDIAN) ||
	    ((!(variant & WKB_NDR)) && IS_BIG_ENDIAN))
	{
		return LW_FALSE;
	}
	return LW_TRUE;
}

/**
 * Write a 32-bit unsigned integer into a WKB buffer honoring variant (binary or hex)
 * and requested endianness, and return the pointer to the next write position.
 *
 * The function writes the 32-bit value `ival` into `buf` using WKB_INT_SIZE bytes
 * (or twice that many ASCII hex characters when WKB_HEX is set in `variant`).
 * If the requested byte order differs from the machine's native order the byte
 * order is swapped before writing. On mismatch between sizeof(int) and
 * WKB_INT_SIZE an error is logged.
 *
 * @param ival  The 32-bit unsigned integer to encode.
 * @param buf   Destination buffer where encoded bytes (or hex chars) are written.
 *              Must have space for at least WKB_INT_SIZE bytes (or 2*WKB_INT_SIZE
 *              chars for hex variants).
 * @param variant Bitmask specifying WKB encoding variant (may include WKB_HEX and
 *                endianness flags). Controls hex vs binary output and byte order.
 * @return Pointer into `buf` immediately after the bytes written.
 */
static uint8_t *
integer_to_wkb_buf(const uint32_t ival, uint8_t *buf, uint8_t variant)
{
	uint8_t *iptr = (uint8_t *)(&ival);
	int i = 0;

	if ( sizeof(int) != WKB_INT_SIZE )
	{
		lwerror("Machine int size is not %d bytes!", WKB_INT_SIZE);
	}
	LWDEBUGF(4, "Writing value '%u'", ival);
	if ( variant & WKB_HEX )
	{
		int swap = wkb_swap_bytes(variant);
		/* Machine/request arch mismatch, so flip byte order */
		for ( i = 0; i < WKB_INT_SIZE; i++ )
		{
			int j = (swap ? WKB_INT_SIZE - 1 - i : i);
			uint8_t b = iptr[j];
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
		if ( wkb_swap_bytes(variant) )
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
 * Write a single byte into a WKB buffer, using hex-encoding when requested.
 *
 * If the WKB_HEX bit is set in variant, writes two ASCII hex characters
 * representing the byte and returns buf + 2; otherwise writes the raw byte
 * and returns buf + 1.
 *
 * @param bval Byte value to write.
 * @param buf Destination buffer to write into; must have at least 2 bytes if hex encoding is used.
 * @param variant Bitmask of WKB variant flags (tests WKB_HEX).
 * @returns Pointer to the next write position in buf after the written data.
 */
static uint8_t* byte_to_wkb_buf(const uint8_t bval, uint8_t *buf, uint8_t variant)
{
	if ( variant & WKB_HEX )
	{
		/* Convert single byte to 2 hex characters */
		buf[0] = hexchr[bval >> 4];      /* Top four bits to 0-F */
		buf[1] = hexchr[bval & 0x0F];    /* Bottom four bits to 0-F */
		return buf + 2;
	}
	else
	{
		buf[0] = bval;
		return buf + 1;
	}
}

/**
 * Write an IEEE-754 NaN double into a WKB buffer.
 *
 * Writes an 8-byte NaN pattern to buf using either little-endian (NDR) or
 * big-endian (XDR) byte order and either binary or hex-encoded output,
 * controlled by flags in variant (WKB_NDR and WKB_HEX). Advances and returns
 * a pointer to the position immediately after the written data.
 *
 * @param buf Buffer to write into; must have space for 8 raw bytes or 16 hex bytes.
 * @param variant Bitmask of WKB flags (may include WKB_NDR for NDR vs XDR and
 *                WKB_HEX to request hex-encoding).
 * @return Pointer to buf advanced past the written NaN bytes (buf + 8 for binary,
 *         buf + 16 for hex).
 */
static uint8_t* double_nan_to_wkb_buf(uint8_t *buf, uint8_t variant)
{
#define NAN_SIZE 8
	const uint8_t ndr_nan[NAN_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f};
	const uint8_t xdr_nan[NAN_SIZE] = {0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	if ( variant & WKB_HEX )
	{
		for (int i = 0; i < NAN_SIZE; i++)
		{
			uint8_t b = (variant & WKB_NDR) ? ndr_nan[i] : xdr_nan[i];
			/* Top four bits to 0-F */
			buf[2*i] = hexchr[b >> 4];
			/* Bottom four bits to 0-F */
			buf[2*i + 1] = hexchr[b & 0x0F];
		}
		return buf + (2 * NAN_SIZE);
	}
	else
	{
		for (int i = 0; i < NAN_SIZE; i++)
		{
			buf[i] = (variant & WKB_NDR) ? ndr_nan[i] : xdr_nan[i];;
		}
		return buf + NAN_SIZE;
	}
}

/*
* Float64
*/
static uint8_t* double_to_wkb_buf(const double d, uint8_t *buf, uint8_t variant)
{
	uint8_t *dptr = (uint8_t *)(&d);
	int i = 0;

	if ( sizeof(double) != WKB_DOUBLE_SIZE )
	{
		lwerror("Machine double size is not %d bytes!", WKB_DOUBLE_SIZE);
	}

	if ( variant & WKB_HEX )
	{
		int swap =  wkb_swap_bytes(variant);
		/* Machine/request arch mismatch, so flip byte order */
		for ( i = 0; i < WKB_DOUBLE_SIZE; i++ )
		{
			int j = (swap ? WKB_DOUBLE_SIZE - 1 - i : i);
			uint8_t b = dptr[j];
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
		if ( wkb_swap_bytes(variant) )
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


/*
* Empty
*/
static size_t empty_to_wkb_size(const LWGEOM *geom, uint8_t variant)
{
	/* endian byte + type integer */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE;

	/* optional srid integer */
	if ( lwgeom_wkb_needs_srid(geom, variant) )
		size += WKB_INT_SIZE;

	/* Represent POINT EMPTY as POINT(NaN NaN) */
	if ( geom->type == POINTTYPE )
	{
		const LWPOINT *pt = (LWPOINT*)geom;
		size += WKB_DOUBLE_SIZE * FLAGS_NDIMS(pt->point->flags);
	}
	/* num-elements */
	else
	{
		size += WKB_INT_SIZE;
	}

	return size;
}

static uint8_t* empty_to_wkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant)
{
	uint32_t wkb_type = lwgeom_wkb_type(geom, variant);

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);

	/* Set the geometry type */
	buf = integer_to_wkb_buf(wkb_type, buf, variant);

	/* Set the SRID if necessary */
	if ( lwgeom_wkb_needs_srid(geom, variant) )
		buf = integer_to_wkb_buf(geom->srid, buf, variant);

	/* Represent POINT EMPTY as POINT(NaN NaN) */
	if ( geom->type == POINTTYPE )
	{
		const LWPOINT *pt = (LWPOINT*)geom;
		for (int i = 0; i < FLAGS_NDIMS(pt->point->flags); i++)
		{
			buf = double_nan_to_wkb_buf(buf, variant);
		}
	}
	/* Everything else is flagged as empty using num-elements == 0 */
	else
	{
		/* Set nrings/npoints/ngeoms to zero */
		buf = integer_to_wkb_buf(0, buf, variant);
	}

	return buf;
}

/*
* POINTARRAY
*/
static size_t ptarray_to_wkb_size(const POINTARRAY *pa, uint8_t variant)
{
	int dims = 2;
	size_t size = 0;

	if ( variant & (WKB_ISO | WKB_EXTENDED) )
		dims = FLAGS_NDIMS(pa->flags);

	/* Include the npoints if it's not a POINT type) */
	if ( ! ( variant & WKB_NO_NPOINTS ) )
		size += WKB_INT_SIZE;

	/* size of the double list */
	size += (size_t)pa->npoints * dims * WKB_DOUBLE_SIZE;

	return size;
}

static uint8_t* ptarray_to_wkb_buf(const POINTARRAY *pa, uint8_t *buf, uint8_t variant)
{
	uint32_t dims = 2;
	uint32_t pa_dims = FLAGS_NDIMS(pa->flags);
	uint32_t i, j;
	double *dbl_ptr;

	/* SFSQL is always 2-d. Extended and ISO use all available dimensions */
	if ( (variant & WKB_ISO) || (variant & WKB_EXTENDED) )
		dims = pa_dims;

	/* Set the number of points (if it's not a POINT type) */
	if ( ! ( variant & WKB_NO_NPOINTS ) )
		buf = integer_to_wkb_buf(pa->npoints, buf, variant);

	/* Bulk copy the coordinates when: dimensionality matches, output format */
	/* is not hex, and output endian matches internal endian. */
	if ( pa->npoints && (dims == pa_dims) && ! wkb_swap_bytes(variant) && ! (variant & WKB_HEX)  )
	{
		size_t size = (size_t)pa->npoints * dims * WKB_DOUBLE_SIZE;
		memcpy(buf, getPoint_internal(pa, 0), size);
		buf += size;
	}
	/* Copy coordinates one-by-one otherwise */
	else
	{
		for ( i = 0; i < pa->npoints; i++ )
		{
			LWDEBUGF(4, "Writing point #%d", i);
			dbl_ptr = (double*)getPoint_internal(pa, i);
			for ( j = 0; j < dims; j++ )
			{
				LWDEBUGF(4, "Writing dimension #%d (buf = %p)", j, buf);
				buf = double_to_wkb_buf(dbl_ptr[j], buf, variant);
			}
		}
	}
	LWDEBUGF(4, "Done (buf = %p)", buf);
	return buf;
}

/*
* POINT
*/
static size_t lwpoint_to_wkb_size(const LWPOINT *pt, uint8_t variant)
{
	/* Endian flag + type number */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & WKB_EXTENDED) && lwgeom_is_empty((LWGEOM*)pt) )
		return empty_to_wkb_size((LWGEOM*)pt, variant);

	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)pt, variant) )
		size += WKB_INT_SIZE;

	/* Points */
	size += ptarray_to_wkb_size(pt->point, variant | WKB_NO_NPOINTS);
	return size;
}

static uint8_t* lwpoint_to_wkb_buf(const LWPOINT *pt, uint8_t *buf, uint8_t variant)
{
	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & WKB_EXTENDED) && lwgeom_is_empty((LWGEOM*)pt) )
		return empty_to_wkb_buf((LWGEOM*)pt, buf, variant);

	/* Set the endian flag */
	LWDEBUGF(4, "Entering function, buf = %p", buf);
	buf = endian_to_wkb_buf(buf, variant);
	LWDEBUGF(4, "Endian set, buf = %p", buf);
	/* Set the geometry type */
	buf = integer_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)pt, variant), buf, variant);
	LWDEBUGF(4, "Type set, buf = %p", buf);
	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)pt, variant) )
	{
		buf = integer_to_wkb_buf(pt->srid, buf, variant);
		LWDEBUGF(4, "SRID set, buf = %p", buf);
	}
	/* Set the coordinates */
	buf = ptarray_to_wkb_buf(pt->point, buf, variant | WKB_NO_NPOINTS);
	LWDEBUGF(4, "Pointarray set, buf = %p", buf);
	return buf;
}

/*
* LINESTRING, CIRCULARSTRING
*/
static size_t lwline_to_wkb_size(const LWLINE *line, uint8_t variant)
{
	/* Endian flag + type number */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & WKB_EXTENDED) && lwgeom_is_empty((LWGEOM*)line) )
		return empty_to_wkb_size((LWGEOM*)line, variant);

	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)line, variant) )
		size += WKB_INT_SIZE;

	/* Size of point array */
	size += ptarray_to_wkb_size(line->points, variant);
	return size;
}

static uint8_t* lwline_to_wkb_buf(const LWLINE *line, uint8_t *buf, uint8_t variant)
{
	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & WKB_EXTENDED) && lwgeom_is_empty((LWGEOM*)line) )
		return empty_to_wkb_buf((LWGEOM*)line, buf, variant);

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = integer_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)line, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)line, variant) )
		buf = integer_to_wkb_buf(line->srid, buf, variant);
	/* Set the coordinates */
	buf = ptarray_to_wkb_buf(line->points, buf, variant);
	return buf;
}

/*
* TRIANGLE
*/
static size_t lwtriangle_to_wkb_size(const LWTRIANGLE *tri, uint8_t variant)
{
	/* endian flag + type number + number of rings */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE + WKB_INT_SIZE;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & WKB_EXTENDED) && lwgeom_is_empty((LWGEOM*)tri) )
		return empty_to_wkb_size((LWGEOM*)tri, variant);

	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)tri, variant) )
		size += WKB_INT_SIZE;

	/* How big is this point array? */
	size += ptarray_to_wkb_size(tri->points, variant);

	return size;
}

static uint8_t* lwtriangle_to_wkb_buf(const LWTRIANGLE *tri, uint8_t *buf, uint8_t variant)
{
	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & WKB_EXTENDED) && lwgeom_is_empty((LWGEOM*)tri) )
		return empty_to_wkb_buf((LWGEOM*)tri, buf, variant);

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);

	/* Set the geometry type */
	buf = integer_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)tri, variant), buf, variant);

	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)tri, variant) )
		buf = integer_to_wkb_buf(tri->srid, buf, variant);

	/* Set the number of rings (only one, it's a triangle, buddy) */
	buf = integer_to_wkb_buf(1, buf, variant);

	/* Write that ring */
	buf = ptarray_to_wkb_buf(tri->points, buf, variant);

	return buf;
}

/*
* POLYGON
*/
static size_t lwpoly_to_wkb_size(const LWPOLY *poly, uint8_t variant)
{
	/* endian flag + type number + number of rings */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE + WKB_INT_SIZE;
	uint32_t i = 0;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & WKB_EXTENDED) && lwgeom_is_empty((LWGEOM*)poly) )
		return empty_to_wkb_size((LWGEOM*)poly, variant);

	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)poly, variant) )
		size += WKB_INT_SIZE;

	for ( i = 0; i < poly->nrings; i++ )
	{
		/* Size of ring point array */
		size += ptarray_to_wkb_size(poly->rings[i], variant);
	}

	return size;
}

static uint8_t* lwpoly_to_wkb_buf(const LWPOLY *poly, uint8_t *buf, uint8_t variant)
{
	uint32_t i;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & WKB_EXTENDED) && lwgeom_is_empty((LWGEOM*)poly) )
		return empty_to_wkb_buf((LWGEOM*)poly, buf, variant);

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = integer_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)poly, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)poly, variant) )
		buf = integer_to_wkb_buf(poly->srid, buf, variant);
	/* Set the number of rings */
	buf = integer_to_wkb_buf(poly->nrings, buf, variant);

	for ( i = 0; i < poly->nrings; i++ )
	{
		buf = ptarray_to_wkb_buf(poly->rings[i], buf, variant);
	}

	return buf;
}


/*
* MULTIPOINT, MULTILINESTRING, MULTIPOLYGON, GEOMETRYCOLLECTION
* MULTICURVE, COMPOUNDCURVE, MULTISURFACE, CURVEPOLYGON, TIN,
* POLYHEDRALSURFACE
*/
static size_t lwcollection_to_wkb_size(const LWCOLLECTION *col, uint8_t variant)
{
	/* Endian flag + type number + number of subgeoms */
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE + WKB_INT_SIZE;
	uint32_t i = 0;

	/* Extended WKB needs space for optional SRID integer */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)col, variant) )
		size += WKB_INT_SIZE;

	for ( i = 0; i < col->ngeoms; i++ )
	{
		/* size of subgeom */
		size += lwgeom_to_wkb_size((LWGEOM*)col->geoms[i], variant | WKB_NO_SRID);
	}

	return size;
}

/**
 * Write a geometry collection into a WKB buffer.
 *
 * Encodes the collection header (endianness, geometry type, optional SRID,
 * and number of sub-geometries) and then serializes each contained geometry.
 * Sub-geometries inherit the parent's SRID and are written with WKB_NO_SRID.
 *
 * @param col Collection to serialize.
 * @param buf Pointer to the current write position in the output buffer; the
 *            function advances this pointer as it writes.
 * @param variant Bitmask selecting WKB variant/flags (e.g. NDR/XDR, HEX,
 *                EXTENDED, ISO). Controls SRID inclusion and encoding form.
 * @return Updated buffer pointer positioned immediately after the written data.
 */
static uint8_t* lwcollection_to_wkb_buf(const LWCOLLECTION *col, uint8_t *buf, uint8_t variant)
{
	uint32_t i;

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = integer_to_wkb_buf(lwgeom_wkb_type((LWGEOM*)col, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( lwgeom_wkb_needs_srid((LWGEOM*)col, variant) )
		buf = integer_to_wkb_buf(col->srid, buf, variant);
	/* Set the number of sub-geometries */
	buf = integer_to_wkb_buf(col->ngeoms, buf, variant);

	/* Write the sub-geometries. Sub-geometries do not get SRIDs, they
	   inherit from their parents. */
	for ( i = 0; i < col->ngeoms; i++ )
	{
		buf = lwgeom_to_wkb_buf(col->geoms[i], buf, variant | WKB_NO_SRID);
	}

	return buf;
}

/**
 * Compute the number of bytes required to encode a NURBS curve as WKB (ISO/IEC 13249-3:2016).
 *
 * The computed size accounts for:
 * - endianness marker and geometry type,
 * - optional SRID integer when the variant requests it,
 * - degree and control-point count,
 * - per-control-point data: a byte-order marker, coordinate doubles for active dimensions,
 *   a one-byte "weight present" flag, and an optional double weight when the point's weight != 1.0,
 * - knot-count integer and knot double values (a uniform knot vector is generated if the curve has none).
 *
 * The caller must pass a valid non-NULL LWNURBSCURVE pointer. The variant argument influences
 * whether an SRID integer is included (extended/ISO handling).
 *
 * @param curve NURBS curve to size (must be non-NULL).
 * @param variant WKB variant flags that determine SRID inclusion and encoding options.
 * @return number of bytes required to write the curve in WKB form.
 */
static size_t lwnurbscurve_to_wkb_size(const LWNURBSCURVE *curve, uint8_t variant)
{
    size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE; /* endian + type */

    /* Extended WKB needs space for optional SRID integer */
    if ( lwgeom_wkb_needs_srid((LWGEOM*)curve, variant) )
        size += WKB_INT_SIZE;

    /* ISO/IEC 13249-3:2016 compliant structure:
     * <byte order> <wkbnurbs> [ <wkbdegree> <wkbcontrolpoints binary> <wkbknots binary> ]
     */

    size += WKB_INT_SIZE; /* degree */

    /* Control points count */
    size += WKB_INT_SIZE; /* npoints */

    uint32_t npoints = curve->points ? curve->points->npoints : 0;
    uint32_t dims = curve->points ? FLAGS_NDIMS(curve->points->flags) : 2;

    /* ISO format: Each control point has individual weight structure
     * <nurbspoint binary representation> ::=
     * <byte order> [ <wkbweightedpoint> <bit> [ <wkbweight> ] ]
     */
    for (uint32_t i = 0; i < npoints; i++) {
        size += WKB_BYTE_SIZE; /* byte order for each point */
        size += dims * WKB_DOUBLE_SIZE; /* point coordinates */
        size += WKB_BYTE_SIZE; /* weight bit flag */

        /* Add weight value if this point has a custom weight */
        if (curve->weights && i < curve->nweights && curve->weights[i] != 1.0) {
            size += WKB_DOUBLE_SIZE; /* weight value */
        }
    }

    /* Knots are always required in WKB output (generate uniform if not present) */
    size += WKB_INT_SIZE; /* nknots count */
    uint32_t nknots_for_size = 0;
    double *knots_for_size = lwnurbscurve_get_knots_for_wkb(curve, &nknots_for_size);
    if (knots_for_size) {
			size += WKB_DOUBLE_SIZE * nknots_for_size;
			lwfree(knots_for_size); /* Just needed the count */
		} else if (nknots_for_size > 0) {
			/* Inconsistent state - got count but no array */
			lwerror("Failed to get knots for NURBS curve");
			return 0;
 		}

    return size;
}

/**
 * Encode a NURBS curve into WKB (ISO/IEC 13249-3:2016) and write into a buffer.
 *
 * Produces an ISO-compliant WKB representation for a LWNURBSCURVE:
 * - writes byte-order marker and geometry type, and optional SRID for extended variants;
 * - writes curve degree and control-point count;
 * - for each control point writes a per-point byte-order marker, coordinates (per point dimensionality),
 *   a single-byte weight-presence flag, and the weight value when present (default weight = 1.0 is omitted);
 * - writes the knot vector length followed by knot values (requests knots via lwnurbscurve_get_knots_for_wkb;
 *   if none are returned a zero knot-count is written as a fallback).
 *
 * The function advances the provided buffer pointer as it writes. It may allocate a temporary knot array
 * from lwnurbscurve_get_knots_for_wkb and frees it before returning.
 *
 * @param curve NURBS curve to encode.
 * @param buf   Pointer to the output buffer where WKB bytes (or hex characters, depending on variant) are written.
 * @param variant Bitmask selecting WKB variant/encoding options (endianness, extended/ISO/hex modes).
 * @return Updated buffer pointer positioned immediately after the last written byte/character.
 */
static uint8_t* lwnurbscurve_to_wkb_buf(const LWNURBSCURVE *curve, uint8_t *buf, uint8_t variant)
{
    uint32_t wkb_type = lwgeom_wkb_type((LWGEOM*)curve, variant);

    /* Write endian flag */
    buf = endian_to_wkb_buf(buf, variant);

    /* Write type */
    buf = integer_to_wkb_buf(wkb_type, buf, variant);

    /* Set the optional SRID for extended variant */
    if ( lwgeom_wkb_needs_srid((LWGEOM*)curve, variant) )
        buf = integer_to_wkb_buf(curve->srid, buf, variant);

    /* ISO/IEC 13249-3:2016 compliant structure */
    buf = integer_to_wkb_buf(curve->degree, buf, variant);

    /* Write control points count */
    uint32_t npoints = curve->points ? curve->points->npoints : 0;
    uint32_t dims = curve->points ? FLAGS_NDIMS(curve->points->flags) : 2;
    buf = integer_to_wkb_buf(npoints, buf, variant);

    /* ISO format: Write each control point with individual weight structure
     * <nurbspoint binary representation> ::=
     * <byte order> [ <wkbweightedpoint> <bit> [ <wkbweight> ] ]
     */
    for (uint32_t i = 0; i < npoints; i++) {
        /* Write byte order for this point (ISO requirement) */
        buf = endian_to_wkb_buf(buf, variant);

        /* Write point coordinates */
        if (curve->points) {
            double *coords = (double*)getPoint_internal(curve->points, i);
            for (uint32_t d = 0; d < dims; d++) {
                buf = double_to_wkb_buf(coords[d], buf, variant);
            }
        }

        /* Write weight bit flag */
        uint8_t has_weight = 0;
        double weight_value = 1.0; /* default weight */

        if (curve->weights && i < curve->nweights && curve->weights[i] != 1.0) {
            has_weight = 1;
            weight_value = curve->weights[i];
        }

        buf = byte_to_wkb_buf(has_weight, buf, variant);

        /* Write weight value if flag = 1 */
        if (has_weight) {
            buf = double_to_wkb_buf(weight_value, buf, variant);
        }
    }

    /* Write knots (always required - generate uniform if not present) */
    {
        uint32_t nknots = 0;
        double *knots = lwnurbscurve_get_knots_for_wkb(curve, &nknots);
        if (knots && nknots > 0) {
            buf = integer_to_wkb_buf(nknots, buf, variant);
            for (uint32_t i = 0; i < nknots; i++) {
                buf = double_to_wkb_buf(knots[i], buf, variant);
            }
            lwfree(knots);
        } else {
            /* Fallback - should not happen */
            buf = integer_to_wkb_buf(0, buf, variant);
        }
    }

    return buf;
}

/**
 * Compute the number of bytes required to encode a geometry as WKB for a given variant.
 *
 * Determines the exact buffer size needed to write the provided LWGEOM in the specified
 * WKB variant (handles ISO/extended/hex encodings, SRID inclusion, and per-geometry
 * element sizing). Empty geometries are handled by the dedicated empty-to-WKB sizing
 * unless the extended variant bit is set, in which case extended encoding rules apply.
 *
 * @param geom Pointer to the geometry to size; returns 0 and logs an error if NULL.
 * @param variant Bitmask of WKB variant flags (e.g., WKB_EXTENDED, WKB_HEX, NDR/XDR)
 *        that control dimensionality, SRID inclusion, and encoding form.
 * @return Number of bytes required to encode `geom` under `variant`, or 0 on error.
 */
static size_t
lwgeom_to_wkb_size(const LWGEOM *geom, uint8_t variant)
{
	size_t size = 0;

	if (geom == NULL)
	{
		LWDEBUG(4, "Cannot convert NULL into WKB.");
		lwerror("Cannot convert NULL into WKB.");
		return 0;
	}

	/* Short circuit out empty geometries */
	if ( (!(variant & WKB_EXTENDED)) && lwgeom_is_empty(geom) )
	{
		return empty_to_wkb_size(geom, variant);
	}

	switch ( geom->type )
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

		/* Triangle has one ring of three points */
		case TRIANGLETYPE:
			size += lwtriangle_to_wkb_size((LWTRIANGLE*)geom, variant);
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
		case POLYHEDRALSURFACETYPE:
		case TINTYPE:
			size += lwcollection_to_wkb_size((LWCOLLECTION*)geom, variant);
			break;
		case NURBSCURVETYPE:
			size += lwnurbscurve_to_wkb_size((LWNURBSCURVE*)geom, variant);
			break;
		/* Unknown type! */
		default:
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(geom->type));
	}

	return size;
}

/**
 * Serialize a LWGEOM into WKB format, writing into the provided buffer.
 *
 * Dispatches to the appropriate type-specific writer based on geom->type.
 * For empty geometries, if the WKB_EXTENDED flag is not set the geometry is
 * written using the canonical "empty" representation; otherwise empties are
 * preserved and written by the type-specific writer.
 *
 * @param geom Geometry to serialize (must be non-NULL).
 * @param buf Destination buffer where WKB bytes will be written. The function
 *            writes starting at this pointer and returns the pointer advanced
 *            past the written data.
 * @param variant Bitflags controlling WKB flavor (e.g., WKB_EXTENDED, WKB_HEX,
 *                WKB_NDR/WKB_XDR). These flags influence SRID inclusion, hex
 *                vs binary output, and other variant-specific encoding rules.
 * @return Pointer to the buffer position immediately after the written WKB on
 *         success; NULL (0) on unsupported/unknown geometry type or other error.
 */

static uint8_t* lwgeom_to_wkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant)
{

	/* Do not simplify empties when outputting to canonical form */
	if (lwgeom_is_empty(geom) && !(variant & WKB_EXTENDED))
		return empty_to_wkb_buf(geom, buf, variant);

	switch ( geom->type )
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

		/* Triangle has one ring of three points */
		case TRIANGLETYPE:
			return lwtriangle_to_wkb_buf((LWTRIANGLE*)geom, buf, variant);

		/* All these Collection types have 'ngeoms' and 'geoms' elements */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COMPOUNDTYPE:
		case CURVEPOLYTYPE:
		case MULTICURVETYPE:
		case MULTISURFACETYPE:
		case COLLECTIONTYPE:
		case POLYHEDRALSURFACETYPE:
		case TINTYPE:
			return lwcollection_to_wkb_buf((LWCOLLECTION*)geom, buf, variant);
		case NURBSCURVETYPE:
			return lwnurbscurve_to_wkb_buf((LWNURBSCURVE*)geom, buf, variant);

		/* Unknown type! */
		default:
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(geom->type));
	}
	/* Return value to keep compiler happy. */
	return 0;
}

/**
* Convert LWGEOM to a char* in WKB format. Caller is responsible for freeing
* the returned array.
*
* @param variant. Unsigned bitmask value. Accepts one of: WKB_ISO, WKB_EXTENDED, WKB_SFSQL.
* Accepts any of: WKB_NDR, WKB_HEX. For example: Variant = ( WKB_ISO | WKB_NDR ) would
* return the little-endian ISO form of WKB. For Example: Variant = ( WKB_EXTENDED | WKB_HEX )
* would return the big-endian extended form of WKB, as hex-encoded ASCII (the "canonical form").
* @param size_out If supplied, will return the size of the returned memory segment,
* including the null terminator in the case of ASCII.
*/
static ptrdiff_t
lwgeom_to_wkb_write_buf(const LWGEOM *geom, uint8_t variant, uint8_t *buffer)
{
	/* If neither or both variants are specified, choose the native order */
	if (!(variant & WKB_NDR || variant & WKB_XDR) || (variant & WKB_NDR && variant & WKB_XDR))
	{
		if (IS_BIG_ENDIAN)
			variant = variant | WKB_XDR;
		else
			variant = variant | WKB_NDR;
	}

	/* Write the WKB into the output buffer */
	int written_bytes = (lwgeom_to_wkb_buf(geom, buffer, variant) - buffer);

	return written_bytes;
}

uint8_t *
lwgeom_to_wkb_buffer(const LWGEOM *geom, uint8_t variant)
{
	size_t b_size = lwgeom_to_wkb_size(geom, variant);
	/* Hex string takes twice as much space as binary + a null character */
	if (variant & WKB_HEX)
	{
		b_size = 2 * b_size + 1;
	}

	uint8_t *buffer = (uint8_t *)lwalloc(b_size);
	ptrdiff_t written_size = lwgeom_to_wkb_write_buf(geom, variant, buffer);
	if (variant & WKB_HEX)
	{
		buffer[written_size] = '\0';
		written_size++;
	}

	if (written_size != (ptrdiff_t)b_size)
	{
		char *wkt = lwgeom_to_wkt(geom, WKT_EXTENDED, 15, NULL);
		lwerror("Output WKB is not the same size as the allocated buffer. Variant: %u, Geom: %s", variant, wkt);
		lwfree(wkt);
		lwfree(buffer);
		return NULL;
	}

	return buffer;
}

char *
lwgeom_to_hexwkb_buffer(const LWGEOM *geom, uint8_t variant)
{
	return (char *)lwgeom_to_wkb_buffer(geom, variant | WKB_HEX);
}

lwvarlena_t *
lwgeom_to_wkb_varlena(const LWGEOM *geom, uint8_t variant)
{
	size_t b_size = lwgeom_to_wkb_size(geom, variant);
	/* Hex string takes twice as much space as binary, but No NULL ending in varlena */
	if (variant & WKB_HEX)
	{
		b_size = 2 * b_size;
	}

	lwvarlena_t *buffer = (lwvarlena_t *)lwalloc(b_size + LWVARHDRSZ);
	int written_size = lwgeom_to_wkb_write_buf(geom, variant, (uint8_t *)buffer->data);
	if (written_size != (ptrdiff_t)b_size)
	{
		char *wkt = lwgeom_to_wkt(geom, WKT_EXTENDED, 15, NULL);
		lwerror("Output WKB is not the same size as the allocated buffer. Variant: %u, Geom: %s", variant, wkt);
		lwfree(wkt);
		lwfree(buffer);
		return NULL;
	}
	LWSIZE_SET(buffer->size, written_size + LWVARHDRSZ);
	return buffer;
}

lwvarlena_t *
lwgeom_to_hexwkb_varlena(const LWGEOM *geom, uint8_t variant)
{
	return lwgeom_to_wkb_varlena(geom, variant | WKB_HEX);
}
