/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 *
 * Copyright (C) 2013 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwout_twkb.h"
#include "varint.h"

/*
* GeometryType
*/
static uint8_t lwgeom_twkb_type(const LWGEOM *geom, uint8_t variant)
{
	LWDEBUGF(2, "Entered  lwgeom_twkb_type",0);
	uint8_t wkb_type = 0;
	uint8_t type_flag = 0;
	int dims;
	
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
		lwerror("Unsupported geometry type: %s [%d]",
			lwtype_name(geom->type), geom->type);
	}

	/*Set the type*/
	TYPE_DIM_SET_TYPE(type_flag,wkb_type);	
	LWDEBUGF(4, "Writing type '%d'", wkb_type);
	
	/*Set number of dimensions*/
	dims = FLAGS_NDIMS(geom->flags);
	if (dims>4)
		lwerror("TWKB only supports 4 dimensions");	
	TYPE_DIM_SET_DIM(type_flag,dims);	
	LWDEBUGF(4, "Writing ndims '%d'", dims);
	
	return type_flag;
}


/**
Function for putting a Byte value into the buffer
*/
static int uint8_to_twkb_buf(const uint8_t ival, uint8_t **buf)
{
	LWDEBUGF(2, "Entered  uint8_to_twkb_buf",0);	
	LWDEBUGF(4, "Writing value %d",ival);
	memcpy(*buf, &ival, WKB_BYTE_SIZE);
	(*buf)++;
	return 0;
}

/*
* Empty
*/
static size_t empty_to_twkb_size(const LWGEOM *geom, uint8_t variant, int64_t id) 
{
	LWDEBUGF(2, "Entered  empty_to_twkb_size",0);
	size_t size=0;
	/*size of ID*/
	size += varint_s64_encoded_size((int64_t) id);
	/*size of npoints*/
	size += varint_u64_encoded_size((uint64_t) 0);

	return size;
}

static int empty_to_twkb_buf(const LWGEOM *geom, uint8_t **buf, uint8_t variant, int64_t id)
{
	LWDEBUGF(2, "Entered  empty_to_twkb_buf",0);
	uint32_t wkb_type = lwgeom_twkb_type(geom, variant);
	if ( geom->type == POINTTYPE )
	{
		/* Change POINT to MULTIPOINT */
		wkb_type &= ~WKB_POINT_TYPE;     /* clear POINT flag */
		wkb_type |= WKB_MULTIPOINT_TYPE; /* set MULTIPOINT flag */
	}
	uint8_t flag=0;
	
	/*Copy the flag to the buffer*/
	uint8_to_twkb_buf(flag,buf);
	
	/* Set the geometry id */	
	varint_s64_encode_buf(id, buf);
	
	/* Set the geometry type */	
	uint8_to_twkb_buf(wkb_type,buf);	
	
	/* Set nrings/npoints/ngeoms to zero */
	varint_u64_encode_buf(0, buf);
	return 0;
}

/**
Calculates the size of the bbox in varInts in the form:
xmin, xmax, deltax, deltay
*/
static size_t sizeof_bbox(int dims,int64_t min_values[], int64_t max_values[])
{
	int i;
	size_t size = 0;
	for (i=0;i<dims;i++)
	{
		size+=varint_s64_encoded_size(min_values[i]);
		size+=varint_s64_encoded_size((max_values[i]-min_values[i]));
	}	
	return size;
}
/**
Writes the bbox in varInts in the form:
xmin, xmax, deltax, deltay
*/
static void write_bbox(uint8_t **buf,int dims,int64_t min_values[], int64_t max_values[])
{
	int i;
	for (i=0;i<dims;i++)
	{
		varint_s64_encode_buf(min_values[i],buf);
		varint_s64_encode_buf((max_values[i]-min_values[i]),buf);
	}
	
}

