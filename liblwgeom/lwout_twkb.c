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

/*
* SwapBytes?
*/
static inline int wkb_swap_bytes(uint8_t variant)
{
	/* If requested variant matches machine arch, we don't have to swap! */
	if ( ((variant & WKB_NDR) && (getMachineEndian() == NDR)) ||
	     ((! (variant & WKB_NDR)) && (getMachineEndian() == XDR)) )
	{
		return LW_FALSE;
	}
	return LW_TRUE;
}

/*
* Integer32
*/
static uint8_t* int32_to_twkb_buf(const int ival, uint8_t *buf, uint8_t variant)
{
	char *iptr = (char*)(&ival);
	int i = 0;

	if ( sizeof(int) != WKB_INT_SIZE )
	{
		lwerror("Machine int size is not %d bytes!", WKB_INT_SIZE);
	}
	LWDEBUGF(4, "Writing value '%u'", ival);

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

/*
* Byte
*/
static uint8_t* uint8_to_twkb_buf(const uint8_t ival, uint8_t *buf)
{
		memcpy(buf, &ival, WKB_BYTE_SIZE);
		return buf + 1;
}

/*
* All sizes of integers
* This is for copying the different storaze sizes of the coordinates to the buffer.
*/
static uint8_t* to_twkb_buf(uint8_t *iptr, uint8_t *buf, uint8_t variant, int the_size)
{
	int i = 0;

	/* Machine/request arch mismatch, so flip byte order */
	if ( wkb_swap_bytes(variant)&&the_size>1 )
	{
		for ( i = 0; i < the_size; i++ )
		{
			buf[i] = iptr[the_size - 1 - i];
		}
	}
	/* If machine arch and requested arch match, don't flip byte order */
	else
	{
		memcpy(buf, iptr, the_size);
	}
	return buf + the_size;
}

/*
* Empty
*/
static size_t empty_to_twkb_size(const LWGEOM *geom, uint8_t variant)
{

	/* Endian flag/precision + id + type number + npoints*/
	size_t size = WKB_BYTE_SIZE + WKB_INT_SIZE + WKB_BYTE_SIZE + WKB_INT_SIZE;

	return size;
}

static uint8_t* empty_to_twkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id)
{
	uint32_t wkb_type = lwgeom_twkb_type(geom, variant);
	if ( geom->type == POINTTYPE )
	{
		/* Change POINT to MULTIPOINT */
		wkb_type &= ~WKB_POINT_TYPE;     /* clear POINT flag */
		wkb_type |= WKB_MULTIPOINT_TYPE; /* set MULTIPOINT flag */
	}
	uint8_t flag=0;
	
	/* Set the endian flag */
	END_PREC_SET__ENDIANESS(flag, ((variant & WKB_NDR) ? 1 : 0));
	/* Tell what precision to use*/
	END_PREC_SET__PRECISION(flag,prec);
	
	/*Copy the flag to the buffer*/
	buf = uint8_to_twkb_buf(flag,buf);
	
	/* Set the geometry id */
	buf = int32_to_twkb_buf(id, buf, variant);	
	
	/* Set the geometry type */	
	buf = uint8_to_twkb_buf(wkb_type,buf);	
	

	/* Set nrings/npoints/ngeoms to zero */
	buf = int32_to_twkb_buf(0, buf, variant);
	return buf;
}

static size_t ptarray_to_twkb_size(const POINTARRAY *pa, uint8_t variant,int prec,int accum_rel[],int method)
{
	switch (method)
	{
		case 0:
		return ptarray_to_twkb_size_m0(pa, variant,prec,accum_rel);
			break;
		/* Unknown method! */
		default:
			lwerror("Unsupported compression method: %d",method );
	}		
}




