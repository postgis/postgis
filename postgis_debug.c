
/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************
 * $Log$
 * Revision 1.10  2003/07/25 17:08:37  pramsey
 * Moved Cygwin endian define out of source files into postgis.h common
 * header file.
 *
 * Revision 1.9  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 *
 **********************************************************************/

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


#include "postgis.h"
#include "utils/elog.h"


#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


// #define DEBUG_GIST
//#define DEBUG_GIST2

void print_box2d(BOX *box);


void dump_bytes( char *a, int numb)
{
	int	t;

	for (t=0; t<numb; t++)
	{
		printf("	+ Byte #%i has value %i (%x)\n",t,a[t],a[t]);
	}
}


//debug function - whats really in that BOX3D?
void print_box(BOX3D *box)
{
	printf("box is at %p\n",box);
	if (box == NULL)
	{
		printf ("	+ BOX IS NULL\n");
		return;
	}
	printf("	+ LLB = [%g,%g,%g]\n", box->LLB.x, box->LLB.y,box->LLB.z);
	printf("	+ URT = [%g,%g,%g]\n", box->URT.x, box->URT.y,box->URT.z);
}

void print_box2d(BOX *box)
{
	printf (" BOX[%g %g , %g %g] ",box->low.x, box->low.y, box->high.x, box->high.y);
}

//debug function - whats really in that BOX3D?
void print_box_oneline(BOX3D *box)
{
	printf(" (%p) {",box);
	if (box == NULL)
	{
		printf ("BOX IS NULL}");
		return;
	}
	printf("[%g,%g,%g] ", box->LLB.x, box->LLB.y,box->LLB.z);
	printf("[%g,%g,%g]}", box->URT.x, box->URT.y,box->URT.z);
}

//find the size of geometry
PG_FUNCTION_INFO_V1(mem_size);
Datum mem_size(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(geom1->size);
}

//find the size of geometry
PG_FUNCTION_INFO_V1(numb_sub_objs);
Datum numb_sub_objs(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(geom1->nobjs);
}

//get summary info on a GEOMETRY
PG_FUNCTION_INFO_V1(summary);
Datum summary(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j,i;
	POLYGON3D			*poly;
	LINE3D			*line;
	char				*result;
	int				size;
	char				tmp[100];
	text				*mytext;


	size = 1;
	result = palloc(1);
	result[0] = 0; //null terminate it


	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;
		type1=  geom1->objType[j];

		if (type1 == POINTTYPE)	//point
		{
			size += 30;
			result = repalloc(result,size);
			sprintf(tmp,"Object %i is a POINT()\n",j);
			strcat(result,tmp);
		}

		if (type1 == LINETYPE)	//line
		{
			line = (LINE3D *) o1;

			size += 57;
			result = repalloc(result,size);
			sprintf(tmp,"Object %i is a LINESTRING() with %i points\n",j,line->npoints);
			strcat(result,tmp);
		}

		if (type1 == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;

			size += 57*(poly->nrings +1);
			result = repalloc(result,size);
			sprintf(tmp,"Object %i is a POLYGON() with %i rings\n",j,poly->nrings);
			strcat(result,tmp);
			for (i=0; i<poly->nrings;i++)
			{
				sprintf(tmp,"     + ring %i has %i points\n",i,poly->npoints[i]);
				strcat(result,tmp);
			}
		}
	}

		// create a text obj to return
	mytext = (text *) palloc(VARHDRSZ  + strlen(result) );
	VARATT_SIZEP(mytext) = VARHDRSZ + strlen(result) ;
	memcpy(VARDATA(mytext) , result, strlen(result) );
	pfree(result);
	PG_RETURN_POINTER(mytext);
}



//  DO NOT USE THESE decoding function (except for debugging purposes)
//    The code is NOT maintained and is thrown together


