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

G_LINESTRING* glinestring_new_from_gptarray(GPTARRAY *ptarray)
{
	G_LINESTRING *gline = NULL;
	assert(ptarray);
	gline = (G_LINESTRING*)lwalloc(sizeof(G_LINESTRING));
	gline->flags = ptarray->flags;
	gline->type = LINETYPE;
	gline->bbox = NULL;
	gline->srid = 0;
	gline->points = ptarray;
	return gline;
}

G_LINESTRING* glinestring_new(uchar flags)
{
	G_LINESTRING *gline = NULL;
	gline = (G_LINESTRING*)lwalloc(sizeof(G_LINESTRING));
	gline->flags = flags;
	gline->type = LINETYPE;
	gline->bbox = NULL;
	gline->srid = 0;
	gline->points = gptarray_new(gline->flags);
	return gline;
}
