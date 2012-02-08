/**********************************************************************
 * $Id:$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _LIBTGEOM_H
#define _LIBTGEOM_H 1

#include <stdint.h>

/**
* TSERIALIZED (mostly inspired by GSERIALIZED struct)
*/
typedef struct
{
        uint32_t size; /* For PgSQL use only, use VAR* macros to manipulate. */
        uint32_t srid; /* SRID */
        uint8_t flags; /* HasZ, HasM, HasBBox, IsGeodetic, IsReadOnly, IsSolid */
        uint8_t *data; /* See tserialized.txt */
} TSERIALIZED;



typedef struct 
{
	POINT4D *s;		/* Edge starting point */
	POINT4D *e;		/* Edge ending point */
	int count;		/* Count how many time this edge is used in the TGEOM.
				   Caution: We don't care about edge orientation ! */
} TEDGE;

typedef struct
{
	uint32_t nedges;
	uint32_t maxedges;
	int32_t *edges;		/* Array of edge index, a negative value
				   means that the edge is reversed */
	int32_t nrings;
	POINTARRAY **rings;	/* Internal rings array */
} TFACE;

typedef struct
{
	uint8_t type;		
	uint8_t flags;		
	uint32_t srid;		/* 0 == unknown */
	BOX3D *bbox;		/* NULL == unneeded */
	uint32_t nedges;
	uint32_t maxedges;
	TEDGE **edges;
	uint32_t nfaces;
	uint32_t maxfaces;
	TFACE **faces;
} TGEOM;

extern TGEOM* tgeom_new(uint8_t type, int hasz, int hasm);
extern void tgeom_free(TGEOM *tgeom);
extern TSERIALIZED* tserialized_from_lwgeom(LWGEOM *lwgeom);
extern TGEOM* tgeom_from_lwgeom(const LWGEOM *lwgeom);
extern LWGEOM* lwgeom_from_tserialized(TSERIALIZED *t);
extern LWGEOM* lwgeom_from_tgeom(TGEOM *tgeom);
extern int lwgeom_is_solid(LWGEOM *lwgeom);
TSERIALIZED * tgeom_serialize(const TGEOM *tgeom);
TGEOM * tgeom_deserialize(TSERIALIZED *serialized_form);
double tgeom_perimeter2d(TGEOM* tgeom);
double tgeom_perimeter(TGEOM* tgeom);
extern void printTGEOM(TGEOM *tgeom);

#endif /* !defined _LIBTGEOM_H */