/**
Calculates the needed space for storing a specific pointarray as varInt-encoded
*/
static size_t ptarray_to_twkb_size(const POINTARRAY *pa, uint8_t variant,int factor,int64_t accum_rel[3][4])
{
	LWDEBUGF(2, "Entered ptarray_to_twkb_size_m1",0);
	int64_t r;
	int dims = FLAGS_NDIMS(pa->flags);
	size_t size = 0;
	int j,i;
	double *dbl_ptr;
	/*The variable factor is used to "shift" the double float coordinate to keep enough significant digits, 
	for demanded precision, when cast to integer*/
//	factor=pow(10,prec);
	/* Include the npoints size if it's not a POINT type) */
	if ( ! ( variant & WKB_NO_NPOINTS ) )
	{
		LWDEBUGF(4, "We add space for npoints",0);
		size+=varint_u64_encoded_size(pa->npoints);
	}

	for ( i = 0; i < pa->npoints; i++ )
	{
		dbl_ptr = (double*)getPoint_internal(pa, i);
		for ( j = 0; j < dims; j++ )
		{
			LWDEBUGF(4, "dim nr %d, refvalue is %d",j, accum_rel[0][j]);
			r=(int64_t) lround(factor*dbl_ptr[j]-accum_rel[0][j]);
			accum_rel[0][j]+=r;		
			if(variant&TWKB_BBOXES)
			{
				if(accum_rel[0][j]<accum_rel[1][j])
					accum_rel[1][j]=accum_rel[0][j];
				if(accum_rel[0][j]>accum_rel[2][j])
					accum_rel[2][j]=accum_rel[0][j];
			}
			size += varint_s64_encoded_size((int64_t) r);
			LWDEBUGF(4, "deltavalue = %d and resulting refvalue is %d",r,accum_rel[0][j]);
		}
	}
	return size;
}


/**
Stores a pointarray as varInts in the buffer
*/
static int ptarray_to_twkb_buf(const POINTARRAY *pa, uint8_t **buf, uint8_t variant,int64_t factor,int64_t accum_rel[])
{
	LWDEBUGF(2, "entered ptarray_to_twkb_buf_m1\n",0);
	int64_t r;
	int dims = FLAGS_NDIMS(pa->flags);
	int i, j;
	double *dbl_ptr;
	//factor=pow(10,prec);
	
	
		
	/* Set the number of points (if it's not a POINT type) */
	if ( ! ( variant & WKB_NO_NPOINTS ) )
	{
		varint_u64_encode_buf(pa->npoints,buf);
		LWDEBUGF(4, "Register npoints:%d",pa->npoints);	
	}

	for ( i = 0; i < pa->npoints; i++ )
	{
		LWDEBUGF(4, "Writing point #%d", i);
		dbl_ptr = (double*)getPoint_internal(pa, i);
		for ( j = 0; j < dims; j++ )
		{
			/*To get the relative coordinate we don't get the distance from the last point
			but instead the distance from our accumulated last point
			This is important to not build up a accumulated error when rounding the coordinates*/				
			r=(int64_t) lround(factor*dbl_ptr[j]-accum_rel[j]);		
			LWDEBUGF(4, "deltavalue: %d, ",r );				
			accum_rel[j]+=r;
			varint_s64_encode_buf(r,buf);
		}
	}	
	//LWDEBUGF(4, "Done (buf = %p)", buf);
	return 0;
}

/******************************************************************
POINTS
*******************************************************************/
/**
Calculates needed storage size for aggregated points
*/
static size_t  lwgeom_agg_to_twkbpoint_size(lwgeom_id *geom_array,uint8_t variant,int n,int64_t factor,int64_t refpoint[3][4])
{
	lwgeom_id *li;
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of geometries*/
	size += varint_u64_encoded_size((uint64_t) n);

	int i;
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		size += lwpoint_to_twkb_size((LWPOINT*) (li->geom),variant,factor,li->id,refpoint);
	}
	return size;
}
/**
Calculates needed storage size for a point
*/
static size_t lwpoint_to_twkb_size(const LWPOINT *pt,uint8_t variant, int64_t factor, int64_t id,int64_t refpoint[3][4])
{
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & TWKB_ID)
	size	 += varint_s64_encoded_size((int64_t) id);

	/* Points */
	size += ptarray_to_twkb_size(pt->point, variant | WKB_NO_NPOINTS, factor,refpoint);
	return size;
}


/**
Iterates an aggregation of points
*/
static int lwgeom_agg_to_twkbpoint_buf(lwgeom_id* geom_array,int n, uint8_t **buf, uint8_t variant,int64_t factor, int64_t refpoint[3][4])
{

	lwgeom_id *li;
	uint8_t type_flag = 0;
	int i,dims;
	/*Set the type*/
	TYPE_DIM_SET_TYPE(type_flag,21);	
	LWDEBUGF(4, "Writing type '%d'", 21);
	
	/*Set number of dimensions*/
	dims = FLAGS_NDIMS(((LWGEOM*) (geom_array->geom))->flags);
	if (dims>4)
		lwerror("TWKB only supports 4 dimensions");	
	TYPE_DIM_SET_DIM(type_flag,dims);	
	LWDEBUGF(4, "Writing ndims '%d'", dims);
	uint8_to_twkb_buf(type_flag,buf);
	
	if(variant&TWKB_BBOXES)
	{		
		write_bbox(buf,dims,refpoint[1],refpoint[2]);
		//So we only write bboxes to highest level
		variant = variant & ~TWKB_BBOXES;
	}
	
	/* Set number of geometries */
	varint_u64_encode_buf(n, buf);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		lwpoint_to_twkb_buf((LWPOINT*) (li->geom),buf,variant,factor,li->id,refpoint[0]);

	}
	return 0;
}

