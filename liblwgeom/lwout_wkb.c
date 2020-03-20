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

/*
* GeometryType
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

	if ( variant & WKB_EXTENDED )
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
		/* Z types are in the 1000 range */
		if ( FLAGS_GET_Z(geom->flags) )
			wkb_type += 1000;
		/* M types are in the 2000 range */
		if ( FLAGS_GET_M(geom->flags) )
			wkb_type += 2000;
		/* ZM types are in the 1000 + 2000 = 3000 range, see above */
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

/*
* Integer32
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
	size += pa->npoints * dims * WKB_DOUBLE_SIZE;

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
		size_t size = pa->npoints * dims * WKB_DOUBLE_SIZE;
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

/*
* GEOMETRY
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

		/* Unknown type! */
		default:
			lwerror("%s: Unsupported geometry type: %s", __func__, lwtype_name(geom->type));
	}

	return size;
}

/* TODO handle the TRIANGLE type properly */

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
