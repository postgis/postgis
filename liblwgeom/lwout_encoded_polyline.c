/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2014 Kashif Rasul <kashif.rasul@gmail.com> and
 *                Shoaib Burq <saburq@gmail.com>
 *
 **********************************************************************/

#include "stringbuffer.h"
#include "liblwgeom_internal.h"

static lwvarlena_t *lwline_to_encoded_polyline(const LWLINE *, int precision);
static lwvarlena_t *lwmmpoint_to_encoded_polyline(const LWMPOINT *, int precision);
static lwvarlena_t *pointarray_to_encoded_polyline(const POINTARRAY *, int precision);

/* takes a GEOMETRY and returns an Encoded Polyline representation */
extern lwvarlena_t *
lwgeom_to_encoded_polyline(const LWGEOM *geom, int precision)
{
	int type = geom->type;
	switch (type)
	{
	case LINETYPE:
		return lwline_to_encoded_polyline((LWLINE*)geom, precision);
	case MULTIPOINTTYPE:
		return lwmmpoint_to_encoded_polyline((LWMPOINT*)geom, precision);
	default:
		lwerror("lwgeom_to_encoded_polyline: '%s' geometry type not supported",
				lwtype_name(type));
		return NULL;
	}
}

static lwvarlena_t *
lwline_to_encoded_polyline(const LWLINE *line, int precision)
{
	return pointarray_to_encoded_polyline(line->points, precision);
}

static lwvarlena_t *
lwmmpoint_to_encoded_polyline(const LWMPOINT *mpoint, int precision)
{
	LWLINE* line = lwline_from_lwmpoint(mpoint->srid, mpoint);
	lwvarlena_t *encoded_polyline = lwline_to_encoded_polyline(line, precision);

	lwline_free(line);
	return encoded_polyline;
}

static lwvarlena_t *
pointarray_to_encoded_polyline(const POINTARRAY *pa, int precision)
{
	uint32_t i;
	const POINT2D* prevPoint;
	int* delta;
	stringbuffer_t* sb;
	double scale = pow(10, precision);

	/* Empty input is empty string */
	if (pa->npoints == 0)
	{
		lwvarlena_t *v = lwalloc(LWVARHDRSZ);
		LWSIZE_SET(v->size, LWVARHDRSZ);
		return v;
	}

	delta = lwalloc(2 * sizeof(int) * pa->npoints);

	/* Take the double value and multiply it by 1x10^precision, rounding the
	 * result */
	prevPoint = getPoint2d_cp(pa, 0);
	delta[0] = round(prevPoint->y * scale);
	delta[1] = round(prevPoint->x * scale);

	/* Points only include the offset from the previous point */
	for (i = 1; i < pa->npoints; i++)
	{
		const POINT2D* point = getPoint2d_cp(pa, i);
		delta[2 * i] = round(point->y * scale) - round(prevPoint->y * scale);
		delta[(2 * i) + 1] =
			round(point->x * scale) - round(prevPoint->x * scale);
		prevPoint = point;
	}

	/* value to binary: a negative value must be calculated using its two's
	 * complement */
	for (i = 0; i < pa->npoints * 2; i++)
	{
		/* Multiply by 2 for a signed left shift */
		delta[i] *= 2;
		/* if value is negative, invert this encoding */
		if (delta[i] < 0) {
			delta[i] = ~(delta[i]);
		}
	}

	sb = stringbuffer_create();
	for (i = 0; i < pa->npoints * 2; i++)
	{
		int numberToEncode = delta[i];

		while (numberToEncode >= 0x20)
		{
			/* Place the 5-bit chunks into reverse order or
			 each value with 0x20 if another bit chunk follows and add 63*/
			int nextValue = (0x20 | (numberToEncode & 0x1f)) + 63;
			stringbuffer_aprintf(sb, "%c", (char)nextValue);

			/* Break the binary value out into 5-bit chunks */
			numberToEncode >>= 5;
		}

		numberToEncode += 63;
		stringbuffer_aprintf(sb, "%c", (char)numberToEncode);
	}

	lwfree(delta);
	lwvarlena_t *v = stringbuffer_getvarlenacopy(sb);
	stringbuffer_destroy(sb);

	return v;
}