/*
* POINTARRAY
*/
static size_t ptarray_to_twkb_size_m0(const POINTARRAY *pa, uint8_t variant,int prec,int accum_rel[])
{
	LWDEBUGF(2, "ptarray_to_twkb_size entered%d",0);
	int dims = FLAGS_NDIMS(pa->flags);
	int i, j, r, last_size,factor,test_size,r_test,j_test;
	double *dbl_ptr;
	size_t size = 0;	
	last_size=0;
	int start=0;
	
	/*The variable factor is used to "shift" the double float coordinate to keep enough significant digits, 
	for demanded precision, when cast to integer*/
	factor=pow(10,prec);	
	
	/*This is a multidimmenstional array keeping trac of the three different storage sizes.
	It holds number of bytes, max-value and min-value*/
	static int size_array[3][3] = {{1,INT8_MAX,INT8_MIN},{2,INT16_MAX,INT16_MIN},{4,INT32_MAX,INT32_MIN}};
	
	/* Include the npoints size if it's not a POINT type) */
	if ( ! ( variant & WKB_NO_NPOINTS ) )
	{
		LWDEBUGF(2, "We add space for npoints",0);
		size += WKB_INT_SIZE;
	}
	/*if we don't have a ref-point yet*/
	if(accum_rel[1]==INT32_MIN)
	{
		LWDEBUGF(2, "We don't have a ref-point yet so we give space for full coordinates",0);
		/*Get a pointer to the first point of the point array*/
		dbl_ptr = (double*)getPoint_internal(pa, 0);
		
		/*Register the size of the first point
		it is 4 bytes per dimmension*/
		size += dims*WKB_INT_SIZE;
		
		LWDEBUGF(2, "Our geom have %d dims",dims);
		/*Load the accum_rel aray with the first points dimmension*/
		for ( j = 0; j < dims; j++ )
		{	
			LWDEBUGF(4, "dim nr %d",j);
			r = round(factor*dbl_ptr[j]);
			LWDEBUGF(4, "found value %d",r);
			accum_rel[j]=r;
			
			if(fabs(factor*dbl_ptr[j])>size_array[2][1])
				lwerror("The first coordinate exceeds the max_value (%d):%f",size_array[2][1],factor*dbl_ptr[j]);
			
		}	
		start=1;
	}

	LWDEBUGF(2, "We have %d points to iterate ",pa->npoints);
	for ( i = start; i < pa->npoints; i++ )
	{
		dbl_ptr = (double*)getPoint_internal(pa, i);
		for ( j = 0; j < dims; j++ )
		{
			/*To get the relative coordinate we don't get teh distance from the last point
			but instead the distance from our accumulated last point
			This is important to not build up a accumulated error when rounding the coordinates*/
			r=round(factor*dbl_ptr[j]-accum_rel[j]);
			
			
			/*last used size is too small so we have to increase*/
			if(fabs(r)>size_array[last_size][1])
			{
				/*A little ugly, but we sacrify the last possible value fitting into a INT4, just to detect too big values without the need to do the substraction again with a double float instead*/
				if(fabs(r)>=size_array[2][1])
					lwerror("The relative coordinate coordinate exceeds the max_value (%d):%d",size_array[2][1],r);
				/*minimum value for last used size, used to flag a change in size*/
				size +=  size_array[last_size][0];
				
				/*Find how much space we actually need */
				while ( fabs(r)>size_array[(++last_size)][1] && last_size<3) {}
				
				/*register the one byte needed to tell what size we need*/
				size ++;	
			}
			
			/*We don't need that much space so let's investigate if we should decrease*/
			else if(last_size>0 && fabs(r)<size_array[last_size-1][1])
			{
				/*We don't care to change to smaller size if we don't see all dimensions 
				in a coordinate fits in that size, so we don't even test if it isn't the first dimension*/
				if(j==0)
				{
					test_size=0;
					/*Here we test if we can decrease the size for all dimmensions*/
					for ( j_test = 0; j_test < dims;j_test++ )
					{
						r_test=round(factor*dbl_ptr[j_test]-accum_rel[j_test]);
						while ( fabs(r_test)>size_array[(test_size)][1] && test_size<=last_size) 
							{
								LWDEBUGF(4, "testing %d bytes for value %d, dim: %d",size_array[test_size][0],r_test,j_test );
								test_size++;
							}
					}
					if(test_size<last_size)
					{		
						/*
						OK, we decide to actually do the decrease in size
						minimum value for last used size, to flag a change in size*/
						size +=  size_array[last_size][0];
						
						/*We decrease to the biggest size needed for all dimmensions in the point*/
						last_size=test_size;						
						
						/*register the one byte needed to tell what size we need*/
						size++;
					}
				}
									
			}
			accum_rel[j]+=r;
			//add the size of the coordinate
			size +=  size_array[last_size][0];
		}
	}
	return size;
}

