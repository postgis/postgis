/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 *
 * Copyright (C) 2014 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <math.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "varint.h"

/**
* Used for passing the parse state between the parsing functions.
*/
typedef struct 
{
	/*General*/
	uint8_t *twkb; /* Points to start of TWKB */
	uint8_t *twkb_end; /* Expected end of TWKB */
	int check; /* Simple validity checks on geometries */
	/*Info about current geometry*/
	uint8_t magic_byte; /*the magic byte contain info about if twkb contain id, size info, bboxes and precision*/
	int ndims; /* Number of dimmensions */
	int has_z; /*TODO get rid of this*/
	int has_m; /*TODO get rid of this*/
	int has_bboxes;  
	double factor; /*precission factor to get the real numbers from the integers*/
	uint32_t lwtype; /* Current type we are handling */
	/*Parsing info*/
	int read_id; /*This is not telling if the twkb uses id or not. It is just for internal use to tell if id shall be read. Like the points inside multipoint doesn't have id*/
	uint8_t *pos; /* Current parse position */
	int64_t *coords; /*An array to keep delta values from 4 dimmensions*/
} twkb_parse_state;


/**
* Internal function declarations.
*/
LWGEOM* lwgeom_from_twkb_state(twkb_parse_state *s);


/**********************************************************************/

/**
* Check that we are not about to read off the end of the WKB 
* array.
*/
static inline void twkb_parse_state_check(twkb_parse_state *s, size_t next)
{
	
	if( (s->pos + next) > s->twkb_end)
		lwerror("TWKB structure does not match expected size!");
} 

/**
* Take in an unknown kind of wkb type number and ensure it comes out
* as an extended WKB type number (with Z/M/SRID flags masked onto the 
* high bits).
*/
static void lwtype_from_twkb_state(twkb_parse_state *s, uint8_t twkb_type)
{
	LWDEBUG(2,"Entering lwtype_from_twkb_state");

	
	switch (twkb_type)
	{
		case 1: 
			s->lwtype = POINTTYPE;
			break;
		case 2: 
			s->lwtype = LINETYPE;
			break;
		case 3:
			s->lwtype = POLYGONTYPE;
			break;
		case 4:
			s->lwtype = MULTIPOINTTYPE;
			break;
		case 5:
			s->lwtype = MULTILINETYPE;
			break;
		case 6:
			s->lwtype = MULTIPOLYGONTYPE;
			break;
		case 7: 
			s->lwtype = COLLECTIONTYPE;
			break;		

		default: /* Error! */
			lwerror("Unknown WKB type");
			break;	
	}

	LWDEBUGF(4,"Got lwtype %s (%u)", lwtype_name(s->lwtype), s->lwtype);

	return;
}

/**
* Byte
* Read a byte and advance the parse state forward.
*/
static uint8_t byte_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering byte_from_twkb_state");
	uint8_t return_value = 0;

	twkb_parse_state_check(s, WKB_BYTE_SIZE);
	LWDEBUG(4, "Passed state check");
	
	return_value = s->pos[0];
	s->pos += WKB_BYTE_SIZE;
	
	return return_value;
}


/**
* POINTARRAY
* Read a dynamically sized point array and advance the parse state forward.
* First read the number of points, then read the points.
*/
static POINTARRAY* ptarray_from_twkb_state(twkb_parse_state *s, uint32_t npoints)
{
	LWDEBUG(2,"Entering ptarray_from_twkb_state");
	POINTARRAY *pa = NULL;
	uint32_t ndims = s->ndims;
	int i,j;

	
	LWDEBUGF(4,"Pointarray has %d points", npoints);


	/* Empty! */
	if( npoints == 0 )
		return ptarray_construct(s->has_z, s->has_m, npoints);
	
	double *dlist;
	pa = ptarray_construct(s->has_z, s->has_m, npoints);
	dlist = (double*)(pa->serialized_pointlist);
	for( i = 0; i < npoints; i++ )
	{
		for (j=0;j<ndims;j++)
		{
			s->coords[j]+=varint_s64_read(&(s->pos), s->twkb_end);
			dlist[(ndims*i)+j] = s->coords[j]/s->factor;
		}
	}

	return pa;
}

/**
* POINT
*/
static LWPOINT* lwpoint_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering lwpoint_from_twkb_state");
	static uint32_t npoints = 1;

	/* TODO, make an option to use the id-value and return a set with geometry and id*/
	if((s->magic_byte&TWKB_ID) && s->read_id)
		varint_64_jump_n(&(s->pos),1, s->twkb_end); //Jump over id value
	
	POINTARRAY *pa = ptarray_from_twkb_state(s,npoints);	
	return lwpoint_construct(0, NULL, pa);
}

