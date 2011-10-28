/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2004 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwgeom_pg.h"
#include "liblwgeom.h"

#include <stdio.h>
#include <string.h>


char *lwcollection_summary(LWCOLLECTION *collection, int offset);
char *lwpoly_summary(LWPOLY *poly, int offset);
char *lwline_summary(LWLINE *line, int offset);
char *lwpoint_summary(LWPOINT *point, int offset);

char *
lwgeom_summary(const LWGEOM *lwgeom, int offset)
{
	char *result;

	switch (lwgeom->type)
	{
	case POINTTYPE:
		return lwpoint_summary((LWPOINT *)lwgeom, offset);

	case LINETYPE:
		return lwline_summary((LWLINE *)lwgeom, offset);

	case POLYGONTYPE:
		return lwpoly_summary((LWPOLY *)lwgeom, offset);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return lwcollection_summary((LWCOLLECTION *)lwgeom, offset);
	default:
		result = palloc(256);
		sprintf(result, "Object is of unknown type: %d",
		        lwgeom->type);
		return result;
	}

	return NULL;
}

/*
 * Returns an alloced string containing summary for the LWGEOM object
 */
char *
lwpoint_summary(LWPOINT *point, int offset)
{
	char *result;
	char *pad="";

	result = lwalloc(128+offset);

	sprintf(result, "%*.s%s[%s]\n",
	        offset, pad, lwtype_name(point->type),
	        lwtype_zmflags(point->flags));
	return result;
}

char *
lwline_summary(LWLINE *line, int offset)
{
	char *result;
	char *pad="";

	result = lwalloc(128+offset);

	sprintf(result, "%*.s%s[%s] with %d points\n",
	        offset, pad, lwtype_name(line->type),
	        lwtype_zmflags(line->flags),
	        line->points->npoints);
	return result;
}


char *
lwcollection_summary(LWCOLLECTION *col, int offset)
{
	size_t size = 128;
	char *result;
	char *tmp;
	int i;
	char *pad="";

	POSTGIS_DEBUG(2, "lwcollection_summary called");

	result = (char *)lwalloc(size);

	sprintf(result, "%*.s%s[%s] with %d elements\n",
	        offset, pad, lwtype_name(col->type),
	        lwtype_zmflags(col->flags),
	        col->ngeoms);

	for (i=0; i<col->ngeoms; i++)
	{
		tmp = lwgeom_summary(col->geoms[i], offset+2);
		size += strlen(tmp)+1;
		result = lwrealloc(result, size);

		POSTGIS_DEBUGF(4, "Reallocated %d bytes for result", size);

		strcat(result, tmp);
		lwfree(tmp);
	}

	POSTGIS_DEBUG(3, "lwcollection_summary returning");

	return result;
}

char *
lwpoly_summary(LWPOLY *poly, int offset)
{
	char tmp[256];
	size_t size = 64*(poly->nrings+1)+128;
	char *result;
	int i;
	char *pad="";

	POSTGIS_DEBUG(2, "lwpoly_summary called");

	result = lwalloc(size);

	sprintf(result, "%*.s%s[%s] with %i rings\n",
	        offset, pad, lwtype_name(poly->type),
	        lwtype_zmflags(poly->flags),
	        poly->nrings);

	for (i=0; i<poly->nrings; i++)
	{
		sprintf(tmp,"%s   ring %i has %i points\n",
		        pad, i, poly->rings[i]->npoints);
		strcat(result,tmp);
	}

	POSTGIS_DEBUG(3, "lwpoly_summary returning");

	return result;
}