/**
Sends a point to the buffer
*/
static int lwpoint_to_twkb_buf(const LWPOINT *pt, uint8_t **buf, uint8_t variant,int64_t factor, int64_t id,int64_t refpoint[])
{

	
	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & TWKB_ID)
		varint_s64_encode_buf(id, buf);	
	
		
	/* Set the coordinates */
	ptarray_to_twkb_buf(pt->point, buf, variant | WKB_NO_NPOINTS,factor,refpoint);
	LWDEBUGF(4, "Pointarray set, buf = %p", buf);
	return 0;
}

/******************************************************************
LINESTRINGS
*******************************************************************/
/**
Calculates needed storage size for aggregated lines
*/
static size_t  lwgeom_agg_to_twkbline_size(lwgeom_id* geom_array,uint8_t variant,int n,int64_t factor,int64_t refpoint[3][4])
{
	lwgeom_id *li;
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of lines*/
	size += varint_u64_encoded_size((uint64_t) n);
	int i;
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		size += lwline_to_twkb_size((LWLINE*) (li->geom),variant,factor,li->id,refpoint);
	}
	return size;
}
/**
Calculates needed storage size for a line
*/
static size_t lwline_to_twkb_size(const LWLINE *line,uint8_t variant, int64_t factor, int64_t id,int64_t refpoint[3][4])
{	
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & TWKB_ID)
	size	 += varint_s64_encoded_size((int64_t) id);

	/* Size of point array */
	size += ptarray_to_twkb_size(line->points,variant,factor,refpoint);
	return size;
}
/**
Iterates an aggregation of lines
*/
static int lwgeom_agg_to_twkbline_buf(lwgeom_id* geom_array,int n, uint8_t **buf, uint8_t variant,int64_t factor, int64_t refpoint[3][4])
{

	lwgeom_id *li;
	uint8_t type_flag = 0;
	int i,dims;
	/*Set the type*/
	TYPE_DIM_SET_TYPE(type_flag,22);	
	LWDEBUGF(4, "Writing type '%d'",22);
	
	/*Set number of dimensions*/
	dims = FLAGS_NDIMS(((LWGEOM*) (geom_array->geom))->flags);
	if (dims>4)
		lwerror("TWKB only supports 4 dimensions");	
	TYPE_DIM_SET_DIM(type_flag,dims);	
	LWDEBUGF(4, "Writing ndims '%d'", dims);
	uint8_to_twkb_buf(type_flag,buf);
		
	if(variant&TWKB_BBOXES)
	{		
		write_bbox(buf,dims,refpoint[1],refpoint[2]);
		//So we only write bboxes to highest level
		variant = variant & ~TWKB_BBOXES;
	}
	
	/* Set number of geometries */
	varint_u64_encode_buf(n, buf);
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		lwline_to_twkb_buf((LWLINE*) li->geom,buf,variant,factor,li->id,refpoint[0]);
	}
	return 0;
}

/**
Sends a line to the buffer
*/
static int lwline_to_twkb_buf(const LWLINE *line, uint8_t **buf, uint8_t variant,int64_t factor, int64_t id,int64_t refpoint[])
{

	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & TWKB_ID)
		varint_s64_encode_buf(id, buf);				

	/* Set the coordinates */
	ptarray_to_twkb_buf(line->points, buf, variant,factor,refpoint);
	return 0;
}

/******************************************************************
POLYGONS
*******************************************************************/
/**
Calculates needed storage size for aggregated polygon
*/
static size_t  lwgeom_agg_to_twkbpoly_size(lwgeom_id* geom_array,uint8_t variant,int n,int64_t factor,int64_t refpoint[3][4])
{
	lwgeom_id *li;
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of collections*/
	size +=varint_u64_encoded_size((uint64_t) n);
	int i;
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		size += lwpoly_to_twkb_size((LWPOLY*) (li->geom),variant,factor,li->id,refpoint);
	}
	return size;
}

