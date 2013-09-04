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
static uint8_t* uint8_to_twkb_buf(const uint8_t ival, uint8_t *buf)
{
	LWDEBUGF(2, "Entered  uint8_to_twkb_buf",0);
		memcpy(buf, &ival, WKB_BYTE_SIZE);
		return buf + 1;
}

/** 
Encodes a value as varInt described here:
https://developers.google.com/protocol-buffers/docs/encoding#varints
*/
int s_getvarint_size(int64_t val)
{
	
	LWDEBUGF(2, "Entered  s_getvarint_size",0);
	
	uint64_t q;
	int n=0;
	
	q = (val << 1) ^ (val >> 63);
	while ((q>>(7*(n+1))) >0)
	{
		n++;		
	}
	n++;
	return n;
}

/** 
Encodes a value as signed varInt
https://developers.google.com/protocol-buffers/docs/encoding#varints
*/
int u_getvarint_size(uint64_t val)
{
	LWDEBUGF(2, "Entered  u_getvarint_size",0);
	uint64_t q;
	int n=0;
		q =val;
	while ((q>>(7*(n+1))) >0)
	{
		n++;
	}
	n++;
	return n;
}

/**
Function for encoding a value as varInt and putting it in the buffer
*/
static uint8_t* u_varint_to_twkb_buf(uint64_t val, uint8_t *buf)
{
	LWDEBUGF(2, "Entered  u_varint_to_twkb_buf",0);
	uint64_t q;
	int n,grp;
	q =val;
	n=0;
	
	while ((q>>(7*(n+1))) >0)
	{
		grp=128^(127&(q>>(7*n)));
		buf[n]=grp;	
		n++;
	}
	grp=127&(q>>(7*n));
	buf[n]=grp;	
	n++;

	return buf+=n;
}

/**
Function for encoding a varInt value as signed
*/
static uint8_t* s_varint_to_twkb_buf(int64_t val, uint8_t *buf)
{
	LWDEBUGF(2, "Entered  s_varint_to_twkb_buf",0);
	uint64_t q;
	q = (val << 1) ^ (val >> 63);

	return u_varint_to_twkb_buf(q, buf);
}


/*
* Empty
*/
static size_t empty_to_twkb_size(const LWGEOM *geom, uint8_t variant, uint64_t id) 
{
	LWDEBUGF(2, "Entered  empty_to_twkb_size",0);

	/* Endian flag/precision + id + type number + npoints*/
	size_t size = WKB_BYTE_SIZE + WKB_BYTE_SIZE;
	/*size of ID*/
	size += u_getvarint_size((uint64_t) id);
	/*size of npoints*/
	size += u_getvarint_size((uint64_t) 0);

	return size;
}

static uint8_t* empty_to_twkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant,int8_t prec, uint64_t id)
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
	
	/* Set the id flag */
	END_PREC_SET_ID(flag, ((variant & WKB_ID) ? 1 : 0));
	/* Tell what precision to use*/
	END_PREC_SET_PRECISION(flag,prec);
	
	/*Copy the flag to the buffer*/
	buf = uint8_to_twkb_buf(flag,buf);
	
	/* Set the geometry id */	
	buf = u_varint_to_twkb_buf(id, buf);
	
	/* Set the geometry type */	
	buf = uint8_to_twkb_buf(wkb_type,buf);	
	

	/* Set nrings/npoints/ngeoms to zero */
	buf = u_varint_to_twkb_buf(0, buf);
	return buf;
}

/**
Chooses between encoding/compression methods for calculating the needed space
*/
static size_t ptarray_to_twkb_size(const POINTARRAY *pa, uint8_t variant,int prec,int64_t accum_rel[],int method)
{
	LWDEBUGF(2, "Entered ptarray_to_twkb_size",0);
	switch (method)
	{
		case 1:
		return ptarray_to_twkb_size_m1(pa, variant,prec,accum_rel);
			break;		
		/* Unknown method! */
		default:
			lwerror("Unsupported compression method: %d",method );
	}	
	/*Just to make the compiler quiet*/
		return 0;
}




