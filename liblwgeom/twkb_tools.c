/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 *
 * Copyright (C) 2015 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <math.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "varint.h"
#include "lwout_twkb.h"

/**********************************************************************/

/**
* A function to collect many twkb-buffers into one collection
*/
uint8_t* twkb_to_twkbcoll(uint8_t **twkb, size_t *sizes,size_t *out_size, int64_t *idlist, int n)
{
	LWDEBUG(2,"Entering twkb_to_twkbcoll");
	TWKB_STATE ts;
	int i,b;
	uint8_t *buf;
	uint8_t metabyte;
	int has_bbox   =  0;
	int has_size   =  0;
	int extended_dims = 0;
	int is_empty   = 0;
	size_t size;	
	int cursor;
	int64_t max, min;
	int n_dims=0;	
	uint8_t type_prec = 0;
	uint8_t flag = 0;
	size_t size_to_register=0;
	uint8_t *twkb_out;
	size_t buf_size;
	int has_z, has_m;
	
	for ( i = 0; i < MAX_N_DIMS; i++ )
	{
		/* Reset bbox calculation */
		ts.bbox_max[i] = INT64_MIN;
		ts.bbox_min[i] = INT64_MAX;
	}	
	
	/*For each buffer, collect their bounding box*/
	for (i=0;i<n;i++)
	{		
		LWDEBUGF(2,"Read header of buffer %d",i);
		cursor = 0;		
		buf = twkb[i];
		buf_size=sizes[i];
		size_to_register+=buf_size;
		
		/*jump over type byte*/
		cursor++;		
		metabyte = *(buf+cursor);
		cursor++;

		has_bbox   =  metabyte & 0x01;
		has_size   = (metabyte & 0x02) >> 1;
		extended_dims = (metabyte & 0x08) >> 3;
		is_empty   = (metabyte & 0x10) >> 4;
		
		if ( extended_dims )
		{
			extended_dims = *(buf+cursor);
			cursor++;

			has_z    = (extended_dims & 0x01);
			has_m    = (extended_dims & 0x02) >> 1;
		}
		else
		{
			has_z = 0;
			has_m = 0;
		}
		
		n_dims=2+has_z+has_m;	
		if(has_size)
		{
			/*jump over size_value*/
			cursor+=varint_size(buf+cursor, buf+buf_size);
		}

		if(!has_bbox)
			lwerror("We can only collect twkb with included bbox");
		
		/*read bbox and expand to all*/
		for(b=0;b<n_dims;b++)
		{
			min = varint_s64_decode(buf+cursor, buf+buf_size, &size);
			cursor+=size;
			if(min<ts.bbox_min[b])
				ts.bbox_min[b]=min;
			max = min + varint_s64_decode(buf+cursor, buf+buf_size, &size);
			cursor+=size;					
			if(max>ts.bbox_max[b])
				ts.bbox_max[b]=max;			
		}			
	}
	
	/*Ok, we have all bounding boxes, then it's time to write*/
	ts.header_buf = bytebuffer_create_with_size(16);	
	ts.geom_buf = bytebuffer_create_with_size(32);	
	
	/* The type will always be COLLECTIONTYPE */
	TYPE_PREC_SET_TYPE(type_prec,COLLECTIONTYPE);
	/* Precision has no meaning here. All subgeoemtries holds their own precision */
	TYPE_PREC_SET_PREC(type_prec,0);
	/* Write the type and precision byte */
	bytebuffer_append_byte(ts.header_buf, type_prec);

	/* METADATA BYTE */
	/* Always bboxes */
	FIRST_BYTE_SET_BBOXES(flag, 1);
	/* Always sizes */
	FIRST_BYTE_SET_SIZES(flag,1);
	/* Is there idlist? */
	FIRST_BYTE_SET_IDLIST(flag, idlist && ! is_empty);
	/* No extended byte */
	FIRST_BYTE_SET_EXTENDED(flag, 0);
	/* Write the header byte */
	bytebuffer_append_byte(ts.header_buf, flag);	
	
	
	/*Write number of geometries*/
	bytebuffer_append_uvarint(ts.geom_buf, n);
	
	/* We've been handed an idlist, so write it in */
	if ( idlist )
	{
		for ( i = 0; i < n; i++ )
		{
			bytebuffer_append_varint(ts.geom_buf, idlist[i]);
		}
	}
	
	/*Write the size of our new collection from size-info to the end*/
	size_to_register += sizeof_bbox(&ts,n_dims) + bytebuffer_getlength(ts.geom_buf);	
	bytebuffer_append_uvarint(ts.header_buf, size_to_register);	
	
	/*Write the bbox to the buffer*/
	write_bbox(&ts, n_dims);	
	
	/*Put the id-list after the header*/
	bytebuffer_append_bytebuffer(ts.header_buf,ts.geom_buf);	
	
	/*Copy all ingoing buffers "as is" to the new buffer*/
	for ( i = 0; i < n; i++ )
	{
		bytebuffer_append_bulk(ts.header_buf, twkb[i], sizes[i]);		
	}
	
	
	bytebuffer_destroy(ts.geom_buf);
	
	if ( out_size )
		*out_size = bytebuffer_getlength(ts.header_buf);	
	
	twkb_out = ts.header_buf->buf_start;

	lwfree(ts.header_buf);
	
	return twkb_out;
}