/**
Calculates needed storage size for a polygon
*/
static size_t lwpoly_to_twkb_size(const LWPOLY *poly,uint8_t variant, int64_t factor, int64_t id,int64_t refpoint[3][4])
{
	LWDEBUGF(2, "lwpoly_to_twkb_size entered%d",0);
	int i;	
	
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & TWKB_ID)
		size	 += varint_s64_encoded_size((int64_t) id);
	
	/*nrings*/
	size += varint_u64_encoded_size((uint64_t) poly->nrings);
	
	LWDEBUGF(2, "we have %d rings to iterate",poly->nrings);
	for ( i = 0; i < poly->nrings; i++ )
	{
		/* Size of ring point array */
		size += ptarray_to_twkb_size(poly->rings[i],variant,factor,refpoint);
	}

	return size;
}

/**
Iterates an aggregation of polygons
*/
static int lwgeom_agg_to_twkbpoly_buf(lwgeom_id* geom_array,int n, uint8_t **buf, uint8_t variant,int64_t factor, int64_t refpoint[3][4])
{

	lwgeom_id *li;
	uint8_t type_flag = 0;
	int i,dims;
	/*Set the type*/
	TYPE_DIM_SET_TYPE(type_flag,23);	
	LWDEBUGF(4, "Writing type '%d'", 23);
	
	/*Set number of dimensions*/
	dims = FLAGS_NDIMS(((LWGEOM*) (geom_array->geom))->flags);
	if (dims>4)
		lwerror("TWKB only supports 4 dimensions");	
	TYPE_DIM_SET_DIM(type_flag,dims);	
	LWDEBUGF(4, "Writing ndims '%d'", dims);
	uint8_to_twkb_buf(type_flag,buf);
	
			
	if(variant&TWKB_BBOXES)
	{		
		write_bbox(buf,dims,refpoint[1],refpoint[2]);
		//So we only write bboxes to highest level
		variant = variant & ~TWKB_BBOXES;
	}
	
	
	/* Set number of geometries */
	varint_u64_encode_buf(n, buf);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		lwpoly_to_twkb_buf((LWPOLY*) (li->geom),buf,variant,factor,li->id,refpoint[0]);
	}
	return 0;
}

/**
Sends a polygon to the buffer
*/
static int lwpoly_to_twkb_buf(const LWPOLY *poly, uint8_t **buf, uint8_t variant,int64_t factor, int64_t id,int64_t refpoint[])
{
	int i;
	
	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & TWKB_ID)
		varint_s64_encode_buf(id, buf);	

	/* Set the number of rings */
	varint_u64_encode_buf(poly->nrings, buf);
	
	for ( i = 0; i < poly->nrings; i++ )
	{
		ptarray_to_twkb_buf(poly->rings[i], buf, variant,factor,refpoint);
	}

	return 0;
}

/******************************************************************
COLLECTIONS
*******************************************************************/
/**
Calculates needed storage size for aggregated collection
*/
static size_t  lwgeom_agg_to_twkbcollection_size(lwgeom_id* geom_array,uint8_t variant,int n,int64_t factor,int64_t refpoint[3][4])
{
	lwgeom_id *li;
	LWDEBUGF(4, "lwgeom_agg_to_twkbcollection_size entered with %d collections",n);
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of collections*/
	size += varint_u64_encoded_size((uint64_t) n);
	int i;
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		size += lwcollection_to_twkb_size((LWCOLLECTION*) (li->geom),variant,factor,li->id,refpoint);
	}
	return size;
}

/**
Calculates needed storage size for a collection
*/
static size_t lwcollection_to_twkb_size(const LWCOLLECTION *col,uint8_t variant, int64_t factor, int64_t id,int64_t refpoint[3][4])
{
	LWDEBUGF(2, "lwcollection_to_twkb_size entered, %d",0);
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & TWKB_ID)
		size	 += varint_s64_encoded_size((int64_t) id);
	/* size of geoms */
	size += varint_u64_encoded_size((uint64_t) col->ngeoms); 
	int i = 0;

	for ( i = 0; i < col->ngeoms; i++ )
	{
		/* size of subgeom */
		size += lwgeom_to_twkb_size((LWGEOM*)col->geoms[i],variant & ~TWKB_ID, factor,id,refpoint);
	}

	return size;
}

