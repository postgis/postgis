#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"

#include "fmgr.h"
#include "utils/elog.h"


#include "lwgeom.h"

extern char *wkb_to_lwgeom(char *wkb, int SRID,int *size);
extern char *lwgeom_to_wkb(char *serialized_form,int *wkblength,char desiredWKBEndian);
extern void swap_char(char	*a,char *b);
extern void	flip_endian_double(char 	*d);
extern void		flip_endian_int32(char		*i);
extern char wkb_dims(uint32 type);
extern char wkb_simpletype (uint32 type);
extern uint32 constructWKBType(int simple_type, char dims);
extern bool requiresflip(char WKBendianflag);
extern void flipPoints(char *pts, int npoints, char dims);
extern uint32 constructWKBType(int simple_type, char dims);

extern LWPOINT *wkb_point_to_lwpoint(char *wkb,int SRID);
extern LWLINE *wkb_line_to_lwline(char *wkb,int SRID);
extern LWPOLY *wkb_poly_to_lwpoly(char *wkb,int SRID);

extern char *lwline_to_wkb(LWLINE *line, char desiredWKBEndian, int *wkbsize);
extern char *lwpoint_to_wkb(LWPOINT *point, char desiredWKBEndian, int *wkbsize);
extern char *lwpoly_to_wkb(LWPOLY *poly, char desiredWKBEndian, int *wkbsize);

// 3d or 4d.  There is NOT a (x,y,m) point type!!!
#define WKB3DOFFSET 0x80000000
#define WKB4DOFFSET 0x40000000

Datum LWGEOMFromWKB(PG_FUNCTION_ARGS);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS);

// LWGEOMFromWKB(wkb,  [SRID] )
PG_FUNCTION_INFO_V1(LWGEOMFromWKB);
Datum LWGEOMFromWKB(PG_FUNCTION_ARGS)
{
		char   *wkb_input = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int    SRID ;
		int    size;
		char *lwgeom;
		char * result;

				if ( ! PG_ARGISNULL(1) )
					SRID = PG_GETARG_INT32(1);
				else
					SRID = -1;

		lwgeom = wkb_to_lwgeom(wkb_input, SRID,&size);
		result = palloc(size+4);

		memcpy(result+4, lwgeom, size);
		size+=4;
		memcpy(result, &size, 4);

		pfree(lwgeom);

		PG_RETURN_POINTER(result);
}

// WKBFromLWGEOM(wkb)
PG_FUNCTION_INFO_V1(WKBFromLWGEOM);
Datum WKBFromLWGEOM(PG_FUNCTION_ARGS)
{
		char   *lwgeom_input = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int    size;
		char *wkb = lwgeom_to_wkb(lwgeom_input+4,&size,1); // 0=XDR, 1=NDR

		char *result = palloc(size+4);

		memcpy(result+4, wkb, size);
		size+=4;
		memcpy(result, &size, 4);
		pfree(wkb);

		PG_RETURN_POINTER(result);
}


//given a wkb (with 3d and 4d extension)
// create the serialized form of an equivelent LWGEOM
// size of the lwgeom form is also returned
//
// note - to actually serialize, you should put the first 4
// bytes as size (size +4), then the results of this function.
//
// because this function modifies the wkb (to make it the
// server's endian), you should give it a copy of the wkb.
// The underlying LWGEOM will point into this WKB, so
// be extreamly careful when you free it!
//
// also, wkb should point to the 1st wkb character; NOT
// the postgresql length int32.
char *wkb_to_lwgeom(char *wkb, int SRID,int *size)
{
	uint32 wkbtype;
	char   dims;
	bool need_flip =  requiresflip( wkb[0] );

	LWPOINT *pt;
	LWLINE  *line;
	LWPOLY  *poly;
	char    *multigeom = NULL;
	char	*result = NULL;
	int simpletype;

	if (need_flip)
		flip_endian_int32(wkb+1);

	wkbtype = get_uint32(wkb+1);
 	dims = wkb_dims(wkbtype);
	simpletype = wkb_simpletype(wkbtype);

	switch (simpletype)
	{
		case POINTTYPE:
						pt = wkb_point_to_lwpoint(wkb, SRID);
						result  = lwpoint_serialize(pt);
						*size = lwpoint_findlength(result);
						pfree_point(pt);
						break;
		case LINETYPE:
						line = wkb_line_to_lwline(wkb, SRID);
						result  = lwline_serialize(line);
						*size = lwline_findlength(result);
						pfree_line(line);
						break;
		case POLYGONTYPE:
						poly = wkb_poly_to_lwpoly(wkb, SRID);
						result  = lwpoly_serialize(poly);
						*size = lwpoly_findlength(result);
						pfree_polygon(poly);
						break;
		case MULTIPOINTTYPE:
						result = multigeom;
						break;
		case MULTILINETYPE:
						break;
		case MULTIPOLYGONTYPE:
						break;
		case COLLECTIONTYPE:
						break;
	}
	return result;
}

