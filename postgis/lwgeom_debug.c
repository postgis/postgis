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

/* Place to hold the ZM string used in other summaries */
static char tflags[4];

static char *
lwtype_zmflags(uint8_t flags)
{
	int flagno = 0;
	if ( FLAGS_GET_Z(flags) ) tflags[flagno++] = 'Z';
	if ( FLAGS_GET_M(flags) ) tflags[flagno++] = 'M';
	if ( FLAGS_GET_BBOX(flags) ) tflags[flagno++] = 'B';
	tflags[flagno] = '\0';

	POSTGIS_DEBUGF(4, "Flags: %s - returning %p", flags, tflags);

	return tflags;
}

/*
 * Returns an alloced string containing summary for the LWGEOM object
 */
static char *
lwpoint_summary(LWPOINT *point, int offset)
{
	char *result;
	char *pad="";
	char *zmflags = lwtype_zmflags(point->flags);

	result = palloc(128+offset);

	sprintf(result, "%*.s%s[%s]\n",
	        offset, pad, lwtype_name(point->type),
	        zmflags);
	return result;
}

static char *
lwline_summary(LWLINE *line, int offset)
{
	char *result;
	char *pad="";
	char *zmflags = lwtype_zmflags(line->flags);

	result = palloc(128+offset);

	sprintf(result, "%*.s%s[%s] with %d points\n",
	        offset, pad, lwtype_name(line->type),
	        zmflags,
	        line->points->npoints);
	return result;
}


static char *
lwcollection_summary(LWCOLLECTION *col, int offset)
{
	size_t size = 128;
	char *result;
	char *tmp;
	int i;
	char *pad="";
	char *zmflags = lwtype_zmflags(col->flags);

	POSTGIS_DEBUG(2, "lwcollection_summary called");

	result = (char *)palloc(size);

	sprintf(result, "%*.s%s[%s] with %d elements\n",
	        offset, pad, lwtype_name(col->type),
	        zmflags,
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

static char *
lwpoly_summary(LWPOLY *poly, int offset)
{
	char tmp[256];
	size_t size = 64*(poly->nrings+1)+128;
	char *result;
	int i;
	char *pad="";
	char *zmflags = lwtype_zmflags(poly->flags);

	POSTGIS_DEBUG(2, "lwpoly_summary called");

	result = palloc(size);

	sprintf(result, "%*.s%s[%s] with %i rings\n",
	        offset, pad, lwtype_name(poly->type),
	        zmflags,
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