/**
Iterates an aggregation of collections
*/
static int lwgeom_agg_to_twkbcollection_buf(lwgeom_id* geom_array,int n, uint8_t **buf, uint8_t variant,int64_t factor, int64_t refpoint[3][4])
{

	lwgeom_id *li;
	uint8_t type_flag = 0;
	int i,dims;
	/*Set the type*/
	TYPE_DIM_SET_TYPE(type_flag,24);	
	LWDEBUGF(4, "Writing type '%d'", 24);
	
	/*Set number of dimensions*/
	dims = FLAGS_NDIMS(((LWGEOM*) (geom_array->geom))->flags);
	if (dims>4)
		lwerror("TWKB only supports 4 dimensions");	
	TYPE_DIM_SET_DIM(type_flag,dims);	
	LWDEBUGF(4, "Writing ndims '%d'", dims);
	uint8_to_twkb_buf(type_flag,buf);
	
			
	if(variant&TWKB_BBOXES)
	{		
		write_bbox(buf,dims,refpoint[1],refpoint[2]);
		//So we only write bboxes to highest level
		variant = variant & ~TWKB_BBOXES;
	}
	
	/* Set number of geometries */
	varint_u64_encode_buf(n, buf);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		lwcollection_to_twkb_buf((LWCOLLECTION*) li->geom,buf,variant,factor,li->id,refpoint);
	}
	return 0;
}

/**
Iterates a collection
*/
static int lwcollection_to_twkb_buf(const LWCOLLECTION *col, uint8_t **buf, uint8_t variant,int64_t factor, int64_t id,int64_t refpoint[3][4])
{
	int i;

	
	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & TWKB_ID)
		varint_s64_encode_buf(id, buf);	

	/* Set the number of rings */
	varint_u64_encode_buf(col->ngeoms, buf);
	/* Write the sub-geometries. Sub-geometries do not get SRIDs, they
	   inherit from their parents. */
	for ( i = 0; i < col->ngeoms; i++ )
	{
		lwgeom_to_twkb_buf(col->geoms[i], buf, variant & ~TWKB_ID,factor,id,refpoint);
	}

	return 0;
}


/******************************************************************
Handle whole TWKB
*******************************************************************/

/**
Calculates the needed space for a geometry as twkb
*/
static size_t lwgeom_to_twkb_size(const LWGEOM *geom,uint8_t variant, int64_t factor, int64_t id,int64_t refpoint[3][4])
{
	LWDEBUGF(2, "lwgeom_to_twkb_size entered %d",0);
	size_t size = 0;
	if ( geom == NULL )
		return 0;

	/* Short circuit out empty geometries */
	if ( lwgeom_is_empty(geom) )
	{
		return empty_to_twkb_size(geom, variant,id);
	}
	/*add size of type-declaration*/
	if (!(variant &  TWKB_NO_TYPE))
		size += WKB_BYTE_SIZE;
	switch ( geom->type )
	{
		case POINTTYPE:
			size += lwpoint_to_twkb_size((LWPOINT*)geom, variant, factor,id,refpoint);
			break;

		/* LineString and CircularString both have points elements */
		case CIRCSTRINGTYPE:
		case LINETYPE:
			size += lwline_to_twkb_size((LWLINE*)geom, variant, factor,id,refpoint);
			break;

		/* Polygon has nrings and rings elements */
		case POLYGONTYPE:
			size += lwpoly_to_twkb_size((LWPOLY*)geom, variant, factor,id,refpoint);
			break;

		/* Triangle has one ring of three points 
		case TRIANGLETYPE:
			size += lwtriangle_to_twkb_size((LWTRIANGLE*)geom, variant);
			break;*/

		/* All these Collection types have ngeoms and geoms elements */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
			size += lwcollection_to_twkb_size((LWCOLLECTION*)geom, variant | TWKB_NO_TYPE, factor,id,refpoint);
			break;
		case COLLECTIONTYPE:
			size += lwcollection_to_twkb_size((LWCOLLECTION*)geom, variant, factor,id,refpoint);
			break;

		/* Unknown type! */
		default:
			lwerror("Unsupported geometry type: %s [%d]", lwtype_name(geom->type), geom->type);
	}

	return size;
}


