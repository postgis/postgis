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
 * Copyright 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"

void
lwmline_release(LWMLINE *lwmline)
{
	lwgeom_release(lwmline_as_lwgeom(lwmline));
}

LWMLINE *
lwmline_construct_empty(int32_t srid, char hasz, char hasm)
{
	LWMLINE *ret = (LWMLINE*)lwcollection_construct_empty(MULTILINETYPE, srid, hasz, hasm);
	return ret;
}



LWMLINE* lwmline_add_lwline(LWMLINE *mobj, const LWLINE *obj)
{
	return (LWMLINE*)lwcollection_add_lwgeom((LWCOLLECTION*)mobj, (LWGEOM*)obj);
}

/**
* Re-write the measure ordinate (or add one, if it isn't already there) interpolating
* the measure between the supplied start and end values.
*/
LWMLINE*
lwmline_measured_from_lwmline(const LWMLINE *lwmline, double m_start, double m_end)
{
	uint32_t i = 0;
	int hasm = 0, hasz = 0;
	double length = 0.0, length_so_far = 0.0;
	double m_range = m_end - m_start;
	LWGEOM **geoms = NULL;

	if ( lwmline->type != MULTILINETYPE )
	{
		lwerror("lwmline_measured_from_lmwline: only multiline types supported");
		return NULL;
	}

	hasz = FLAGS_GET_Z(lwmline->flags);
	hasm = 1;

	/* Calculate the total length of the mline */
	for ( i = 0; i < lwmline->ngeoms; i++ )
	{
		LWLINE *lwline = (LWLINE*)lwmline->geoms[i];
		if ( lwline->points && lwline->points->npoints > 1 )
		{
			length += ptarray_length_2d(lwline->points);
		}
	}

	if ( lwgeom_is_empty((LWGEOM*)lwmline) )
	{
		return (LWMLINE*)lwcollection_construct_empty(MULTILINETYPE, lwmline->srid, hasz, hasm);
	}

	geoms = lwalloc(sizeof(LWGEOM*) * lwmline->ngeoms);

	for ( i = 0; i < lwmline->ngeoms; i++ )
	{
		double sub_m_start, sub_m_end;
		double sub_length = 0.0;
		LWLINE *lwline = (LWLINE*)lwmline->geoms[i];

		if ( lwline->points && lwline->points->npoints > 1 )
		{
			sub_length = ptarray_length_2d(lwline->points);
		}

		sub_m_start = (m_start + m_range * length_so_far / length);
		sub_m_end = (m_start + m_range * (length_so_far + sub_length) / length);

		geoms[i] = (LWGEOM*)lwline_measured_from_lwline(lwline, sub_m_start, sub_m_end);

		length_so_far += sub_length;
	}

	return (LWMLINE*)lwcollection_construct(lwmline->type, lwmline->srid, NULL, lwmline->ngeoms, geoms);
}

void lwmline_free(LWMLINE *mline)
{
	if (!mline)
		return;

	if (mline->bbox)
		lwfree(mline->bbox);

	if (mline->geoms)
	{
		for (uint32_t i = 0; i < mline->ngeoms; i++)
			if (mline->geoms[i])
				lwline_free(mline->geoms[i]);
		lwfree(mline->geoms);
	}

	lwfree(mline);
}
