/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 *
 * Copyright 2013 Kashif Rasul <kashif.rasul@gmail.com> and
 *                Shoaib Burq <saburq@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <string.h>
#include "liblwgeom_internal.h"

static char * lwline_to_encoded_polyline(const LWLINE*);
static char * lwmmpoint_to_encoded_polyline(const LWMPOINT*);
static char * pointarray_to_encoded_polyline(const POINTARRAY*);

/* takes a GEOMETRY and returns an Encoded Polyline representation */
extern char *
lwgeom_to_encoded_polyline(const LWGEOM *geom)
{
	int type = geom->type;
	switch (type)
	{
	case LINETYPE:
		return lwline_to_encoded_polyline((LWLINE*)geom);
	case MULTIPOINTTYPE:
		return lwmmpoint_to_encoded_polyline((LWMPOINT*)geom);
	default:
		lwerror("lwgeom_to_encoded_polyline: '%s' geometry type not supported", lwtype_name(type));
		return NULL;
	}
}

static 
char * lwline_to_encoded_polyline(const LWLINE *line)
{
	return pointarray_to_encoded_polyline(line->points);
}

static
char * lwmmpoint_to_encoded_polyline(const LWMPOINT *mpoint)
{
	LWLINE *line = lwline_from_lwmpoint(mpoint->srid, mpoint);
	char *encoded_polyline = lwline_to_encoded_polyline(line);

	lwline_free(line);
	return encoded_polyline;
}

static
char * pointarray_to_encoded_polyline(const POINTARRAY *pa)
{
	int i;
	const POINT2D *prevPoint;
	int *delta = malloc(2*sizeof(int)*pa->npoints);
	char *encoded_polyline;

	/* Take the double value and multiply it by 1e5, rounding the result */
	prevPoint = getPoint2d_cp(pa, 0);
	delta[0] = (int)(prevPoint->y*1e5);
	delta[1] = (int)(prevPoint->x*1e5);

	/*  points only include the offset from the previous point */
	for (i=1; i<pa->npoints; i++)
	{
		const POINT2D *point = getPoint2d_cp(pa, i);
		delta[2*i] = (int)(point->y*1e5) - (int)(prevPoint->y*1e5);
		delta[(2*i)+1] = (int)(point->x*1e5) - (int)(prevPoint->x*1e5);
		prevPoint = point;
	}
	
	/* value to binary: a negative value must be calculated using its two's complement */
	for (i=0; i<pa->npoints*2; i++)
	{
		/* Left-shift the binary value one bit */
		delta[i] <<= 1;
		/* if value is negative, invert this encoding */
		if (delta[i] < 0) {
			delta[i] = ~(delta[i]);
		}
	}

	for (i=0; i<pa->npoints*2; i++)
	{
		int numberToEncode = delta[i];

		while (numberToEncode >= 0x20) {
			/* Place the 5-bit chunks into reverse order or 
			 each value with 0x20 if another bit chunk follows and add 63*/
			int nextValue = (0x20 | (numberToEncode & 0x1f)) + 63;
			encoded_polyline += sprintf(encoded_polyline, "%c", nextValue);
			if(92 == nextValue)
				encoded_polyline += sprintf(encoded_polyline, "%c", nextValue);

			/* Break the binary value out into 5-bit chunks */
			numberToEncode >>= 5;
		}

		numberToEncode += 63;
		encoded_polyline += sprintf(encoded_polyline, "%c", numberToEncode);
		if(92 == numberToEncode)
			encoded_polyline += sprintf(encoded_polyline, "%c", numberToEncode);
	}

	free(delta);
	return encoded_polyline;
}