/**
Calculates the needed space for storing a specific pointarray as varInt-encoded
*/
static size_t ptarray_to_twkb_size_m1(const POINTARRAY *pa, uint8_t variant,int prec,int64_t accum_rel[])
{
	LWDEBUGF(2, "Entered ptarray_to_twkb_size_m1",0);
	int64_t r;
	int dims = FLAGS_NDIMS(pa->flags);
	size_t size = 0;
	int j, factor,i;
	double *dbl_ptr;
	int start=0;
	/*The variable factor is used to "shift" the double float coordinate to keep enough significant digits, 
	for demanded precision, when cast to integer*/
	factor=pow(10,prec);
	/* Include the npoints size if it's not a POINT type) */
	if ( ! ( variant & WKB_NO_NPOINTS ) )
	{
		LWDEBUGF(4, "We add space for npoints",0);
		size+=u_getvarint_size(pa->npoints);
	}
	LWDEBUGF(4, "Refvalue dim 1 is %d",accum_rel[0]);
	if(accum_rel[0]==INT64_MIN)
	{
		LWDEBUGF(4, "We don't have a ref-point yet so we give space for full coordinates",0);
		/*Get a pointer to the first point of the point array*/
		dbl_ptr = (double*)getPoint_internal(pa, 0);
				
		LWDEBUGF(4, "Our geom have %d dims",dims);
		/*Load the accum_rel aray with the first points dimmension*/
		for ( j = 0; j < dims; j++ )
		{	
			
			LWDEBUGF(4, "dim nr %ld, refvalue is %ld",j, accum_rel[j]);
			r = (int64_t) round(factor*dbl_ptr[j]);
			accum_rel[j]=r;
			LWDEBUGF(4, "deltavalue = %ld and resulting refvalue is %ld",r,accum_rel[j]);
			size += s_getvarint_size((int64_t) r);
			
		}	
		start=1;
	}
	for ( i = start; i < pa->npoints; i++ )
	{
		dbl_ptr = (double*)getPoint_internal(pa, i);
		for ( j = 0; j < dims; j++ )
		{
			LWDEBUGF(4, "dim nr %d, refvalue is %d",j, accum_rel[j]);
			r=(int64_t) round(factor*dbl_ptr[j]-accum_rel[j]);
			accum_rel[j]+=r;			
			size += s_getvarint_size((int64_t) r);
			LWDEBUGF(4, "deltavalue = %d and resulting refvalue is %d",r,accum_rel[j]);
		}
	}
	return size;
}

/**
Chooses between encoding/compression methods for storing the pointarray
*/
static uint8_t* ptarray_to_twkb_buf(const POINTARRAY *pa, uint8_t *buf, uint8_t variant,int8_t prec,int64_t accum_rel[],int method)
{
	LWDEBUGF(2, "Entered ptarray_to_twkb_buf",0);
	switch (method)
	{
		case 1:
			buf = ptarray_to_twkb_buf_m1(pa, buf, variant,prec,accum_rel);
		return buf;
		break;
		/* Unknown method! */
		default:
			lwerror("Unsupported compression method: %d",method );
	}	
	/*Just to make the compiler quiet*/
	return 0;	
}

/**
Stores a pointarray as varInts in the buffer
*/
static uint8_t* ptarray_to_twkb_buf_m1(const POINTARRAY *pa, uint8_t *buf, uint8_t variant,int8_t prec,int64_t accum_rel[])
{
	LWDEBUGF(2, "entered ptarray_to_twkb_buf_m1\n",0);
	int64_t r;
	int dims = FLAGS_NDIMS(pa->flags);
	int i, j, factor;
	double *dbl_ptr;
	factor=pow(10,prec);
	int start=0;
	
	
		
	/* Set the number of points (if it's not a POINT type) */
	if ( ! ( variant & WKB_NO_NPOINTS ) )
	{
		buf = u_varint_to_twkb_buf(pa->npoints,buf);
		LWDEBUGF(4, "Register npoints:%d",pa->npoints);	
	}

	/*if we don't have a ref-point yet*/
	if(accum_rel[0]==INT64_MIN)
	{	
		LWDEBUGF(2, "We don't have a ref-point yet so we store the full coordinates",0);
		/*Get a pointer do the first point of the point array*/
		dbl_ptr = (double*)getPoint_internal(pa, 0);
		
		/*the first coordinate for each dimension is copied to the buffer
		and registered in accum_rel array*/
			for ( j = 0; j < dims; j++ )
			{	
				LWDEBUGF(4, "dim nr %ld, refvalue is %ld",j, accum_rel[j]);
				r = (int64_t) round(factor*dbl_ptr[j]);
				accum_rel[j]=r;
				LWDEBUGF(4, "deltavalue = %ld and resulting refvalue is %ld",r,accum_rel[j]);	
				buf = s_varint_to_twkb_buf(r,buf);
			}
		start=1;
	}
	for ( i = start; i < pa->npoints; i++ )
	{
		LWDEBUGF(4, "Writing point #%d", i);
		dbl_ptr = (double*)getPoint_internal(pa, i);
		for ( j = 0; j < dims; j++ )
		{
			/*To get the relative coordinate we don't get the distance from the last point
			but instead the distance from our accumulated last point
			This is important to not build up a accumulated error when rounding the coordinates*/				
			r=(int64_t) round(factor*dbl_ptr[j]-accum_rel[j]);		
LWDEBUGF(4, "deltavalue: %d, ",r );				
			accum_rel[j]+=r;
			//add the actual coordinate
			//s= getvarint((long) r, &ret, LW_TRUE);
			//memcpy(buf, &ret, s);	
			//buf+=s;
			buf = s_varint_to_twkb_buf(r,buf);
		}
	}	
	//LWDEBUGF(4, "Done (buf = %p)", buf);
	return buf;
}