static uint8_t* ptarray_to_twkb_buf(const POINTARRAY *pa, uint8_t *buf, uint8_t variant,int8_t prec,int accum_rel[],int method)
{
	switch (method)
	{
		case 0:
			buf = ptarray_to_twkb_buf_m0(pa, buf, variant,prec,accum_rel);
		return buf;
		break;
		/* Unknown method! */
		default:
			lwerror("Unsupported compression method: %d",method );
	}		
}
static uint8_t* ptarray_to_twkb_buf_m0(const POINTARRAY *pa, uint8_t *buf, uint8_t variant,int8_t prec,int accum_rel[])
{
	int dims = FLAGS_NDIMS(pa->flags);
	int i, j, r, last_size,factor,test_size,r_test,j_test;
	double *dbl_ptr;
	factor=pow(10,prec);
	last_size=0;
	int start=0;
	/*This is a multidimmenstional array keeping trac of the three different storage sizes.
	It holds number of bytes, max-value and min-value*/
	static int size_array[3][3] = {{1,INT8_MAX,INT8_MIN},{2,INT16_MAX,INT16_MIN},{4,INT32_MAX,INT32_MIN}};
		
	/* Set the number of points (if it's not a POINT type) */
	if ( ! ( variant & WKB_NO_NPOINTS ) )
	{
		buf = int32_to_twkb_buf(pa->npoints, buf, variant);
		LWDEBUGF(4, "Regiter npoints:%d",pa->npoints);	
	}

	/*if we don't have a ref-point yet*/
	if(accum_rel[0]==INT32_MIN)
	{	
		/*Get a pointer do the first point of the point array*/
		dbl_ptr = (double*)getPoint_internal(pa, 0);
		
		/*the first coordinate for each dimension is copied to the buffer
		and registered in accum_rel array*/
			for ( j = 0; j < dims; j++ )
			{	
				r = round(factor*dbl_ptr[j]);
				accum_rel[j]=r;
				LWDEBUGF(4, "Writing dimension #%d (buf = %p)", j, buf);				
				buf = int32_to_twkb_buf(r, buf, variant);
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
				r=round(factor*dbl_ptr[j]-accum_rel[j]);
				//accum_rel[j]+=r;
				//LWDEBUGF(4, "delta value for dim %d is %d, real coordiinate is %f and accumulated coordinate is%d", j, r,dbl_ptr[j],accum_rel[j]  );
				LWDEBUGF(4, "size:%d,dim: %d deltavalue: %d, coordinate: %f accumulated coordinate %d",last_size, j, r,dbl_ptr[j],accum_rel[j]  );
				
				

				/*last used size is too small so we have to increase*/				
				if(fabs(r)>size_array[last_size][1])
				{
					
				LWDEBUGF(4, "increasing size from %d bytes",size_array[last_size][0]);
					/*minimum value for last used size, used to flag a change in size*/
					buf = to_twkb_buf((uint8_t *) &(size_array[last_size][2]),buf,  variant, size_array[last_size][0]);
					
					/*Find how much space we actually need */
					while ( fabs(r)>size_array[(++last_size)][1] && last_size<3) {}
					LWDEBUGF(4, "to size %d bytes",size_array[last_size][0]);
						
					/*register needed space*/
					memcpy( buf, &(size_array[last_size][0]), 1);
					buf++;
				}				
				/*We don't need that much space so let's investigate if we should decrease
				But if it is just a horizontal or vertical line, one dimmension will have short steps but another will still need bigger steps
				So, to avoid size changes up and down for every point we don't decrease size if it is not possible for all dimmensions
				We could here look even further forward to find out what is most optimal, but that will cost in computing instead*/
				else if (last_size>0 && fabs(r)<size_array[last_size-1][1])
				{
					/*We don't care to change to smaller size if we don't see all dimensions 
					in a coordinate fits in that size, so we don't even test if it isn't the first dimension*/
					if(j==0)
					{
						test_size=0;
						/*Here we test if we can decrease the size for all dimmensions*/
						for ( j_test = 0; j_test < dims;j_test++ )
						{
							r_test=round(factor*dbl_ptr[j_test]-accum_rel[j_test]);
							LWDEBUGF(4, "r_test = %d against size %d ; %d",r_test, test_size,size_array[(test_size)][1]  );	
							while ( fabs(r_test)>size_array[(test_size)][1] && test_size<=last_size) 
								{
									LWDEBUGF(4, "testing %d bytes for value %d, dim: %d",size_array[test_size][0],r_test,j_test );
									test_size++;
								}
						}
						if(test_size<last_size)
						{
							/*
							OK, we decide to actually do the decrease in size
							minimum value for last used size, to flag a change in size*/					
							buf = to_twkb_buf((uint8_t *) &(size_array[last_size][2]), buf,variant, size_array[last_size][0]);
														
							LWDEBUGF(4, "decreasing size from %d bytes",size_array[last_size][0]);	
							
							/*We decrease to the biggest size needed for all dimmensions in the point*/
							last_size=test_size;									
							LWDEBUGF(4, "to size %d bytes",size_array[last_size][0]);
							
							/*register how many bytes we need*/
							memcpy( buf, &(size_array[last_size][0]), 1);
							buf++;
						}
					}										
				}			
				accum_rel[j]+=r;
				//add the actual coordinate
				buf = to_twkb_buf((uint8_t *) &r,buf, variant, size_array[last_size][0]);
			}
		}	
	//LWDEBUGF(4, "Done (buf = %p)", buf);
	return buf;
}