static int lwgeom_to_twkb_buf(const LWGEOM *geom, uint8_t **buf, uint8_t variant,int64_t factor, int64_t id,int64_t refpoint[3][4])
{

	if ( lwgeom_is_empty(geom) )
		return empty_to_twkb_buf(geom, buf, variant,id);

	if (!(variant &  TWKB_NO_TYPE))
	{
		uint8_to_twkb_buf(lwgeom_twkb_type(geom, variant),buf);
		
		if(variant&TWKB_BBOXES)
		{		
			write_bbox(buf,FLAGS_NDIMS(geom->flags),refpoint[1],refpoint[2]);
			//So we only write bboxes to highest level
			variant = variant & ~TWKB_BBOXES;
		}
	}
	switch ( geom->type )
	{
		case POINTTYPE:
		{
			LWDEBUGF(4,"Type found is Point, %d",geom->type);
			return lwpoint_to_twkb_buf((LWPOINT*)geom, buf, variant,factor,id,refpoint[0]);
		}
		/* LineString and CircularString both have 'points' elements */
		case CIRCSTRINGTYPE:
		case LINETYPE:
		{
			LWDEBUGF(4,"Type found is Linestring, %d",geom->type);
			return lwline_to_twkb_buf((LWLINE*)geom, buf, variant,factor,id,refpoint[0]);
		}
		/* Polygon has 'nrings' and 'rings' elements */
		case POLYGONTYPE:
		{
			LWDEBUGF(4,"Type found is Polygon, %d",geom->type);
			return lwpoly_to_twkb_buf((LWPOLY*)geom, buf, variant,factor,id,refpoint[0]);
		}
		/* Triangle has one ring of three points 
		case TRIANGLETYPE:
			return lwtriangle_to_twkb_buf((LWTRIANGLE*)geom, buf, variant);
*/
		/* All these Collection types have 'ngeoms' and 'geoms' elements */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		{
			LWDEBUGF(4,"Type found is Multi, %d",geom->type);
			/*the NO_TYPE flag tells that the type not shall be repeated for subgeometries*/
			return lwcollection_to_twkb_buf((LWCOLLECTION*)geom, buf, variant | TWKB_NO_TYPE,factor,id,refpoint);
		}			
		case COLLECTIONTYPE:
		{
			LWDEBUGF(4,"Type found is collection, %d",geom->type);
			return lwcollection_to_twkb_buf((LWCOLLECTION*)geom, buf, variant,factor,id,refpoint);
		}
		/* Unknown type! */
		default:
			lwerror("Unsupported geometry type: %s [%d]", lwtype_name(geom->type), geom->type);
	}
	/* Return value to keep compiler happy. */
	return 0;
}

/**
* Convert LWGEOM to a char* in TWKB format. Caller is responsible for freeing
* the returned array.
*/
uint8_t* lwgeom_to_twkb(const LWGEOM *geom, uint8_t variant, size_t *size_out,int8_t prec, int64_t id)
{
	size_t buf_size=0,size_size=0;
	uint8_t *buf = NULL;
	uint8_t *wkb_out = NULL;
	uint8_t flag=0;
	int64_t factor=pow(10,prec);
	/*an integer array 
	first dimmension holding the reference point, the last used point
	second and third is for max and min values to calculate bounding box.*/
	int64_t refpoint[3][4]={{0,0,0,0},{INT64_MAX,INT64_MAX,INT64_MAX,INT64_MAX},{INT64_MIN,INT64_MIN,INT64_MIN,INT64_MIN}};
	
	/* Initialize output size */
	if ( size_out ) *size_out = 0;

	if ( geom == NULL )
	{
		LWDEBUG(4,"Cannot convert NULL into WKB.");
		lwerror("Cannot convert NULL into WKB.");
		return NULL;
	}

	/* Calculate the required size of the output buffer */
	
	/*Adding the size for the first byte*/
	buf_size += 1;
	buf_size += lwgeom_to_twkb_size(geom,variant,factor,id,refpoint);
	LWDEBUGF(4, "WKB output size: %d", buf_size);

	
	//add the size of the bbox
	if(variant&TWKB_BBOXES)
	{		
		buf_size += sizeof_bbox(FLAGS_NDIMS(geom->flags),refpoint[1],refpoint[2]);
	}

		
	//reset refpoints
	refpoint[0][0]=refpoint[0][1]=refpoint[0][2]=refpoint[0][3]=0;
	
	//reset if NO_TYPE is used for subgeoemtries in size calculations
	variant = variant & ~TWKB_NO_TYPE;
	
	
	
	if ( buf_size == 0 )
	{
		LWDEBUG(4,"Error calculating output TWKB buffer size.");
		lwerror("Error calculating output TWKB buffer size.");
		return NULL;
	}
	/*If we want geometry sizes, we need space for that (the size we are going to store is the twkb_size - first byte and size value itself)*/
	if(variant & TWKB_SIZES)
		size_size=varint_u64_encoded_size((uint64_t) (buf_size-1));
	
	/* Allocate the buffer */
	buf = lwalloc(buf_size+size_size);
	if ( buf == NULL )
	{
		LWDEBUGF(4,"Unable to allocate %d bytes for TWKB output buffer.", buf_size);
		lwerror("Unable to allocate %d bytes for TWKB output buffer.", buf_size);
		return NULL;
	}

	/* Retain a pointer to the front of the buffer for later */
	wkb_out = buf;		


	
	/* set ID bit if ID*/
	FIRST_BYTE_SET_ID(flag, ((variant & TWKB_ID) ? 0xFF : 0));
	/*  set second bit if we are going to store resulting size*/	
	FIRST_BYTE_SET_SIZES(flag, ((variant & TWKB_SIZES) ? 0xFF : 0));
	/*  set third bit if we are going to store bboxes*/	
	FIRST_BYTE_SET_BBOXES(flag, ((variant & TWKB_BBOXES) ? 0xFF : 0));
	
	/* Tell what precision to use*/
	FIRST_BYTE_SET_PRECISION(flag,prec);
	
	/*Copy the flag to the buffer*/
	uint8_to_twkb_buf(flag,&buf);
	
	/*Write the size of the geometry*/
	if(variant & TWKB_SIZES)
		varint_u64_encode_buf((uint64_t)  (buf_size-1), &buf);
	
	/* Write the WKB into the output buffer */
	lwgeom_to_twkb_buf(geom, &buf,variant, factor,id,refpoint);

	LWDEBUGF(4,"buf (%p) - wkb_out (%p) = %d", buf, wkb_out, buf - wkb_out);

	/* The buffer pointer should now land at the end of the allocated buffer space. Let's check. */
	if ( (buf_size+size_size) != (buf - wkb_out) )
	{
		LWDEBUG(4,"Output TWKB is not the same size as the allocated buffer.");
		lwerror("Output TWKB is not the same size as the allocated buffer (precalculated size:%d, allocated size:%d)", buf_size, (buf - wkb_out));
		lwfree(wkb_out);
		return NULL;
	}

	/* Report output size */
	if ( size_out ) *size_out = (buf_size+size_size);

	return wkb_out;
}