/******************************************************************
POINTS
*******************************************************************/
/**
Calculates needed storage size for aggregated points
*/
static size_t  lwgeom_agg_to_twkbpoint_size(lwgeom_id *geom_array,uint8_t variant,int n,int8_t prec,int64_t refpoint[],int method)
{
	lwgeom_id *li;
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of geometries*/
	size += u_getvarint_size((uint64_t) 2);

	int i;
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		size += lwpoint_to_twkb_size((LWPOINT*) (li->geom),variant,prec,li->id,refpoint,method);
	}
	return size;
}
/**
Calculates needed storage size for a point
*/
static size_t lwpoint_to_twkb_size(const LWPOINT *pt,uint8_t variant, int8_t prec, uint64_t id,int64_t refpoint[],int method)
{
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & WKB_ID)
	size	 += u_getvarint_size((uint64_t) id);

	/* Points */
	size += ptarray_to_twkb_size(pt->point, variant | WKB_NO_NPOINTS, prec,refpoint,method);
	return size;
}

/**
Iterates an aggregation of points
*/
static uint8_t* lwgeom_agg_to_twkbpoint_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int64_t refpoint[],int method)
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
	buf = uint8_to_twkb_buf(type_flag,buf);
	
	/* Set number of geometries */
	buf = u_varint_to_twkb_buf(n, buf);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		buf = lwpoint_to_twkb_buf((LWPOINT*) (li->geom),buf,variant,prec,li->id,refpoint,method);

	}
	return buf;
}

/**
Sends a point to the buffer
*/
static uint8_t* lwpoint_to_twkb_buf(const LWPOINT *pt, uint8_t *buf, uint8_t variant,int8_t prec, uint64_t id,int64_t refpoint[],int method)
{

	
	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & WKB_ID)
		buf = u_varint_to_twkb_buf(id, buf);	
	
		
	/* Set the coordinates */
	buf = ptarray_to_twkb_buf(pt->point, buf, variant | WKB_NO_NPOINTS,prec,refpoint,method);
	LWDEBUGF(4, "Pointarray set, buf = %p", buf);
	return buf;
}

/******************************************************************
LINESTRINGS
*******************************************************************/
/**
Calculates needed storage size for aggregated lines
*/
static size_t  lwgeom_agg_to_twkbline_size(lwgeom_id* geom_array,uint8_t variant,int n,int8_t prec,int64_t refpoint[],int method)
{
	lwgeom_id *li;
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of lines*/
	size += u_getvarint_size((uint64_t) n);
	int i;
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		size += lwline_to_twkb_size((LWLINE*) (li->geom),variant,prec,li->id,refpoint,method);
	}
	return size;
}
/**
Calculates needed storage size for a line
*/
static size_t lwline_to_twkb_size(const LWLINE *line,uint8_t variant, int8_t prec, uint64_t id,int64_t refpoint[],int method)
{	
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & WKB_ID)
	size	 += u_getvarint_size((uint64_t) id);

	/* Size of point array */
	size += ptarray_to_twkb_size(line->points,variant,prec,refpoint,method);
	return size;
}
/**
Iterates an aggregation of lines
*/
static uint8_t* lwgeom_agg_to_twkbline_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int64_t refpoint[],int method)
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
	buf = uint8_to_twkb_buf(type_flag,buf);
	
	/* Set number of geometries */
	buf = u_varint_to_twkb_buf(n, buf);
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		buf = lwline_to_twkb_buf((LWLINE*) li->geom,buf,variant,prec,li->id,refpoint,method);
	}
	return buf;
}