static size_t  lwgeom_agg_to_twkbpoint_size(lwgeom_id *geom_array,uint8_t variant,int n,int8_t prec,int refpoint[],int method)
{
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of points*/
	size += WKB_INT_SIZE;

	int i;
	LWPOINT *p;
	for (i=0;i<n;i++)
	{
		p=(LWPOINT *) ((geom_array+i)->geom);
		size += lwpoint_to_twkb_size(p,variant,prec,refpoint,method);
	}
	return size;
}
/*
* POINT
*/
static size_t lwpoint_to_twkb_size(const LWPOINT *pt,uint8_t variant, int8_t prec,int refpoint[],int method)
{
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (!(variant & WKB_NO_ID))
	size	 += WKB_INT_SIZE;

	/* Points */
	size += ptarray_to_twkb_size(pt->point, variant | WKB_NO_NPOINTS, prec,refpoint,method);
	return size;
}


static uint8_t* lwgeom_agg_to_twkbpoint_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int refpoint[],int method)
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
	buf = int32_to_twkb_buf(n, buf, variant);

	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		buf = lwpoint_to_twkb_buf((LWPOINT*) (li->geom),buf,variant,prec,li->id,refpoint,method);
	}
	return buf;
}

static uint8_t* lwpoint_to_twkb_buf(const LWPOINT *pt, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method)
{

	
	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (!(variant & WKB_NO_ID))
		buf = int32_to_twkb_buf(id, buf, variant);	
	
		
	/* Set the coordinates */
	buf = ptarray_to_twkb_buf(pt->point, buf, variant | WKB_NO_NPOINTS,prec,refpoint,method);
	LWDEBUGF(4, "Pointarray set, buf = %p", buf);
	return buf;
}