/**
* LINESTRING
*/
static LWLINE* lwline_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering lwline_from_twkb_state");
	uint32_t npoints;

	/* TODO, make an option to use the id-value and return a set with geometry and id*/
	if((s->magic_byte&TWKB_ID) && s->read_id)
		varint_64_jump_n(&(s->pos),1, s->twkb_end); //Jump over id value
	
	/*get number of points*/
	npoints = varint_u64_read(&(s->pos), s->twkb_end);
	
	POINTARRAY *pa = ptarray_from_twkb_state(s,npoints);	
	if( pa == NULL || pa->npoints == 0 )
		return lwline_construct_empty(0, s->has_z, s->has_m);

	if( s->check & LW_PARSER_CHECK_MINPOINTS && pa->npoints < 2 )
	{
		lwerror("%s must have at least two points", lwtype_name(s->lwtype));
		return NULL;
	}

	return lwline_construct(0, NULL, pa);
}

/**
* POLYGON
*/
static LWPOLY* lwpoly_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering lwpoly_from_twkb_state");
	uint32_t nrings,npoints;
	int i;
	/* TODO, make an option to use the id-value and return a set with geometry and id*/
	if((s->magic_byte&TWKB_ID) && s->read_id)
		varint_64_jump_n(&(s->pos),1, s->twkb_end); //Jump over id value
	
	/*get number of rings*/
	nrings= varint_u64_read(&(s->pos), s->twkb_end);
	
	LWPOLY *poly = lwpoly_construct_empty(0, s->has_z, s->has_m);

	LWDEBUGF(4,"Polygon has %d rings", nrings);
	
	/* Empty polygon? */
	if( nrings == 0 )
		return poly;



	for( i = 0; i < nrings; i++ )
	{
		/*get number of points*/
		npoints = varint_u64_read(&(s->pos), s->twkb_end);
		POINTARRAY *pa = ptarray_from_twkb_state(s,npoints);
		if( pa == NULL )
			continue;

		/* Check for at least four points. */
		if( s->check & LW_PARSER_CHECK_MINPOINTS && pa->npoints < 4 )
		{
			LWDEBUGF(2, "%s must have at least four points in each ring", lwtype_name(s->lwtype));
			lwerror("%s must have at least four points in each ring", lwtype_name(s->lwtype));
			return NULL;
		}

		/* Check that first and last points are the same. */
		if( s->check & LW_PARSER_CHECK_CLOSURE && ! ptarray_is_closed_2d(pa) )
		{
			/*TODO copy the first point to the last instead of error*/
			LWDEBUGF(2, "%s must have closed rings", lwtype_name(s->lwtype));
			lwerror("%s must have closed rings", lwtype_name(s->lwtype));
			return NULL;
		}
		
		/* Add ring to polygon */
		if ( lwpoly_add_ring(poly, pa) == LW_FAILURE )
		{
			LWDEBUG(2, "Unable to add ring to polygon");
			lwerror("Unable to add ring to polygon");
		}

	}
	return poly;	
}


/**
* MULTIPOINT
*/
static LWCOLLECTION* lwmultipoint_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering lwmultipoint_from_twkb_state");
	int ngeoms;
	LWGEOM *geom = NULL;
	
	/* TODO, make an option to use the id-value and return a set with geometry and id*/
	if((s->magic_byte&TWKB_ID) && s->read_id)
		varint_64_jump_n(&(s->pos),1, s->twkb_end); //Jump over id value
	
	/*Now we switch off id reading for subgeometries*/
	s->read_id=LW_FALSE;
	/*get number of geometries*/
	ngeoms= varint_u64_read(&(s->pos), s->twkb_end);	
	LWDEBUGF(4,"Number of geometries %d",ngeoms);
	LWCOLLECTION *col = lwcollection_construct_empty(s->lwtype,0, s->has_z, s->has_m);
	while (ngeoms>0)
	{
		geom = (LWGEOM*) lwpoint_from_twkb_state(s);
		if ( lwcollection_add_lwgeom(col, geom) == NULL )
		{
			lwerror("Unable to add geometry (%p) to collection (%p)", geom, col);
			return NULL;
		}
		ngeoms--;
	}
	/*Better turn it on again because we don't know what will come next*/
	s->read_id=LW_FALSE;
	return col;	
}