/**
Sends a line to the buffer
*/
static uint8_t* lwline_to_twkb_buf(const LWLINE *line, uint8_t *buf, uint8_t variant,int8_t prec, uint64_t id,int64_t refpoint[],int method)
{

	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & WKB_ID)
		buf = u_varint_to_twkb_buf(id, buf);				
	
	
	/* Set the coordinates */
	buf = ptarray_to_twkb_buf(line->points, buf, variant,prec,refpoint,method);
	return buf;
}

/******************************************************************
POLYGONS
*******************************************************************/
/**
Calculates needed storage size for aggregated polygon
*/
static size_t  lwgeom_agg_to_twkbpoly_size(lwgeom_id* geom_array,uint8_t variant,int n,int8_t prec,int64_t refpoint[],int method)
{
	lwgeom_id *li;
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of collections*/
	size +=u_getvarint_size((uint64_t) n);
	int i;
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		size += lwpoly_to_twkb_size((LWPOLY*) (li->geom),variant,prec,li->id,refpoint,method);
	}
	return size;
}

/**
Calculates needed storage size for a polygon
*/
static size_t lwpoly_to_twkb_size(const LWPOLY *poly,uint8_t variant, int8_t prec, uint64_t id,int64_t refpoint[],int method)
{
	LWDEBUGF(2, "lwpoly_to_twkb_size entered%d",0);
	int i;	
	
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & WKB_ID)
	size	 += u_getvarint_size((uint64_t) id);
	
	/*nrings*/
	size += u_getvarint_size((uint64_t) poly->nrings);
	
	LWDEBUGF(2, "we have %d rings to iterate",poly->nrings);
	for ( i = 0; i < poly->nrings; i++ )
	{
		/* Size of ring point array */
		size += ptarray_to_twkb_size(poly->rings[i],variant,prec,refpoint,method);
	}

	return size;
}

/**
Iterates an aggregation of polygons
*/
static uint8_t* lwgeom_agg_to_twkbpoly_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int64_t refpoint[],int method)
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
	buf = uint8_to_twkb_buf(type_flag,buf);
	/* Set number of geometries */
	buf = u_varint_to_twkb_buf(n, buf);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		buf = lwpoly_to_twkb_buf((LWPOLY*) (li->geom),buf,variant,prec,li->id,refpoint,method);
	}
	return buf;
}

/**
Sends a polygon to the buffer
*/
static uint8_t* lwpoly_to_twkb_buf(const LWPOLY *poly, uint8_t *buf, uint8_t variant,int8_t prec, uint64_t id,int64_t refpoint[],int method)
{
	int i;
	
	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (variant & WKB_ID)
		buf = u_varint_to_twkb_buf(id, buf);	

	/* Set the number of rings */
	buf = u_varint_to_twkb_buf(poly->nrings, buf);
	
	for ( i = 0; i < poly->nrings; i++ )
	{
		buf = ptarray_to_twkb_buf(poly->rings[i], buf, variant,prec,refpoint,method);
	}

	return buf;
}

/******************************************************************
COLLECTIONS
*******************************************************************/
/**
Calculates needed storage size for aggregated collection
*/
static size_t  lwgeom_agg_to_twkbcollection_size(lwgeom_id* geom_array,uint8_t variant,int n,int8_t prec,int64_t refpoint[],int method)
{
	lwgeom_id *li;
	LWDEBUGF(4, "lwgeom_agg_to_twkbcollection_size entered with %d collections",n);
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of collections*/
	size += u_getvarint_size((uint64_t) n);
	int i;
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		size += lwcollection_to_twkb_size((LWCOLLECTION*) (li->geom),variant,prec,li->id,refpoint,method);
	}
	return size;
}