static size_t  lwgeom_agg_to_twkbline_size(lwgeom_id* geom_array,uint8_t variant,int n,int8_t prec,int refpoint[],int method)
{
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of collections*/
	size += WKB_INT_SIZE;
	int i;
	for (i=0;i<n;i++)
	{
		size += lwline_to_twkb_size((LWLINE*)((geom_array+i)->geom),variant,prec,refpoint,method);
	}
	return size;
}
/*
* LINESTRING, CIRCULARSTRING
*/
static size_t lwline_to_twkb_size(const LWLINE *line,uint8_t variant, int8_t prec,int refpoint[],int method)
{	
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (!(variant & WKB_NO_ID))
	size	 += WKB_INT_SIZE;

	/* Size of point array */
	size += ptarray_to_twkb_size(line->points,variant,prec,refpoint,method);
	return size;
}

static uint8_t* lwgeom_agg_to_twkbline_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int refpoint[],int method)
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
	buf = int32_to_twkb_buf(n, buf, variant);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		buf = lwline_to_twkb_buf((LWLINE*) li->geom,buf,variant,prec,li->id,refpoint,method);
	}
	return buf;
}


static uint8_t* lwline_to_twkb_buf(const LWLINE *line, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method)
{

	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (!(variant & WKB_NO_ID))
		buf = int32_to_twkb_buf(id, buf, variant);			
	
	
	/* Set the coordinates */
	buf = ptarray_to_twkb_buf(line->points, buf, variant,prec,refpoint,method);
	return buf;
}


static size_t  lwgeom_agg_to_twkbpoly_size(lwgeom_id* geom_array,uint8_t variant,int n,int8_t prec,int refpoint[],int method)
{
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of collections*/
	size += WKB_INT_SIZE;
	int i;
	for (i=0;i<n;i++)
	{
		size += lwpoly_to_twkb_size((LWPOLY*)((geom_array+i)->geom),variant,prec,refpoint,method);
	}
	return size;
}
/*
* POLYGON
*/
static size_t lwpoly_to_twkb_size(const LWPOLY *poly,uint8_t variant, int8_t prec,int refpoint[],int method)
{
	LWDEBUGF(2, "lwpoly_to_twkb_size entered%d",0);
	int i;	
	
	size_t size = 0;
	/* geometry id, if not subgeometry in type 4,5 or 6*/
	if (!(variant & WKB_NO_ID))
		size	 += WKB_INT_SIZE;
	
	/*nrings*/
	size += WKB_INT_SIZE;
	
	LWDEBUGF(2, "we have %d rings to iterate",poly->nrings);
	for ( i = 0; i < poly->nrings; i++ )
	{
		/* Size of ring point array */
		size += ptarray_to_twkb_size(poly->rings[i],variant,prec,refpoint,method);
	}

	return size;
}

static uint8_t* lwgeom_agg_to_twkbpoly_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int refpoint[],int method)
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
	buf = int32_to_twkb_buf(n, buf, variant);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		buf = lwpoly_to_twkb_buf((LWPOLY*) (li->geom),buf,variant,prec,li->id,refpoint,method);
	}
	return buf;
}

static uint8_t* lwpoly_to_twkb_buf(const LWPOLY *poly, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method)
{
	int i;
	
	/* Set the geometry id, if not subgeometry in type 4,5 or 6*/
	if (!(variant & WKB_NO_ID))
		buf = int32_to_twkb_buf(id, buf, variant);		

	/* Set the number of rings */
	buf = int32_to_twkb_buf(poly->nrings, buf, variant);

	for ( i = 0; i < poly->nrings; i++ )
	{
		buf = ptarray_to_twkb_buf(poly->rings[i], buf, variant,prec,refpoint,method);
	}

	return buf;
}


