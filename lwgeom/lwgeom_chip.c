#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "postgres.h"
#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"
#include "fmgr.h"
#include "utils/elog.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"

// External functions (what's again the reason for using explicit hex form ?)
extern void deparse_hex(unsigned char str, unsigned char *result);
extern unsigned char parse_hex(char *str);

// Internal funcs
void swap_char(char *a,char *b);
void flip_endian_double(char *d);
void flip_endian_int32(char *i);


// Prototypes
Datum CHIP_in(PG_FUNCTION_ARGS);
Datum CHIP_out(PG_FUNCTION_ARGS);
Datum CHIP_to_LWGEOM(PG_FUNCTION_ARGS);
Datum CHIP_getSRID(PG_FUNCTION_ARGS);
Datum CHIP_getFactor(PG_FUNCTION_ARGS);
Datum CHIP_setFactor(PG_FUNCTION_ARGS);
Datum CHIP_getDatatype(PG_FUNCTION_ARGS);
Datum CHIP_getCompression(PG_FUNCTION_ARGS);
Datum CHIP_getHeight(PG_FUNCTION_ARGS);
Datum CHIP_getWidth(PG_FUNCTION_ARGS);
Datum CHIP_setSRID(PG_FUNCTION_ARGS);


// input is a string with hex chars in it.  Convert to binary and put in the result
PG_FUNCTION_INFO_V1(CHIP_in);
Datum CHIP_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	CHIP *result;
	int size;
	int t;
	int input_str_len;
	int datum_size;

//printf("chip_in called\n");

	input_str_len = strlen(str);

	if  ( ( ( (int)(input_str_len/2.0) ) *2.0) != input_str_len)
	{
		elog(ERROR,"CHIP_in parser - should be even number of characters!");
		PG_RETURN_NULL();
	}

 	if (strspn(str,"0123456789ABCDEF") != strlen(str) )
	{
		elog(ERROR,"CHIP_in parser - input contains bad characters.  Should only have '0123456789ABCDEF'!");
		PG_RETURN_NULL();	
	}
	size = (input_str_len/2) ;
	result = (CHIP *) palloc(size);
	
	for (t=0;t<size;t++)
	{
		((unsigned char *)result)[t] = parse_hex( &str[t*2]) ;
	}
// if endian is wrong, flip it otherwise do nothing
	result->size = size;
	if (result->size < sizeof(CHIP) )
	{
		elog(ERROR,"CHIP_in parser - bad data (too small to be a CHIP)!");
		PG_RETURN_NULL();	
	}

	if (result->endian_hint != 1)
	{
		//need to do an endian flip
		flip_endian_int32( (char *)   &result->endian_hint);

		flip_endian_double((char *)  &result->bvol.xmin);
		flip_endian_double((char *)  &result->bvol.ymin);
		flip_endian_double((char *)  &result->bvol.zmin);

		flip_endian_double((char *)  &result->bvol.xmax);
		flip_endian_double((char *)  &result->bvol.ymax);
		flip_endian_double((char *)  &result->bvol.zmax);

		flip_endian_int32( (char *)  & result->SRID);	
			//dont know what to do with future[8] ...

		flip_endian_int32( (char *)  & result->height);	
		flip_endian_int32( (char *)  & result->width);
		flip_endian_int32( (char *)  & result->compression);
		flip_endian_int32( (char *)  & result->factor);
		flip_endian_int32( (char *)  & result->datatype);

	}
	if (result->endian_hint != 1 )
	{
		elog(ERROR,"CHIP_in parser - bad data (endian flag != 1)!");
		PG_RETURN_NULL();	
	}
	datum_size = 4;

	if ( (result->datatype == 6) || (result->datatype == 7) || (result->datatype == 106) || (result->datatype == 107) )
	{
		datum_size = 2;
	}
	if ( (result->datatype == 8) || (result->datatype == 108) )
	{
		datum_size=1;
	}

	if (result->compression ==0) //only true for non-compressed data
	{
		if (result->size != (sizeof(CHIP) + datum_size * result->width*result->height) )
		{
			elog(ERROR,"CHIP_in parser - bad data (actual size != computed size)!");
			PG_RETURN_NULL();
		}
	}

	PG_RETURN_POINTER(result);
}


