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



// input is a string with hex chars in it.  Convert to binary and put in the result
PG_FUNCTION_INFO_V1(CHIP_in);
Datum CHIP_in(PG_FUNCTION_ARGS)
{
	char	   		*str = PG_GETARG_CSTRING(0);
	CHIP		 	*result;
	int			size;
	int			t;
	int			input_str_len;
	int			datum_size;

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

		flip_endian_double((char *)  &result->bvol.LLB.x);
		flip_endian_double((char *)  &result->bvol.LLB.y);
		flip_endian_double((char *)  &result->bvol.LLB.z);

		flip_endian_double((char *)  &result->bvol.URT.x);
		flip_endian_double((char *)  &result->bvol.URT.y);
		flip_endian_double((char *)  &result->bvol.URT.z);

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
	CHIP				      *chip = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char					*result;
	int					size_result;
	int					t;

//printf("chip_out called\n");

	size_result = (chip->size ) *2 +1; //+1 for null char
	result = palloc (size_result);
	result[size_result-1] = 0; //null terminate

	for (t=0; t< (chip->size); t++)
	{
		deparse_hex( ((unsigned char *) chip)[t], &result[t*2]);			
	}
	PG_RETURN_CSTRING(result);	
}



PG_FUNCTION_INFO_V1(CHIP_to_geom);
Datum CHIP_to_geom(PG_FUNCTION_ARGS)
{
	CHIP				      *chip = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY				*result;
	
	POLYGON3D			*poly;
	POINT3D			pts[5];	//5 points around box
	int				pts_per_ring[1];
	int				poly_size;
	
	//use LLB's z value (we're going to set is3d to false)

	set_point( &pts[0], chip->bvol.LLB.x , chip->bvol.LLB.y , chip->bvol.LLB.z );
	set_point( &pts[1], chip->bvol.URT.x , chip->bvol.LLB.y , chip->bvol.LLB.z );
	set_point( &pts[2], chip->bvol.URT.x , chip->bvol.URT.y , chip->bvol.LLB.z );
	set_point( &pts[3], chip->bvol.LLB.x , chip->bvol.URT.y , chip->bvol.LLB.z );
	set_point( &pts[4], chip->bvol.LLB.x , chip->bvol.LLB.y , chip->bvol.LLB.z );
	
	pts_per_ring[0] = 5; //ring has 5 points

		//make a polygon
	poly = make_polygon(1, pts_per_ring, pts, 5, &poly_size);

	result = make_oneobj_geometry(poly_size, (char *)poly, POLYGONTYPE, FALSE,chip->SRID, 1.0, 0.0, 0.0);

	PG_RETURN_POINTER(result);	

}

PG_FUNCTION_INFO_V1(srid_chip);
Datum srid_chip(PG_FUNCTION_ARGS)
{ 
	CHIP		   *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(c->SRID);
}

PG_FUNCTION_INFO_V1(factor_chip);
Datum factor_chip(PG_FUNCTION_ARGS)
{ 
	CHIP		   *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_FLOAT4(c->factor);
}


PG_FUNCTION_INFO_V1(datatype_chip);
Datum datatype_chip(PG_FUNCTION_ARGS)
{ 
	CHIP		   *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(c->datatype);
}

PG_FUNCTION_INFO_V1(compression_chip);
Datum compression_chip(PG_FUNCTION_ARGS)
{ 
	CHIP		   *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(c->compression);
}


PG_FUNCTION_INFO_V1(height_chip);
Datum height_chip(PG_FUNCTION_ARGS)
{ 
	CHIP		   *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(c->height);
}

PG_FUNCTION_INFO_V1(width_chip);
Datum width_chip(PG_FUNCTION_ARGS)
{ 
	CHIP		   *c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(c->width);
}



PG_FUNCTION_INFO_V1(setsrid_chip);
Datum setsrid_chip(PG_FUNCTION_ARGS)
{ 
	CHIP 	*c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32	new_srid = PG_GETARG_INT32(1);
	CHIP  *result;

	result = (CHIP *) palloc(c->size);

	memcpy(result,c,c->size);
	result->SRID = new_srid;

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(setfactor_chip);
Datum setfactor_chip(PG_FUNCTION_ARGS)
{ 
	CHIP 	*c = (CHIP *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	float	factor = PG_GETARG_FLOAT4(1);
	CHIP  *result;

	result = (CHIP *) palloc(c->size);

	memcpy(result,c,c->size);
	result->factor = factor;

	PG_RETURN_POINTER(result);
}