static size_t  lwgeom_agg_to_twkbcollection_size(lwgeom_id* geom_array,uint8_t variant,int n,int8_t prec,int refpoint[],int method)
{
	LWDEBUGF(4, "lwgeom_agg_to_twkbcollection_size entered with %d collections",n);
	/*One byte for type declaration*/
	size_t size = WKB_BYTE_SIZE;
	/*One integer holding number of collections*/
	size += WKB_INT_SIZE;
	int i;
	for (i=0;i<n;i++)
	{
		size += lwcollection_to_twkb_size((LWCOLLECTION*)((geom_array+i)->geom),variant,prec,refpoint,method);
	}
	return size;
}
/*
* MULTIPOINT, MULTILINESTRING, MULTIPOLYGON, GEOMETRYCOLLECTION
* MULTICURVE, COMPOUNDCURVE, MULTISURFACE, CURVEPOLYGON, TIN, 
* POLYHEDRALSURFACE
*/
static size_t lwcollection_to_twkb_size(const LWCOLLECTION *col,uint8_t variant, int8_t prec,int refpoint[],int method)
{
	LWDEBUGF(2, "lwcollection_to_twkb_size entered, %d",0);
	/* id*/
	size_t size = WKB_INT_SIZE;
	/* size of geoms */
	size += WKB_INT_SIZE; 
	int i = 0;

	for ( i = 0; i < col->ngeoms; i++ )
	{
		/* size of subgeom */
		size += lwgeom_to_twkb_size((LWGEOM*)col->geoms[i],variant | WKB_NO_ID, prec,refpoint,method);
	}

	return size;
}

static uint8_t* lwgeom_agg_to_twkbcollection_buf(lwgeom_id* geom_array,int n, uint8_t *buf, uint8_t variant,int8_t prec, int refpoint[],int method)
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
	buf = int32_to_twkb_buf(n, buf, variant);
	
	for (i=0;i<n;i++)
	{
		li=(geom_array+i);
		buf = lwcollection_to_twkb_buf((LWCOLLECTION*) li->geom,buf,variant,prec,li->id,refpoint,method);
	}
	return buf;
}

static uint8_t* lwcollection_to_twkb_buf(const LWCOLLECTION *col, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method)
{
	int i;

	
	/* Set the geometry id */
	buf = int32_to_twkb_buf(id, buf, variant);	

	/* Set the number of rings */
	buf = int32_to_twkb_buf(col->ngeoms, buf, variant);

	/* Write the sub-geometries. Sub-geometries do not get SRIDs, they
	   inherit from their parents. */
	for ( i = 0; i < col->ngeoms; i++ )
	{
		buf = lwgeom_to_twkb_buf(col->geoms[i], buf, variant | WKB_NO_ID,prec,id,refpoint,method);
	}

	return buf;
}