//NOTE: THIS CHANGES THE ENDIAN OF THE WKB!!
// we make sure the point is correctly endianed
// and make a LWPOINT that points into it.
// wkb --> point to the endian definition of the wkb point
LWPOINT *wkb_point_to_lwpoint(char *wkb,int SRID)
{
	uint32 wkbtype;
	char   dims;
	char   simpletype;
	POINTARRAY *pa;

	bool need_flip =  requiresflip( wkb[0] );
	if (need_flip)
		flip_endian_int32(wkb+1);

	wkbtype = get_uint32(wkb+1);
 	dims = wkb_dims(wkbtype);
	simpletype = wkb_simpletype(wkbtype);

	if (simpletype != POINTTYPE)
		elog(ERROR,"WKB::point parser - go wrong type");

	pa = pointArray_construct(wkb+5, dims, 1);

	return lwpoint_construct(dims, SRID, pa);
}

LWLINE *wkb_line_to_lwline(char *wkb,int SRID)
{
		uint32 wkbtype;
		char   dims;
		char   simpletype;
		POINTARRAY *pa;
		int npoints;

	bool need_flip =  requiresflip( wkb[0] );
	if (need_flip)
		flip_endian_int32(wkb+1); // type

	wkbtype = get_uint32(wkb+1);
 	dims = wkb_dims(wkbtype);
	simpletype = wkb_simpletype(wkbtype);

	if (simpletype != LINETYPE)
		elog(ERROR,"WKB::line parser - go wrong type");

	if (need_flip)
		flip_endian_int32(wkb+5); // npoints

	npoints = get_uint32(wkb+5);

	if (need_flip)
		flipPoints(wkb+9,npoints,dims);

	pa = pointArray_construct(wkb+9, dims, npoints);

	return lwline_construct(dims, SRID, pa);
}

LWPOLY *wkb_poly_to_lwpoly(char *wkb,int SRID)
{
		uint32 wkbtype;
		char   dims;
		char   simpletype;
		POINTARRAY *pa;
		int npoints;
		POINTARRAY **rings;
		int nrings;
		int t;
		char *loc;
		int ptsize =16;

		bool need_flip =  requiresflip( wkb[0] );
		if (need_flip)
			flip_endian_int32(wkb+1); // type

		wkbtype = get_uint32(wkb+1);
		dims = wkb_dims(wkbtype);
		simpletype = wkb_simpletype(wkbtype);

		if (simpletype != POLYGONTYPE)
			elog(ERROR,"WKB::polygon parser - go wrong type");

		if (need_flip)
			flip_endian_int32(wkb+5); // nrings

		nrings = get_uint32(wkb+5);

		loc = wkb+5;

			//point size

		if (dims == FOURDIMS)
		{
			ptsize = 32;
		}
		if (dims == THREEDIMS)
		{
			ptsize = 24;
        }


		rings = (POINTARRAY **) palloc(sizeof(POINTARRAY*) * nrings);

		for (t=0;t<nrings;t++)
		{

			if (need_flip)
				flip_endian_int32(loc); // npoints

			npoints = get_uint32(loc);

					// read a ring
			if (need_flip)
				flipPoints(loc+4,npoints,dims);

			pa = pointArray_construct(loc+4, dims, npoints);

			loc += 4;
			loc += npoints * ptsize;
			rings[t] = pa;
		}
		return lwpoly_construct(dims, SRID, nrings,rings);
}


// takes a lwgeom and converts it to an appropriate wkb
//
// length of the wkb is returned in wkblength (this doesnt
//  include the 4 bytes needed for the postgresql int32 length)
//
char *lwgeom_to_wkb(char *serialized_form,int *wkblength,char desiredWKBEndian)
{
	char simple_type = lwgeom_getType(serialized_form[0]);
	char *result = NULL;
	LWPOINT *pt;
	LWLINE *line;
	LWPOLY *poly;
	char   *multigeom = NULL;


	switch (simple_type)
	{
		case POINTTYPE:
						pt = lwpoint_deserialize(serialized_form);
						result = lwpoint_to_wkb(pt, desiredWKBEndian, wkblength);
						pfree_point(pt);
						break;
		case LINETYPE:
						line = lwline_deserialize(serialized_form);
						result = lwline_to_wkb(line, desiredWKBEndian, wkblength);
						pfree_line(line);
						break;
		case POLYGONTYPE:
						poly = lwpoly_deserialize(serialized_form);
						result = lwpoly_to_wkb(poly, desiredWKBEndian, wkblength);
						pfree_polygon(poly );
						break;
		case MULTIPOINTTYPE:
						result  = multigeom;
						break;
		case MULTILINETYPE:
						break;
		case MULTIPOLYGONTYPE:
						break;
		case COLLECTIONTYPE:
						break;
	}
	return result;
}