/**
* MULTILINESTRING
*/
static LWCOLLECTION* lwmultiline_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering lwmultilinestring_from_twkb_state");
	int ngeoms;
	LWGEOM *geom = NULL;
	
	/* TODO, make an option to use the id-value and return a set with geometry and id*/
	if((s->magic_byte&TWKB_ID) && s->read_id)
		varint_64_jump_n(&(s->pos),1, s->twkb_end); //Jump over id value
	
	/*Now we switch off id reading for subgeometries*/
	s->read_id=LW_FALSE;
	/*get number of geometries*/
	ngeoms= varint_u64_read(&(s->pos), s->twkb_end);	
	LWDEBUGF(4,"Number of geometries %d",ngeoms);
	LWCOLLECTION *col = lwcollection_construct_empty(s->lwtype,0, s->has_z, s->has_m);
	while (ngeoms>0)
	{
		geom = (LWGEOM*) lwline_from_twkb_state(s);
		if ( lwcollection_add_lwgeom(col, geom) == NULL )
		{
			lwerror("Unable to add geometry (%p) to collection (%p)", geom, col);
			return NULL;
		}
		ngeoms--;
	}
	/*Better turn it on again because we don't know what will come next*/
	s->read_id=LW_FALSE;
	return col;	
}

/**
* MULTIPOLYGON
*/
static LWCOLLECTION* lwmultipoly_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering lwmultipolygon_from_twkb_state");
	int ngeoms;
	LWGEOM *geom = NULL;
	
	/* TODO, make an option to use the id-value and return a set with geometry and id*/
	if((s->magic_byte&TWKB_ID) && s->read_id)
		varint_64_jump_n(&(s->pos),1, s->twkb_end); //Jump over id value
	
	/*Now we switch off id reading for subgeometries*/
	s->read_id=LW_FALSE;
	/*get number of geometries*/
	ngeoms= varint_u64_read(&(s->pos), s->twkb_end);	
	LWDEBUGF(4,"Number of geometries %d",ngeoms);
	LWCOLLECTION *col = lwcollection_construct_empty(s->lwtype,0, s->has_z, s->has_m);
	while (ngeoms>0)
	{
		geom = (LWGEOM*) lwpoly_from_twkb_state(s);
		if ( lwcollection_add_lwgeom(col, geom) == NULL )
		{
			lwerror("Unable to add geometry (%p) to collection (%p)", geom, col);
			return NULL;
		}
		ngeoms--;
	}
	/*Better turn it on again because we don't know what will come next*/
	s->read_id=LW_FALSE;
	return col;	
}


/**
* COLLECTION, MULTIPOINTTYPE, MULTILINETYPE, MULTIPOLYGONTYPE
**/
static LWCOLLECTION* lwcollection_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering lwcollection_from_twkb_state");
	int ngeoms;
	LWGEOM *geom = NULL;
	
	/* TODO, make an option to use the id-value and return a set with geometry and id*/
	if((s->magic_byte&TWKB_ID) && s->read_id)
		varint_64_jump_n(&(s->pos),1, s->twkb_end); //Jump over id value
	
	/*Now we switch off id reading for subgeometries*/
	s->read_id=LW_FALSE;
		
	/*get number of geometries*/
	ngeoms= varint_u64_read(&(s->pos), s->twkb_end);	
	LWDEBUGF(4,"Number of geometries %d",ngeoms);
	LWCOLLECTION *col = lwcollection_construct_empty(s->lwtype,0, s->has_z, s->has_m);
	while (ngeoms>0)
	{
		geom = lwgeom_from_twkb_state(s);
		if ( lwcollection_add_lwgeom(col, geom) == NULL )
		{
			lwerror("Unable to add geometry (%p) to collection (%p)", geom, col);
			return NULL;
		}
		ngeoms--;
	}
	/*We better turn id reading on again because we don't know what will come next*/
	s->read_id=LW_FALSE;
	return col;	
}


static void magicbyte_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering magicbyte_from_twkb_state");
	
	int  precission;
	s->has_bboxes=LW_FALSE;
	
	/*Get the first magic byte*/
	s->magic_byte = byte_from_twkb_state(s);
	
	/*Read precission from the last 4 bits of the magic_byte*/
	
	precission=(s->magic_byte&0xF0)>>4;
	s->factor=pow(10,1.0*precission);
	
	/*If the twkb-geometry has size information we just jump over it*/
	if(s->magic_byte&TWKB_SIZES)
		varint_64_jump_n(&(s->pos),1, s->twkb_end); //Jump over size information
	
	/*If our dataset has bboxes we just set a flag for that. We cannot do anything about it before we know the number of dimmensions*/
	if(s->magic_byte&TWKB_BBOXES)
		s->has_bboxes=LW_TRUE;
}