/*
* GEOMETRY
*/
static size_t lwgeom_to_twkb_size(const LWGEOM *geom,uint8_t variant, int8_t prec,int refpoint[],int method)
{
	LWDEBUGF(2, "lwgeom_to_twkb_size entered %d",0);
	size_t size = 0;
	if ( geom == NULL )
		return 0;

	/* Short circuit out empty geometries */
	if ( lwgeom_is_empty(geom) )
	{
		return empty_to_twkb_size(geom, variant);
	}
	/*add size of type-declaration*/
	if (!(variant &  WKB_NO_TYPE))
		size += WKB_BYTE_SIZE;
	switch ( geom->type )
	{
		case POINTTYPE:
			size += lwpoint_to_twkb_size((LWPOINT*)geom, variant, prec,refpoint,method);
			break;

		/* LineString and CircularString both have points elements */
		case CIRCSTRINGTYPE:
		case LINETYPE:
			size += lwline_to_twkb_size((LWLINE*)geom, variant, prec,refpoint,method);
			break;

		/* Polygon has nrings and rings elements */
		case POLYGONTYPE:
			size += lwpoly_to_twkb_size((LWPOLY*)geom, variant, prec,refpoint,method);
			break;

		/* Triangle has one ring of three points 
		case TRIANGLETYPE:
			size += lwtriangle_to_twkb_size((LWTRIANGLE*)geom, variant);
			break;*/

		/* All these Collection types have ngeoms and geoms elements */
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
			size += lwcollection_to_twkb_size((LWCOLLECTION*)geom, variant | WKB_NO_TYPE, prec,refpoint,method);
			break;
		case COLLECTIONTYPE:
			size += lwcollection_to_twkb_size((LWCOLLECTION*)geom, variant, prec,refpoint,method);
			break;

		/* Unknown type! */
		default:
			lwerror("Unsupported geometry type: %s [%d]", lwtype_name(geom->type), geom->type);
	}

	return size;
}

/* TODO handle the TRIANGLE type properly */

static uint8_t* lwgeom_to_twkb_buf(const LWGEOM *geom, uint8_t *buf, uint8_t variant,int8_t prec, uint32_t id,int refpoint[],int method)
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
uint8_t* lwgeom_to_twkb(const LWGEOM *geom, uint8_t variant, size_t *size_out,int8_t prec, uint32_t id,int method)
{
	size_t buf_size;
	uint8_t *buf = NULL;
	uint8_t *wkb_out = NULL;
	uint8_t flag=0;
	/*an integer array holding the reference point. In most cases the last used point
	but in the case of pointcloud it is a user defined refpoint.
	INT32_MIN indicates that the ref-point is not set yet*/
	int refpoint[4]= {INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN};
	int refpoint2[4]= {INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN};
	
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
	buf_size += lwgeom_to_twkb_size(geom,variant,prec,refpoint,method);
	LWDEBUGF(4, "WKB output size: %d", buf_size);

	if ( buf_size == 0 )
	{
		LWDEBUG(4,"Error calculating output WKB buffer size.");
		lwerror("Error calculating output WKB buffer size.");
		return NULL;
	}

	/* If neither or both variants are specified, choose the native order */
	if ( ! (variant & WKB_NDR || variant & WKB_XDR) ||
	       (variant & WKB_NDR && variant & WKB_XDR) )
	{
		if ( getMachineEndian() == NDR ) 
			variant = variant | WKB_NDR;
		else
			variant = variant | WKB_XDR;
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
	END_PREC_SET__ENDIANESS(flag, ((variant & WKB_NDR) ? 1 : 0));
	/* Tell what method to use*/
	END_PREC_SET__METHOD(flag, method);
	/* Tell what precision to use*/
	END_PREC_SET__PRECISION(flag,prec);
	
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
	int refpoint[4]= {INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN};
	int refpoint2[4]= {INT32_MIN,INT32_MIN,INT32_MIN,INT32_MIN};
	
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
		buf_size=6;
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

	/* If neither or both variants are specified, choose the native order */
	if ( ! (variant & WKB_NDR || variant & WKB_XDR) ||
	       (variant & WKB_NDR && variant & WKB_XDR) )
	{
		if ( getMachineEndian() == NDR ) 
			variant = variant | WKB_NDR;
		else
			variant = variant | WKB_XDR;
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
	END_PREC_SET__ENDIANESS(flag, ((variant & WKB_NDR) ? 1 : 0));
	/* Tell what method to use*/
	END_PREC_SET__METHOD(flag, method);
	/* Tell what precision to use*/
	END_PREC_SET__PRECISION(flag,prec);
	
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
		buf = int32_to_twkb_buf(chk_homogenity, buf, variant);
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