//given a CHIP structure, convert it to Hex and put it in a string
PG_FUNCTION_INFO_V1(CHIP_out);
Datum CHIP_out(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *result;
	int size_result;
	int t;

//printf("chip_out called\n");

	size_result = (chip->size ) *2 +1; // +1 for null char
	result = palloc (size_result);
	result[size_result-1] = 0; //null terminate

	for (t=0; t< (chip->size); t++)
	{
		deparse_hex( ((unsigned char *) chip)[t], &result[t*2]);
	}
	PG_RETURN_CSTRING(result);	
}



PG_FUNCTION_INFO_V1(CHIP_to_LWGEOM);
Datum CHIP_to_LWGEOM(PG_FUNCTION_ARGS)
{
	CHIP *chip = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	POINT2D *pts = palloc(sizeof(POINT2D)*5);
	POINTARRAY *pa[1];
	LWPOLY *poly;
	char *ser;
	int wantbbox = false;
	
	// Assign coordinates to POINT2D array
	pts[0].x = chip->bvol.xmin; pts[0].y = chip->bvol.ymin;
	pts[1].x = chip->bvol.xmin; pts[1].y = chip->bvol.ymax;
	pts[2].x = chip->bvol.xmax; pts[2].y = chip->bvol.ymax;
	pts[3].x = chip->bvol.xmax; pts[3].y = chip->bvol.ymin;
	pts[4].x = chip->bvol.xmin; pts[4].y = chip->bvol.ymin;
	
	// Construct point array
	pa[0] = palloc(sizeof(POINTARRAY));
	pa[0]->serialized_pointlist = (char *)pts;
	pa[0]->ndims = 2;
	pa[0]->npoints = 5;

	// Construct polygon
	poly = lwpoly_construct(2, chip->SRID, 1, pa);

	// Serialize polygon
	ser = lwpoly_serialize(poly);

	// Construct PG_LWGEOM 
	result = PG_LWGEOM_construct(ser, chip->SRID, wantbbox);
	
	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(CHIP_getSRID);
Datum CHIP_getSRID(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->SRID);
}

PG_FUNCTION_INFO_V1(CHIP_getFactor);
Datum CHIP_getFactor(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_FLOAT4(c->factor);
}

PG_FUNCTION_INFO_V1(CHIP_setFactor);
Datum CHIP_setFactor(PG_FUNCTION_ARGS)
{ 
	CHIP 	*c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	float	factor = PG_GETARG_FLOAT4(1);
	CHIP  *result;

	result = (CHIP *) palloc(c->size);

	memcpy(result,c,c->size);
	result->factor = factor;

	PG_RETURN_POINTER(result);
}



PG_FUNCTION_INFO_V1(CHIP_getDatatype);
Datum CHIP_getDatatype(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->datatype);
}

PG_FUNCTION_INFO_V1(CHIP_getCompression);
Datum CHIP_getCompression(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->compression);
}

PG_FUNCTION_INFO_V1(CHIP_getHeight);
Datum CHIP_getHeight(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->height);
}

PG_FUNCTION_INFO_V1(CHIP_getWidth);
Datum CHIP_getWidth(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_INT32(c->width);
}


PG_FUNCTION_INFO_V1(CHIP_setSRID);
Datum CHIP_setSRID(PG_FUNCTION_ARGS)
{ 
	CHIP *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32 new_srid = PG_GETARG_INT32(1);
	CHIP *result;

	result = (CHIP *) palloc(c->size);

	memcpy(result, c, c->size);
	result->SRID = new_srid;

	PG_RETURN_POINTER(result);
}

void	flip_endian_double(char 	*d)
{
	swap_char(d+7, d);
	swap_char(d+6, d+1);
	swap_char(d+5, d+2);
	swap_char(d+4, d+3);
}

void swap_char(char	*a,char *b)
{
	char c;

	c = *a;
	*a=*b;
	*b=c;
}

void		flip_endian_int32(char		*i)
{
	swap_char (i+3,i);
	swap_char (i+2,i+1);
}
