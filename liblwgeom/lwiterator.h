/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _LWITERATOR
#define _LWITERATOR 1

struct LISTNODE
{
	struct LISTNODE* next;
	const LWGEOM* geom;
};
typedef struct LISTNODE LISTNODE;

typedef struct
{
	LISTNODE* geoms;
	uint32_t ring;
	uint32_t i;
} LWITERATOR;

void lwiterator_create(const LWGEOM* g, LWITERATOR* s);
void lwiterator_destroy(LWITERATOR* s);
int lwiterator_has_next(LWITERATOR* s);
int lwiterator_next(LWITERATOR* s, POINT4D* p);
int lwiterator_peek(LWITERATOR* s, POINT4D* p);

#endif
