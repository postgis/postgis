/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"



int
lwcompound_is_closed(const LWCOMPOUND *compound)
{
	size_t size;
	int npoints=0;

	if (!FLAGS_GET_Z(compound->flags))
		size = sizeof(POINT2D);
	else    size = sizeof(POINT3D);

	if      (compound->geoms[compound->ngeoms - 1]->type == CIRCSTRINGTYPE)
		npoints = ((LWCIRCSTRING *)compound->geoms[compound->ngeoms - 1])->points->npoints;
	else if (compound->geoms[compound->ngeoms - 1]->type == LINETYPE)
		npoints = ((LWLINE *)compound->geoms[compound->ngeoms - 1])->points->npoints;

	if ( memcmp(getPoint_internal( (POINTARRAY *)compound->geoms[0]->data, 0),
	            getPoint_internal( (POINTARRAY *)compound->geoms[compound->ngeoms - 1]->data,
	                               npoints - 1),
	            size) ) return LW_FALSE;

	return LW_TRUE;
}

double lwcompound_length(const LWCOMPOUND *comp)
{
	double length = 0.0;
	LWLINE *line;
	if ( lwgeom_is_empty((LWGEOM*)comp) )
		return 0.0;
	line = lwcompound_segmentize(comp, 32);
	length = lwline_length(line);
	lwline_free(line);
	return length;
}

double lwcompound_length_2d(const LWCOMPOUND *comp)
{
	double length = 0.0;
	LWLINE *line;
	if ( lwgeom_is_empty((LWGEOM*)comp) )
		return 0.0;
	line = lwcompound_segmentize(comp, 32);
	length = lwline_length_2d(line);
	lwline_free(line);
	return length;
}

int lwcompound_add_lwgeom(LWCOMPOUND *comp, LWGEOM *geom)
{
	LWCOLLECTION *col = (LWCOLLECTION*)comp;
	
	/* Empty things can't continuously join up with other things */
	if ( lwgeom_is_empty(geom) )
	{
		LWDEBUG(4, "Got an empty component for a compound curve!");
		return LW_FAILURE;
	}
	
	if( col->ngeoms > 0 )
	{
		POINT4D last, first;
		/* First point of the component we are adding */
		LWLINE *newline = (LWLINE*)geom;
		/* Last point of the previous component */
		LWLINE *prevline = (LWLINE*)(col->geoms[col->ngeoms-1]);

		getPoint4d_p(newline->points, 0, &first);
		getPoint4d_p(prevline->points, prevline->points->npoints-1, &last);
		
		if ( !(FP_EQUALS(first.x,last.x) && FP_EQUALS(first.y,last.y)) )
		{
			LWDEBUG(4, "Components don't join up end-to-end!");
			LWDEBUGF(4, "first pt (%g %g %g %g) last pt (%g %g %g %g)", first.x, first.y, first.z, first.m, last.x, last.y, last.z, last.m);			
			return LW_FAILURE;
		}
	}
	
	col = lwcollection_add_lwgeom(col, geom);
	return LW_SUCCESS;
}

LWCOMPOUND *
lwcompound_construct_empty(int srid, char hasz, char hasm)
{
        LWCOMPOUND *ret = (LWCOMPOUND*)lwcollection_construct_empty(COMPOUNDTYPE, srid, hasz, hasm);
        return ret;
}