// unsupported debugging function.
// given a wkb input thats a geometrycollection, returns its size and prints out
// its contents
//
//  Its really messy - dont even think about using this for anything
//
// you shouldnt call this function; just call decode_wkb() and it will
// call this function
//
void decode_wkb_collection(char *wkb,int	*size)
{
	int	offset =0;
	bool	flipbytes;
	int	total_size=0,sub_size;
	int	numb_sub,t;
	bool	first_one = TRUE;


	if (wkb[0] ==0 )  //big endian
	{
		if (BYTE_ORDER == LITTLE_ENDIAN)
			flipbytes= 1;
		else
			flipbytes= 0;
	}
	else //little
	{
		if (BYTE_ORDER == LITTLE_ENDIAN)
			flipbytes= 0;
		else
			flipbytes= 1;
	}

	memcpy(&numb_sub, wkb+5,4);
	if (flipbytes)
		flip_endian_int32( (char *) & numb_sub) ;

	printf("GEOMETRYCOLLECTION(\n");
	offset = 9;
	for (t=0;t<numb_sub;t++)
	{
			if (first_one)
			{
				first_one = FALSE;
			}
			else
			{
				printf(",");
			}

		printf("	");
		decode_wkb( wkb + offset, &sub_size);
		total_size += sub_size;
		offset += sub_size;
	}
	*size = total_size +9 ;
	printf(")\n");
	return;
}