char *lwpoint_to_wkb(LWPOINT *pt, char desiredWKBEndian, int *wkbsize)
{
	int ptsize = pointArray_ptsize(pt->point);
	char * result;
	uint32 wkbtype ;
	bool need_flip =  requiresflip( desiredWKBEndian );


	*wkbsize = 1+ 4+ ptsize; //endian, type, point

	result = palloc(*wkbsize);

	result[0] = desiredWKBEndian; //endian flag

	wkbtype = constructWKBType(POINTTYPE, pt->point->is3d);
	memcpy(result+1, &wkbtype, 4);
	if (need_flip)
		flip_endian_int32(result+1);

	memcpy(result+5, pt->point->serialized_pointlist, pointArray_ptsize(pt->point) );
	if (need_flip)
		flipPoints(result+5, 1, pt->point->is3d);

	return result;
}

char *lwline_to_wkb(LWLINE *line, char desiredWKBEndian, int *wkbsize)
{
		int ptsize = pointArray_ptsize(line->points);
		char * result;
		uint32 wkbtype ;
		bool need_flip =  requiresflip( desiredWKBEndian );


		*wkbsize = 1+ 4+ line->points->npoints * ptsize; //endian, type, points

		result = palloc(*wkbsize);

		result[0] = desiredWKBEndian; //endian flag

		wkbtype = constructWKBType(LINETYPE, line->points->is3d);
		memcpy(result+1, &wkbtype, 4);
		if (need_flip)
			flip_endian_int32(result+1);

		memcpy(result+5, line->points->serialized_pointlist, pointArray_ptsize(line->points) * line->points->npoints);
		if (need_flip)
			flipPoints(result+5, line->points->npoints, line->points->is3d);
		return result;
}

char *lwpoly_to_wkb(LWPOLY *poly, char desiredWKBEndian, int *wkbsize)
{
		int ptsize = pointArray_ptsize(poly->rings[0]);
		char * result;
		uint32 wkbtype ;
		bool need_flip =  requiresflip( desiredWKBEndian );
		int total_points =0;
		char *loc;
		int t;


		for (t=0;t<poly->nrings;t++)
		{
			total_points += poly->rings[t]->npoints;
		}

		*wkbsize = 1+ 4+ total_points * ptsize + 4* poly->nrings; //endian, type, all points, ring lengths

		result = palloc(*wkbsize);

		result[0] = desiredWKBEndian; //endian flag

		wkbtype = constructWKBType(POLYGONTYPE, poly->is3d);
		memcpy(result+1, &wkbtype, 4);  // type
		if (need_flip)
			flip_endian_int32(result+1);

		memcpy(result+5, &poly->nrings, 4);     // nrings
		if (need_flip)
			flip_endian_int32(result+5);

		loc = result+9;

		for (t=0;t<poly->nrings;t++)
		{
			int npoints =poly->rings[t]->npoints;
			memcpy( loc, &npoints, 4); // npoints
			if (need_flip)
				flip_endian_int32(loc);
			memcpy(loc+4, poly->rings[t]->serialized_pointlist, ptsize * npoints);
			if (need_flip)
				flipPoints(loc+4, npoints, poly->is3d);
			loc += 4+ ptsize * npoints;
		}
		return result;
}


bool requiresflip(char WKBendianflag)
{
	if (WKBendianflag == 1) // NDR
		return ( BYTE_ORDER != LITTLE_ENDIAN );
	if (WKBendianflag == 0) // xDR
		return ( BYTE_ORDER != BIG_ENDIAN );
	elog(ERROR,"WKB: endian flag isnt a 0 or 1.  WKB screwed?");
	return 0; //shouldnt get here
}

void swap_char(char	*a,char *b)
{
	char c;

	c = *a;
	*a=*b;
	*b=c;
}


void	flip_endian_double(char 	*d)
{
	swap_char(d+7, d);
	swap_char(d+6, d+1);
	swap_char(d+5, d+2);
	swap_char(d+4, d+3);
}

void		flip_endian_int32(char		*i)
{
	swap_char (i+3,i);
	swap_char (i+2,i+1);
}

// given a wkb type
// return twoDims, threeDims, or fourDims
char wkb_dims(uint32 type)
{
	if (type & 0x80000000)
		return THREEDIMS;
	if (type & 0x40000000)
		return FOURDIMS;
	return TWODIMS;
}


char wkb_simpletype (uint32 type)
{
	return type & 0x0F;
}

void flipPoints(char *pts, int npoints, char dims)
{
	int t;
	char *loc = pts;
	int size =16;

	if (dims == FOURDIMS)
	{
		size = 32;
	}
	if (dims == THREEDIMS)
	{
		size = 24;
	}

	for (t=0;t<npoints;t++)
	{
		flip_endian_double(loc);
		flip_endian_double(loc+8);
		if ( (dims == THREEDIMS)  || (dims == FOURDIMS) )
		{
			flip_endian_double(loc+16);
		}
		if (dims == FOURDIMS)
		{
			flip_endian_double(loc+24);
		}
		loc += size;
	}
}

uint32 constructWKBType(int simple_type, char dims)
{
	if (dims == TWODIMS)
		return simple_type;
	if (dims == THREEDIMS)
		return simple_type | 0x80000000;

	return simple_type | 0x40000000;
}