/**
Calculates needed storage size for a collection
*/
static size_t lwcollection_to_twkb_size(const LWCOLLECTION *col,uint8_t variant, int8_t prec, uint64_t id,int64_t refpoint[],int method)
{
	LWDEBUGF(2, "lwcollection_to_twkb_size entered, %d",0);
	/* id*/
	size_t size	 = u_getvarint_size((uint64_t) id);
	/* size of geoms */
	size += u_getvarint_size((uint64_t) col->ngeoms); 
	int i = 0;

	for ( i = 0; i < col->ngeoms; i++ )
	{
		/* size of subgeom */
		size += lwgeom_to_twkb_size((LWGEOM*)col->geoms[i],variant | WKB_NO_ID, prec,id,refpoint,method);
	}

	return size;
}

/**
Iterates an aggregation of collections
*/
static uint8_t* lwgeom_agg_to_twkbcollection_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int64_t refpoint[],int method)
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
	buf = uint8_to_twkb_buf(type_flag,buf);
	/* Set number of geometries */
	buf = u_varint_to_twkb_buf(n, buf);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		buf = lwcollection_to_twkb_buf((LWCOLLECTION*) li->geom,buf,variant,prec,li->id,refpoint,method);
	}
	return buf;
}

/**
Iterates a collection
*/
static uint8_t* lwcollection_to_twkb_buf(const LWCOLLECTION *col, uint8_t *buf, uint8_t variant,int8_t prec, uint64_t id,int64_t refpoint[],int method)
{
	int i;

	
	/* Set the geometry id */
	buf = u_varint_to_twkb_buf(id, buf);	

	/* Set the number of rings */
	buf = u_varint_to_twkb_buf(col->ngeoms, buf);
	
	/* Write the sub-geometries. Sub-geometries do not get SRIDs, they
	   inherit from their parents. */
	for ( i = 0; i < col->ngeoms; i++ )
	{
		buf = lwgeom_to_twkb_buf(col->geoms[i], buf, variant | WKB_NO_ID,prec,id,refpoint,method);
	}

	return buf;
}

/**
Calculates the needed space for a geometry as twkb
*/
static size_t lwgeom_to_twkb_size(const LWGEOM *geom,uint8_t variant, int8_t prec, uint64_t id,int64_t refpoint[],int method)
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
	if (!(variant &  WKB_NO_TYPE))
		size += WKB_BYTE_SIZE;
	switch ( geom->type )
	{
		case POINTTYPE:
			size += lwpoint_to_twkb_size((LWPOINT*)geom, variant, prec,id,refpoint,method);
			break;

		/* LineString and CircularString both have points elements */
		case CIRCSTRINGTYPE:
		case LINETYPE:
			size += lwline_to_twkb_size((LWLINE*)geom, variant, prec,id,refpoint,method);
			break;

		/* Polygon has nrings and rings elements */
		case POLYGONTYPE:
			size += lwpoly_to_twkb_size((LWPOLY*)geom, variant, prec,id,refpoint,method);
			break;

		/* Triangle has one ring of three points 
		case TRIANGLETYPE:
			size += lwtriangle_to_twkb_size((LWTRIANGLE*)geom, variant);
			break;*/

		/* All these Collection types have ngeoms and geoms elements */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
			size += lwcollection_to_twkb_size((LWCOLLECTION*)geom, variant | WKB_NO_TYPE, prec,id,refpoint,method);
			break;
		case COLLECTIONTYPE:
			size += lwcollection_to_twkb_size((LWCOLLECTION*)geom, variant, prec,id,refpoint,method);
			break;

		/* Unknown type! */
		default:
			lwerror("Unsupported geometry type: %s [%d]", lwtype_name(geom->type), geom->type);
	}

	return size;
}