/**
* GEOMETRY
* Generic handling for WKB geometries. The front of every WKB geometry
* (including those embedded in collections) is an endian byte, a type
* number and an optional srid number. We handle all those here, then pass
* to the appropriate handler for the specific type.
*/
LWGEOM* lwgeom_from_twkb_state(twkb_parse_state *s)
{
	LWDEBUG(2,"Entering lwgeom_from_twkb_state");
	
	
	uint8_t type_dim_byte;
	uint8_t twkb_type;
		
	/* Read the type and number of dimmensions*/
	type_dim_byte = byte_from_twkb_state(s);	
	
	twkb_type=type_dim_byte&0x1F;	
	s->ndims=(type_dim_byte&0xE0)>>5;	
	
	s->has_z=LW_FALSE;
	s->has_m=LW_FALSE;
	if(s->ndims>2) s->has_z=LW_TRUE;
	if(s->ndims>3) s->has_m=LW_TRUE;	
	
	/*Now we know number of dommensions so we can jump over the bboxes with right number of "jumps"*/
	if (s->has_bboxes)
	{
		varint_64_jump_n(&(s->pos),2*(s->ndims), s->twkb_end); //Jump over bbox
		/*We only have bboxes at top level, so once found we forget about it*/
		s->has_bboxes=LW_FALSE;
	}
	
	lwtype_from_twkb_state(s, twkb_type);
	
		
	/* Do the right thing */
	switch( s->lwtype )
	{
		case POINTTYPE:
			return (LWGEOM*)lwpoint_from_twkb_state(s);
			break;
		case LINETYPE:
			return (LWGEOM*)lwline_from_twkb_state(s);
			break;
		case POLYGONTYPE:
			return (LWGEOM*)lwpoly_from_twkb_state(s);
			break;
		case MULTIPOINTTYPE:
			return (LWGEOM*)lwmultipoint_from_twkb_state(s);
			break;
		case MULTILINETYPE:
			return (LWGEOM*)lwmultiline_from_twkb_state(s);
			break;
		case MULTIPOLYGONTYPE:
			return (LWGEOM*)lwmultipoly_from_twkb_state(s);
			break;
		case COLLECTIONTYPE:
			return (LWGEOM*)lwcollection_from_twkb_state(s);
			break;

		/* Unknown type! */
		default:
			lwerror("Unsupported geometry type: %s [%d]", lwtype_name(s->lwtype), s->lwtype);
	}

	/* Return value to keep compiler happy. */
	return NULL;
	
}

/* TODO add check for SRID consistency */

/**
* WKB inputs *must* have a declared size, to prevent malformed WKB from reading
* off the end of the memory segment (this stops a malevolent user from declaring
* a one-ring polygon to have 10 rings, causing the WKB reader to walk off the 
* end of the memory).
*
* Check is a bitmask of: LW_PARSER_CHECK_MINPOINTS, LW_PARSER_CHECK_ODD, 
* LW_PARSER_CHECK_CLOSURE, LW_PARSER_CHECK_NONE, LW_PARSER_CHECK_ALL
*/
LWGEOM* lwgeom_from_twkb(uint8_t *twkb, size_t twkb_size, char check)
{
	LWDEBUG(2,"Entering lwgeom_from_twkb");
	twkb_parse_state s;
	
	/* Initialize the state appropriately */
	s.twkb = twkb;
	s.twkb_end = twkb+twkb_size;
	s.check = check;
	s.ndims= 0;
	int64_t coords[4] = {0,0,0,0};
	s.coords=coords;
	s.pos = twkb;
	s.read_id=LW_TRUE; /*This is just to tell that IF the twkb uses id it should be read. The functions can switch this off for reading subgeometries for instance*/
	LWDEBUGF(4,"twkb_size: %d",(int) twkb_size);
	/* Hand the check catch-all values */
	if ( check & LW_PARSER_CHECK_NONE ) 
		s.check = 0;
	else
		s.check = check;
	/*read the first byte and put the result in twkb_state*/
	magicbyte_from_twkb_state(&s);
	return lwgeom_from_twkb_state(&s);
}