// unsupported debugging function.
// given a wkb input, returns its size and prints out
// its contents
//
//  Its really messy - dont even think about using this for anything
//
//dump wkb to screen (for debugging)
// assume endian is constant though out structure
void decode_wkb(char *wkb, int *size)
{

	bool	flipbytes;
	uint32	type;
	uint32	n1,n2,n3,t,u,v;
	bool	is3d;
	bool first_one,first_one2,first_one3;

	int	offset,offset1;
	double	x,y,z;
	int	total_points;



	//printf("decoding wkb\n");


	if (wkb[0] ==0 )  //big endian
	{
		if (BYTE_ORDER == LITTLE_ENDIAN)
			flipbytes= 1;
		else
			flipbytes= 0;
	}
	else //little
	{
		if (BYTE_ORDER == LITTLE_ENDIAN)
			flipbytes= 0;
		else
			flipbytes= 1;
	}

	//printf("	+ flipbytes = %i\n", flipbytes);

	//printf("info about wkb:\n");
	memcpy(&type, wkb+1,4);
	if (flipbytes)
		flip_endian_int32( (char *) & type) ;

	is3d = 0;

	if (type > 1000 )
	{
		is3d = 1;
		type = type - 32768;
	}
	//printf("	Type = %i (is3d = %i)\n", type, is3d);
	if (type == 1)  //POINT()
	{
		printf("POINT(");
		if (is3d)
		{
			memcpy(&x, &wkb[5], 8);
			memcpy(&y, &wkb[5+8], 8);
			memcpy(&z, &wkb[5+16], 8);
			if (flipbytes)
			{
				flip_endian_double( (char *) & x) ;
				flip_endian_double( (char *) & y) ;
				flip_endian_double( (char *) & z) ;
			}
			printf("%g %g %g)",x,y,z );
		}
		else
		{
			memcpy(&x, &wkb[5], 8);
			memcpy(&y, &wkb[5+8], 8);
			if (flipbytes)
			{
				flip_endian_double( (char *) & x) ;
				flip_endian_double( (char *) & y) ;
			}
			printf("%g %g)", x,y);
		}
		printf("\n");
		if (is3d)
			*size = 29;
		else
			*size = 21;
		return;

	}
	if (type == 2)
	{
		printf("LINESTRING(");
		memcpy(&n1, &wkb[5],4);
		if (flipbytes)
			flip_endian_int32( (char *) & n1) ;
	//	printf("  --- has %i sub points\n",n1);
		first_one = TRUE;
		for (t=0; t<n1; t++)
		{
			if (first_one)
			{
				first_one = FALSE;
			}
			else
			{
				printf(",");
			}
			if (is3d)
			{
				memcpy(&x, &wkb[9+t*24],8);
				memcpy(&y, &wkb[9+t*24+8],8);
				memcpy(&z, &wkb[9+t*24+16],8);
				if (flipbytes)
				{
					flip_endian_double( (char *) & x) ;
					flip_endian_double( (char *) & y) ;
					flip_endian_double( (char *) & z) ;
				}
				printf("%g %g %g",x,y,z);
			}
			else
			{
				memcpy(&x, &wkb[9+t*16],8);
				memcpy(&y, &wkb[9+t*16+8],8);
				if (flipbytes)
				{
					flip_endian_double( (char *) & x) ;
					flip_endian_double( (char *) & y) ;
				}
				printf("%g %g",x,y);
			}
		}


		printf(")\n");
		if (is3d)
			*size = 9 + n1*24;
		else
			*size = 9 + n1*16;
		return;
	}
	if (type == 3)
	{
		*size = 9;
		printf("POLYGON(");
		memcpy(&n1, &wkb[5],4);
		if (flipbytes)
			flip_endian_int32( (char *) & n1) ;
		//printf("  --- has %i rings\n",n1);
		*size += 4*n1;
		offset= 9;
		first_one = TRUE;
		for (u=0; u<n1; u++)
		{
			memcpy(&n2, &wkb[offset],4);
			if (flipbytes)
				flip_endian_int32( (char *) & n2) ;
		//	printf(" ring %i: has %i points\n",u,n2);


			if (first_one)
			{
				first_one = FALSE;
			}
			else
			{
			  	printf(",");
			}
			printf("(");

			first_one2 = TRUE;
			for (v=0; v< n2; v++)
			{
			if (first_one2)
			{
				first_one2 = FALSE;
			}
			else
			{
			  	printf(",");
			}
				if (is3d)
				{
					memcpy(&x, &wkb[offset + 4+ v*24],8);
					memcpy(&y, &wkb[offset + 4+ v*24 + 8],8);
					memcpy(&z, &wkb[offset + 4+ v*24 + 16],8);
					if (flipbytes)
					{
						flip_endian_double( (char *) & x) ;
						flip_endian_double( (char *) & y) ;
						flip_endian_double( (char *) & z) ;
					}
					printf("%g %g %g",x,y,z);

				}
				else
				{
					memcpy(&x, &wkb[offset +4 +v*16],8);
					memcpy(&y, &wkb[offset +4 +v*16 + 8],8);
					if (flipbytes)
					{
						flip_endian_double( (char *) & x) ;
						flip_endian_double( (char *) & y) ;
					}
					printf("%g %g",x,y);

				}
			}
			if (is3d)
			{
				offset +=4 +24*n2;
				*size += n2*24;
			}
			else
			{
				offset += 4+ 16*n2;
				*size += n2*16;
			}
			printf(")");
		}

		printf(")\n");

		return;

	}
	if (type == 4)
	{
		printf("MULTIPOINT(");
		memcpy(&n1,&wkb[5],4);
		if (flipbytes)
			flip_endian_int32( (char *) & n1) ;
	//	printf(" -- has %i points\n",n1);
		if (is3d)
			*size = 9 + n1*29;
		else
			*size = 9 + n1*21;
		first_one = TRUE;
		for (t=0; t<n1; t++)
		{
			if (first_one)
			{
				first_one= FALSE;
			}
			else
			{
				printf(",");
			}
			if (is3d)
			{
				memcpy(&x, &wkb[9+t*29+5],8);
				memcpy(&y, &wkb[9+t*29+8+5],8);
				memcpy(&z, &wkb[9+t*29+16+5],8);
				if (flipbytes)
				{
					flip_endian_double( (char *) & x) ;
					flip_endian_double( (char *) & y) ;
					flip_endian_double( (char *) & z) ;
				}
				printf("%g %g %g",x,y,z);
			}
			else
			{
				memcpy(&x, &wkb[9+t*21+5],8);
				memcpy(&y, &wkb[9+t*21+8+5],8);
				if (flipbytes)
				{
					flip_endian_double( (char *) & x) ;
					flip_endian_double( (char *) & y) ;
				}
				printf("%g %g",x,y);
			}
		}
		printf (")\n");
		return;
	}
	if (type == 5)
	{
		*size = 9;
		printf("MULTILINESTRING(");
		memcpy(&n2,&wkb[5],4);
		if (flipbytes)
			flip_endian_int32( (char *) & n2) ;
	//	printf(" -- has %i sub-lines\n",n2);
		*size += 9 *n2;
		offset =9;
		first_one2 = TRUE;
		for (u=0; u<n2; u++)
		{
			if (first_one2)
			{
				first_one2= FALSE;
			}
			else
			{
				printf(",");
			}
		printf("(");
		memcpy(&n1, &wkb[5 +offset],4);
		if (flipbytes)
			flip_endian_int32( (char *) & n1) ;
	//	printf("  --- has %i sub points\n",n1);
		first_one = TRUE;
		for (t=0; t<n1; t++)
		{
			if (first_one)
			{
				first_one= FALSE;
			}
			else
			{
				printf(",");
			}

			if (is3d)
			{
				memcpy(&x, &wkb[offset+9+t*24],8);
				memcpy(&y, &wkb[offset+9+t*24+8],8);
				memcpy(&z, &wkb[offset+9+t*24+16],8);
				if (flipbytes)
				{
					flip_endian_double( (char *) & x) ;
					flip_endian_double( (char *) & y) ;
					flip_endian_double( (char *) & z) ;
				}
				printf("%g %g %g",x,y,z);
			}
			else
			{
				memcpy(&x, &wkb[offset+9+t*16],8);
				memcpy(&y, &wkb[offset+9+t*16+8],8);
				if (flipbytes)
				{
					flip_endian_double( (char *) & x) ;
					flip_endian_double( (char *) & y) ;
				}
				printf("%g %g",x,y);
			}
		}
		printf(")");
		if (is3d)
		{
			*size += (24*n1);
			offset += 9 + (24*n1);
		}
		else
		{
			*size += (16*n1);
			offset += 9 + (16*n1);
		}
		}
		printf(")\n");
		return;

	}
	if (type == 6)
	{
		*size = 9;
		printf("MULTIPOLYGON(");
		memcpy(&n3,&wkb[5],4);
		if (flipbytes)
			flip_endian_int32( (char *) & n3) ;
		//printf(" -- has %i sub-poly\n",n3);
		*size += 9*n3;
		offset1 =9;//where polygon starts
		first_one3= TRUE;
	for (t=0;t<n3; t++)  //for each polygon
	{
			if (first_one3)
			{
				first_one3= FALSE;
			}
			else
			{
				printf(",");
			}
		printf("(");
		//printf("polygon #%i\n",t);
		total_points = 0;
		memcpy(&n1,&wkb[offset1+5],4); //# rings
		*size += 4*n1;
		if (flipbytes)
			flip_endian_int32( (char *) & n1) ;
		//printf("This polygon has %i rings\n",n1);
		offset = offset1+9; //where linear rings are
		first_one = TRUE;
		for (u=0; u<n1; u++) //for each ring
		{
			if (first_one)
			{
				first_one= FALSE;
			}
			else
			{
				printf(",");
			}
			printf("(");
			memcpy(&n2, &wkb[offset],4);
			if (flipbytes)
				flip_endian_int32( (char *) & n2) ; //pts in linear ring
		//	printf(" ring %i: has %i points\n",u,n2);
			total_points += n2;
			first_one2 = TRUE;
			for (v=0; v< n2; v++)	//for each point
			{
			if (first_one2)
			{
				first_one2= FALSE;
			}
			else
			{
				printf(",");
			}
				if (is3d)
				{
					memcpy(&x, &wkb[offset + 4+ v*24],8);
					memcpy(&y, &wkb[offset + 4+ v*24 + 8],8);
					memcpy(&z, &wkb[offset + 4+ v*24 + 16],8);
					if (flipbytes)
					{
						flip_endian_double( (char *) & x) ;
						flip_endian_double( (char *) & y) ;
						flip_endian_double( (char *) & z) ;
					}
					printf("%g %g %g",x,y,z);
				}
				else
				{
					memcpy(&x, &wkb[offset +4 +v*16],8);
					memcpy(&y, &wkb[offset +4 +v*16 + 8],8);
					if (flipbytes)
					{
						flip_endian_double( (char *) & x) ;
						flip_endian_double( (char *) & y) ;
					}
					printf("%g %g",x,y);
				}
			}
			if (is3d)
			{
				*size += 24*n2;
				offset += 4+ 24*n2;
			}
			else
			{
				*size += 16*n2;
				offset += 4+ 16*n2;
			}
			printf(")");
		}
		printf(")");
		if (is3d)
			offset1 +=9 +24*total_points +4*n1;
		else
			offset1 += 9+ 16*total_points +4*n1;
	}
	printf(")\n");
	return;
	}
	if (type == 7)
	{
		decode_wkb_collection(wkb, size);
	}
}


char *print_geometry(GEOMETRY *geom)
{
	char	*text;

	text = (char *) DatumGetPointer(DirectFunctionCall1(geometry_out,PointerGetDatum(geom) ) ) ;

	printf( "%s", text);
	return text;
}

void print_point_debug(POINT3D *p)
{
	printf("[%g %g %g]\n",p->x,p->y,p->z);
}