static uint8_t* lwgeom_to_twkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant,int8_t prec, uint64_t id,int64_t refpoint[],int method)
{

	if ( lwgeom_is_empty(geom) )
		return empty_to_twkb_buf(geom, buf, variant,prec,id);

	switch ( geom->type )
	{
		case POINTTYPE:
		{
			if (!(variant &  WKB_NO_TYPE))
				buf = uint8_to_twkb_buf(lwgeom_twkb_type(geom, variant),buf);
			return lwpoint_to_twkb_buf((LWPOINT*)geom, buf, variant,prec,id,refpoint,method);
		}
		/* LineString and CircularString both have 'points' elements */
		case CIRCSTRINGTYPE:
		case LINETYPE:
		{
			if (!(variant &  WKB_NO_TYPE))
				buf = uint8_to_twkb_buf(lwgeom_twkb_type(geom, variant),buf);
			return lwline_to_twkb_buf((LWLINE*)geom, buf, variant,prec,id,refpoint,method);
		}
		/* Polygon has 'nrings' and 'rings' elements */
		case POLYGONTYPE:
		{
			if (!(variant &  WKB_NO_TYPE))
				buf = uint8_to_twkb_buf(lwgeom_twkb_type(geom, variant),buf);
			return lwpoly_to_twkb_buf((LWPOLY*)geom, buf, variant,prec,id,refpoint,method);
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
			buf = uint8_to_twkb_buf(lwgeom_twkb_type(geom, variant),buf);
			return lwcollection_to_twkb_buf((LWCOLLECTION*)geom, buf, variant | WKB_NO_TYPE,prec,id,refpoint,method);
		}			
		case COLLECTIONTYPE:
		{
			buf = uint8_to_twkb_buf(lwgeom_twkb_type(geom, variant),buf);
			return lwcollection_to_twkb_buf((LWCOLLECTION*)geom, buf, variant,prec,id,refpoint,method);
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
uint8_t* lwgeom_to_twkb(const LWGEOM *geom, uint8_t variant, size_t *size_out,int8_t prec, uint64_t id,int method)
{
	size_t buf_size;
	uint8_t *buf = NULL;
	uint8_t *wkb_out = NULL;
	uint8_t flag=0;
	/*an integer array holding the reference point. In most cases the last used point
	but in the case of pointcloud it is a user defined refpoint.
	INT32_MIN indicates that the ref-point is not set yet*/
	int64_t refpoint[4]= {INT64_MIN,INT64_MIN,INT64_MIN,INT64_MIN};
	int64_t refpoint2[4]= {INT64_MIN,INT64_MIN,INT64_MIN,INT64_MIN};
	
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
	buf_size = 1;
	buf_size += lwgeom_to_twkb_size(geom,variant,prec,id,refpoint,method);
	LWDEBUGF(4, "WKB output size: %d", buf_size);

	if ( buf_size == 0 )
	{
		LWDEBUG(4,"Error calculating output WKB buffer size.");
		lwerror("Error calculating output WKB buffer size.");
		return NULL;
	}

	/* Allocate the buffer */
	buf = lwalloc(buf_size);
	if ( buf == NULL )
	{
		LWDEBUGF(4,"Unable to allocate %d bytes for WKB output buffer.", buf_size);
		lwerror("Unable to allocate %d bytes for WKB output buffer.", buf_size);
		return NULL;
	}

	/* Retain a pointer to the front of the buffer for later */
	wkb_out = buf;		


	
	/* set ID bit if ID*/
	END_PREC_SET_ID(flag, ((variant & WKB_ID) ? 1 : 0));
	/* Tell what method to use*/
	END_PREC_SET_METHOD(flag, method);
	/* Tell what precision to use*/
	END_PREC_SET_PRECISION(flag,prec);
	
	/*Copy the flag to the buffer*/
	buf = uint8_to_twkb_buf(flag,buf);
	

	
	/* Write the WKB into the output buffer */
	buf = lwgeom_to_twkb_buf(geom, buf,variant, prec,id,refpoint2,method);

	LWDEBUGF(4,"buf (%p) - wkb_out (%p) = %d", buf, wkb_out, buf - wkb_out);

	/* The buffer pointer should now land at the end of the allocated buffer space. Let's check. */
	if ( buf_size != (buf - wkb_out) )
	{
		LWDEBUG(4,"Output WKB is not the same size as the allocated buffer.");
		lwerror("Output WKB is not the same size as the allocated buffer.");
		lwfree(wkb_out);
		return NULL;
	}

	/* Report output size */
	if ( size_out ) *size_out = buf_size;

	return wkb_out;
}

uint8_t* lwgeom_agg_to_twkb(const twkb_geom_arrays *lwgeom_arrays,uint8_t variant , size_t *size_out,int8_t prec,int method)
{
	size_t buf_size;
	uint8_t *buf = NULL;
	uint8_t flag = 0;
	uint8_t *wkb_out = NULL;
	int chk_homogenity=0;
	uint8_t type_flag = 0;
	int dims;
	/*an integer array holding the reference point. In most cases the last used point
	but in the case of pointcloud it is a user defined refpoint.
	INT32_MIN indicates that the ref-point is not set yet*/
	int64_t refpoint[4]= {INT64_MIN,INT64_MIN,INT64_MIN,INT64_MIN};
	int64_t refpoint2[4]= {INT64_MIN,INT64_MIN,INT64_MIN,INT64_MIN};
	
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
		buf_size = 2+u_getvarint_size((uint64_t) chk_homogenity);
	else
		buf_size=1;
	
	
	if (lwgeom_arrays->n_points > 0)
		buf_size += lwgeom_agg_to_twkbpoint_size(lwgeom_arrays->points,variant,lwgeom_arrays->n_points, prec,refpoint,method);
	if (lwgeom_arrays->n_linestrings > 0)
		buf_size += lwgeom_agg_to_twkbline_size(lwgeom_arrays->linestrings,variant,lwgeom_arrays->n_linestrings, prec,refpoint,method);
	if (lwgeom_arrays->n_polygons > 0)
		buf_size += lwgeom_agg_to_twkbpoly_size(lwgeom_arrays->polygons,variant,lwgeom_arrays->n_polygons, prec,refpoint,method);
	if (lwgeom_arrays->n_collections > 0)
		buf_size += lwgeom_agg_to_twkbcollection_size(lwgeom_arrays->collections,variant,lwgeom_arrays->n_collections, prec,refpoint,method);
	
	
	
	
	
		LWDEBUGF(4, "WKB output size: %d", buf_size);

	if ( buf_size == 0 )
	{
		LWDEBUG(4,"Error calculating output WKB buffer size.");
		lwerror("Error calculating output WKB buffer size.");
		return NULL;
	}

	/* Allocate the buffer */
	buf = lwalloc(buf_size);

	if ( buf == NULL )
	{
		LWDEBUGF(4,"Unable to allocate %d bytes for WKB output buffer.", buf_size);
		lwerror("Unable to allocate %d bytes for WKB output buffer.", buf_size);
		return NULL;
	}

	/* Retain a pointer to the front of the buffer for later */
	wkb_out = buf;	
		
	
	/* Set the endian flag */
	END_PREC_SET_ID(flag, ((variant & WKB_ID) ? 1 : 0));
	/* Tell what method to use*/
	END_PREC_SET_METHOD(flag, method);
	/* Tell what precision to use*/
	END_PREC_SET_PRECISION(flag,prec);
	
	/*Copy the flag to the buffer*/
	buf = uint8_to_twkb_buf(flag,buf);

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
		buf = uint8_to_twkb_buf(type_flag,buf);
		
		/* Set number of geometries */
		buf = u_varint_to_twkb_buf(chk_homogenity, buf);
	}
	
	/* Write the WKB into the output buffer 
	buf = lwgeom_to_twkb_buf(geom, buf,variant, prec,id,refpoint2);*/
	
	if (lwgeom_arrays->n_points > 0)
		buf =lwgeom_agg_to_twkbpoint_buf(lwgeom_arrays->points,lwgeom_arrays->n_points, buf,variant, prec,refpoint2,method);
	if (lwgeom_arrays->n_linestrings > 0)
		buf =lwgeom_agg_to_twkbline_buf(lwgeom_arrays->linestrings,lwgeom_arrays->n_linestrings, buf,variant, prec,refpoint2,method);
	if (lwgeom_arrays->n_polygons > 0)
		buf =lwgeom_agg_to_twkbpoly_buf(lwgeom_arrays->polygons,lwgeom_arrays->n_polygons, buf,variant, prec,refpoint2,method);
	if (lwgeom_arrays->n_collections > 0)
		buf =lwgeom_agg_to_twkbcollection_buf(lwgeom_arrays->collections,lwgeom_arrays->n_collections, buf,variant, prec,refpoint2,method);
	
	
	

	LWDEBUGF(4,"buf (%p) - wkb_out (%p) = %d", buf, wkb_out, buf - wkb_out);

	/* The buffer pointer should now land at the end of the allocated buffer space. Let's check. */
	if ( buf_size != (buf - wkb_out) )
	{
		LWDEBUG(4,"Output WKB is not the same size as the allocated buffer.");
		lwerror("Output WKB is not the same size as the allocated buffer.");
		lwfree(wkb_out);
		return NULL;
	}

	/* Report output size */
	if ( size_out ) *size_out = buf_size;

	return wkb_out;
	
}
