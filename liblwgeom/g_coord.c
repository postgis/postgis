/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "libgeom.h"

GCOORDINATE* gcoord_new(int ndims)
{
	GCOORDINATE *coord = (GCOORDINATE*)lwalloc(sizeof(GCOORDINATE));
	assert(ndims >= 2);
	assert(ndims <= 4);
	if( ! coord ) 
	{
		lwerror("Out of memory!");
		return NULL;
	}
	coord->ordinates = (double*)lwalloc(sizeof(double) * ndims);
	if( ! coord->ordinates ) 
	{
		lwerror("Out of memory!");
		return NULL;
	}
	/* We'll determine extra dimension is Z or M later. */
	if( ndims == 3 ) 
	{
		coord->flags = gflags(1, 0, 0);
	}
	if( ndims == 4 ) 
	{
		coord->flags = gflags(1, 1, 0);
	}
	return coord;
}

GCOORDINATE* gcoord_new_with_flags(uchar flags)
{
	GCOORDINATE *coord = (GCOORDINATE*)lwalloc(sizeof(GCOORDINATE));
	if( ! coord ) 
	{
		lwerror("Out of memory!");
		return NULL;
	}
	coord->ordinates = (double*)lwalloc(sizeof(double) * FLAGS_NDIMS(flags));
	if( ! coord->ordinates ) 
	{
		lwerror("Out of memory!");
		return NULL;
	}
	coord->flags = flags;
	return coord;
}

GCOORDINATE* gcoord_new_with_flags_and_ordinates(uchar flags, double *ordinates)
{
	GCOORDINATE *coord = (GCOORDINATE*)lwalloc(sizeof(GCOORDINATE));
	assert(ordinates);
	if( ! coord ) 
	{
		lwerror("Out of memory!");
		return NULL;
	}
	coord->ordinates = ordinates;
	if( ! coord->ordinates ) 
	{
		lwerror("Out of memory!");
		return NULL;
	}
	coord->flags = flags;
	FLAGS_SET_READONLY(coord->flags, 1);
	return coord;
}

GCOORDINATE* gcoord_copy(GCOORDINATE *coord)
{
	GCOORDINATE *copy = NULL;
	
	assert(coord);
	assert(coord->ordinates);
	
	copy = (GCOORDINATE*)lwalloc(sizeof(GCOORDINATE));
	if( ! copy ) return NULL;
	copy->flags = coord->flags;
	FLAGS_SET_READONLY(copy->flags, 1);
	copy->ordinates = (double*)lwalloc(sizeof(double) * FLAGS_NDIMS(copy->flags));
	if( ! copy->ordinates ) return NULL;
	memcpy(copy->ordinates, coord->ordinates, FLAGS_NDIMS(copy->flags) * sizeof(double));
	return copy;
}

void gcoord_free(GCOORDINATE *coord)
{
	if ( ! coord ) return;
	if ( ! FLAGS_GET_READONLY(coord->flags) && coord->ordinates )
		lwfree(coord->ordinates);
	lwfree(coord);
}

void gcoord_set_x(GCOORDINATE *coord, double x)
{
	assert(coord);
	*(coord->ordinates) = x;
}

void gcoord_set_y(GCOORDINATE *coord, double y)
{
	assert(coord);
	*(coord->ordinates + 1) = y;
}

void gcoord_set_z(GCOORDINATE *coord, double z)
{
	assert(coord);
	assert(FLAGS_GET_Z(coord->flags));
	*(coord->ordinates + 2) = z;
}

void gcoord_set_m(GCOORDINATE *coord, double m)
{
	assert(coord);
	assert(FLAGS_GET_M(coord->flags));
	if(FLAGS_GET_Z(coord->flags))
	{
		*(coord->ordinates + 3) = m;
	}
	else {
		*(coord->ordinates + 2) = m;
	}
}

void gcoord_set_ordinates(GCOORDINATE *coord, double *ordinates)
{
	assert(coord);
	assert(ordinates);
	coord->ordinates = ordinates;
}

void gcoord_set_ordinate(GCOORDINATE *coord, double ordinate, int index)
{
	assert(coord);
	assert(FLAGS_NDIMS(coord->flags)>index);
	assert(index>=0);
	*(coord->ordinates + index) = ordinate;
}

double gcoord_get_x(GCOORDINATE *coord)
{
	assert(coord);
	return *(coord->ordinates);
}

double gcoord_get_y(GCOORDINATE *coord)
{
	assert(coord);
	return *(coord->ordinates + 1);
}

double gcoord_get_z(GCOORDINATE *coord)
{
	assert(coord);
	assert(FLAGS_GET_Z(coord->flags));
	return *(coord->ordinates + 2);
}

double gcoord_get_m(GCOORDINATE *coord)
{
	assert(coord);
	assert(FLAGS_GET_M(coord->flags));
	if(FLAGS_GET_Z(coord->flags))
	{
		return *(coord->ordinates + 3);
	}
	else 
	{
		return *(coord->ordinates + 2);
	}
}
