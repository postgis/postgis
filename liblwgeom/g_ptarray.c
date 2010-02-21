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

GPTARRAY* gptarray_new(uchar flags)
{
	GPTARRAY *ptarr = (GPTARRAY*)lwalloc(sizeof(GPTARRAY));
	ptarr->flags = flags;
	ptarr->capacity = G_PT_ARRAY_DEFAULT_POINTS * FLAGS_NDIMS(flags) * sizeof(double);
	ptarr->npoints = 0;
	ptarr->ordinates = (double*)lwalloc(ptarr->capacity);
	return ptarr;
}

GPTARRAY* gptarray_copy(GPTARRAY *ptarray)
{
	GPTARRAY *copy = NULL;
	assert(ptarray);
	copy = (GPTARRAY*)lwalloc(sizeof(GPTARRAY));
	copy->flags = ptarray->flags;
	copy->capacity = ptarray->npoints * FLAGS_NDIMS(ptarray->flags) * sizeof(double);
	copy->npoints = ptarray->npoints;
	copy->ordinates = (double*)lwalloc(copy->capacity);
	if ( ! copy->ordinates ) return NULL;
	memcpy(copy->ordinates, ptarray->ordinates, copy->capacity);
	return copy;
}

GPTARRAY* gptarray_new_with_size(uchar flags, int npoints)
{
	GPTARRAY *ptarray = (GPTARRAY*)lwalloc(sizeof(GPTARRAY));
	ptarray->flags = flags;
	ptarray->capacity = FLAGS_NDIMS(flags) * npoints * sizeof(double);
	ptarray->npoints = npoints;
	ptarray->ordinates = (double*)lwalloc(ptarray->capacity);
	return ptarray;
}

GPTARRAY* gptarray_new_with_ordinates(uchar flags, int npoints, double *ordinates)
{
	GPTARRAY *ptarray = (GPTARRAY*)lwalloc(sizeof(GPTARRAY));
	assert(ordinates);
	ptarray->flags = flags;
	ptarray->capacity = 0; /* We aren't managing this memory, watch out. */
	ptarray->npoints = npoints;
	ptarray->ordinates = ordinates;
	return ptarray;
}

void gptarray_free(GPTARRAY *ptarray)
{
	assert(ptarray);
	assert(ptarray->ordinates);
	if ( ptarray->capacity > 0 ) /* Only free the ordinates if we are managing them. */
		lwfree(ptarray->ordinates);
	lwfree(ptarray);
}

void gptarray_add_coord(GPTARRAY *ptarray, GCOORDINATE *coord)
{
	assert(ptarray);
	assert(ptarray->flags == coord->flags);
	if ( FLAGS_NDIMS(ptarray->flags) * (ptarray->npoints + 1) * sizeof(double) > ptarray->capacity )
	{
		ptarray->capacity *= 2;
		ptarray->ordinates = lwrealloc(ptarray->ordinates, ptarray->capacity);
		if ( ! ptarray->ordinates )
		{
			lwerror("Out of memory!");
			return;
		}
	}
	memcpy(ptarray->ordinates + FLAGS_NDIMS(ptarray->flags) * ptarray->npoints * sizeof(double),
	       coord->ordinates,
	       FLAGS_NDIMS(coord->flags) * sizeof(double));

	ptarray->npoints++;
}

GCOORDINATE* gptarray_get_coord_ro(GPTARRAY *ptarray, int i)
{
	GCOORDINATE *coord;
	assert(ptarray);
	coord = gcoord_new(ptarray->flags);

	coord->ordinates = ptarray->ordinates + FLAGS_NDIMS(ptarray->flags) * i;

	return coord;
}

GCOORDINATE* gptarray_get_coord_new(GPTARRAY *ptarray, int i)
{
	GCOORDINATE *coord;
	assert(ptarray);
	coord = gcoord_new(ptarray->flags);

	memcpy(coord->ordinates,
	       ptarray->ordinates + FLAGS_NDIMS(ptarray->flags) * i,
	       FLAGS_NDIMS(ptarray->flags) * sizeof(double));

	return coord;
}


void gptarray_set_coord(GPTARRAY *ptarray, int i, GCOORDINATE *coord)
{
	int dim = 0;
	int ndims = 0;

	assert(ptarray);
	assert(coord->flags == ptarray->flags);

	ndims = FLAGS_NDIMS(ptarray->flags);

	for ( dim = 0; dim < ndims; dim++ )
	{
		*(ptarray->ordinates + i * ndims + dim) = *(coord->ordinates + dim);
	}

}

void gptarray_set_x(GPTARRAY *ptarray, int i, double x)
{
	assert(ptarray);

	*(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 0) = x;
}

void gptarray_set_y(GPTARRAY *ptarray, int i, double y)
{
	assert(ptarray);

	*(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 1) = y;
}

void gptarray_set_z(GPTARRAY *ptarray, int i, double z)
{
	assert(ptarray);
	assert(FLAGS_GET_Z(ptarray->flags));

	*(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 2) = z;
}

void gptarray_set_m(GPTARRAY *ptarray, int i, double m)
{
	assert(ptarray);
	assert(FLAGS_GET_M(ptarray->flags));

	if ( FLAGS_GET_Z(ptarray->flags))
	{
		/* Four coordinates */
		*(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 3) = m;
	}
	else
	{
		/* Three coordinates */
		*(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 2) = m;
	}
}

double gptarray_get_x(GPTARRAY *ptarray, int i)
{
	assert(ptarray);
	return *(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 0);
}

double gptarray_get_y(GPTARRAY *ptarray, int i)
{
	assert(ptarray);
	return *(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 1);
}

double gptarray_get_z(GPTARRAY *ptarray, int i)
{
	assert(ptarray);
	assert(FLAGS_GET_Z(ptarray->flags));
	return *(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 2);
}

double gptarray_get_m(GPTARRAY *ptarray, int i)
{
	assert(ptarray);
	assert(FLAGS_GET_M(ptarray->flags));

	if ( FLAGS_GET_Z(ptarray->flags))
	{
		/* Four coordinates */
		return *(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 3);
	}
	else
	{
		/* Three coordinates */
		return *(ptarray->ordinates + i * FLAGS_NDIMS(ptarray->flags) + 2);
	}
}