uint8_t* lwgeom_agg_to_twkb(const twkb_geom_arrays *lwgeom_arrays,uint8_t variant , size_t *size_out,int8_t prec)
{
	size_t buf_size=0,size_size=0;
	uint8_t *buf = NULL;
	uint8_t flag = 0;
	uint8_t *wkb_out = NULL;
	int chk_homogenity=0;
	uint8_t type_flag = 0;
	int dims;
	int64_t factor=pow(10,prec);
	/*an integer array holding the reference point. In most cases the last used point
	but in the case of pointcloud it is a user defined refpoint.
	INT32_MIN indicates that the ref-point is not set yet*/
	int64_t refpoint[3][4]={{0,0,0,0},{INT64_MAX,INT64_MAX,INT64_MAX,INT64_MAX},{INT64_MIN,INT64_MIN,INT64_MIN,INT64_MIN}};
	
	LWDEBUGF(4, "We have collected: %d points, %d linestrings, %d polygons and %d collections",lwgeom_arrays->n_points,lwgeom_arrays->n_linestrings,lwgeom_arrays->n_polygons,lwgeom_arrays->n_collections );

	
	/* Initialize output size */
	if ( size_out ) *size_out = 0;
	
	if (lwgeom_arrays->n_points > 0)
		chk_homogenity++;
	if (lwgeom_arrays->n_linestrings > 0)
		chk_homogenity++;
	if (lwgeom_arrays->n_polygons > 0)
		chk_homogenity++	;
	if (lwgeom_arrays->n_collections > 0)
		chk_homogenity++;
	
	
	
	if(chk_homogenity==0)
		return NULL;
	if(chk_homogenity>1)
		buf_size = 2+varint_u64_encoded_size((uint64_t) chk_homogenity);
	else
		buf_size=1;
	
	
	if (lwgeom_arrays->n_points > 0)
		buf_size += lwgeom_agg_to_twkbpoint_size(lwgeom_arrays->points,variant,lwgeom_arrays->n_points, factor,refpoint);
	if (lwgeom_arrays->n_linestrings > 0)
		buf_size += lwgeom_agg_to_twkbline_size(lwgeom_arrays->linestrings,variant,lwgeom_arrays->n_linestrings, factor,refpoint);
	if (lwgeom_arrays->n_polygons > 0)
		buf_size += lwgeom_agg_to_twkbpoly_size(lwgeom_arrays->polygons,variant,lwgeom_arrays->n_polygons, factor,refpoint);
	if (lwgeom_arrays->n_collections > 0)
		buf_size += lwgeom_agg_to_twkbcollection_size(lwgeom_arrays->collections,variant,lwgeom_arrays->n_collections, factor,refpoint);
	
	
	//reset refpoints
	refpoint[0][0]=refpoint[0][1]=refpoint[0][2]=refpoint[0][3]=0;
	
	//reset if NO_TYPE is used for subgeoemtries in size calculations
	variant = variant & ~TWKB_NO_TYPE;
	
		LWDEBUGF(4, "WKB output size: %d", buf_size);

	if ( buf_size == 0 )
	{
		LWDEBUG(4,"Error calculating output TWKB buffer size.");
		lwerror("Error calculating output TWKB buffer size.");
		return NULL;
	}
	
	/*If we want geometry sizes, we need space for that (the size we are going to store is the twkb_size - first byte and size value itself)*/
	if(variant & TWKB_SIZES)
		size_size=varint_u64_encoded_size((uint64_t) (buf_size-1));
	
	/* Allocate the buffer */
	buf = lwalloc(buf_size+size_size);

	if ( buf == NULL )
	{
		LWDEBUGF(4,"Unable to allocate %d bytes for WKB output buffer.", buf_size);
		lwerror("Unable to allocate %d bytes for WKB output buffer.", buf_size);
		return NULL;
	}

	/* Retain a pointer to the front of the buffer for later */
	wkb_out = buf;	
		
	
	/* Set the id flag */
	FIRST_BYTE_SET_ID(flag, ((variant & TWKB_ID) ? 1 : 0));
	/*  set second bit if we are going to store resulting size*/	
	FIRST_BYTE_SET_SIZES(flag, ((variant & TWKB_SIZES) ? 0xFF : 0));
	/*  set third bit if we are going to store bboxes*/	
	FIRST_BYTE_SET_BBOXES(flag, ((variant & TWKB_BBOXES) ? 0xFF : 0));
	/* Tell what precision to use*/
	FIRST_BYTE_SET_PRECISION(flag,prec);
	
	/*Copy the flag to the buffer*/
	uint8_to_twkb_buf(flag,&buf);

	/*Write the size of the geometry*/
	if(variant & TWKB_SIZES)
		varint_u64_encode_buf((uint64_t)  (buf_size-1), &buf);
	
	/*set type and number of geometries for the top level, if more than 1 type og underlying geometries*/
	if(chk_homogenity>1)
	{

		/*Set the type*/
		TYPE_DIM_SET_TYPE(type_flag,7);	
		LWDEBUGF(4, "Writing type '%d'",7);
		
		/*We just set this to 4 dimmensions. It doesn't matter since all undelying geometries have their own*/
		dims = 4;
		TYPE_DIM_SET_DIM(type_flag,dims);	
		LWDEBUGF(4, "Writing ndims '%d'", dims);
		uint8_to_twkb_buf(type_flag,&buf);
		
		/* Set number of geometries */
		varint_u64_encode_buf(chk_homogenity, &buf);
	}
	
	/* Write the WKB into the output buffer 
	buf = lwgeom_to_twkb_buf(geom, buf,variant, prec,id,refpoint2);*/
	
	if (lwgeom_arrays->n_points > 0)
		lwgeom_agg_to_twkbpoint_buf(lwgeom_arrays->points,lwgeom_arrays->n_points, &buf,variant, factor,refpoint);
	if (lwgeom_arrays->n_linestrings > 0)
		lwgeom_agg_to_twkbline_buf(lwgeom_arrays->linestrings,lwgeom_arrays->n_linestrings, &buf,variant, factor,refpoint);
	if (lwgeom_arrays->n_polygons > 0)
		lwgeom_agg_to_twkbpoly_buf(lwgeom_arrays->polygons,lwgeom_arrays->n_polygons, &buf,variant, factor,refpoint);
	if (lwgeom_arrays->n_collections > 0)
		lwgeom_agg_to_twkbcollection_buf(lwgeom_arrays->collections,lwgeom_arrays->n_collections, &buf,variant, factor,refpoint);
	
	
	

	LWDEBUGF(4,"buf (%p) - wkb_out (%p) = %d", buf, wkb_out, buf - wkb_out);

	/* The buffer pointer should now land at the end of the allocated buffer space. Let's check. */
	if ( (buf_size+size_size) != (buf - wkb_out) )
	{
		LWDEBUG(4,"Output TWKB is not the same size as the allocated buffer.");
		lwerror("Output TWKB is not the same size as the allocated buffer (precalculated size:%d, allocated size:%d)", buf_size, (buf - wkb_out));
		lwfree(wkb_out);
		return NULL;
	}

	/* Report output size */
	if ( size_out ) *size_out =  (buf_size+size_size);

	return wkb_out;
	
}
