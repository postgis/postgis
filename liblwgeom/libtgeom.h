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

/* We use same value and macro to flags than in libgeom.h
   and we add a solid flag for surface enclosing a solid
   (so still 2 unused bits) */
#define FLAGS_GET_SOLID(flags) (((flags) & 0x20)>>5)
#define FLAGS_SET_SOLID(flags, value) ((flags) = (value) ? ((flags) | 0x20) : ((flags) & 0xDF))


/**
* TSERIALIZED (mostly inspired by GSERIALIZED struct)
*/
typedef struct
{
        uint32 size; /* For PgSQL use only, use VAR* macros to manipulate. */
        uint32 srid; /* SRID */
        uchar flags; /* HasZ, HasM, HasBBox, IsGeodetic, IsReadOnly, IsSolid */
        uchar *data; /* See tserialized.txt */
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
	int nedges;
	int maxedges;
	int *edges;		/* Array of edge index, a negative value
				   means that the edge is reversed */
	int nrings;
	POINTARRAY **rings;	/* Internal rings array */
} TFACE;

typedef struct
{
	uchar type;		
	uchar flags;		
	uint32 srid;		/* 0 == unknown */
	BOX3D *bbox;		/* NULL == unneeded */
	int nedges;
	int maxedges;
	TEDGE **edges;
	int nfaces;
	int maxfaces;
	TFACE **faces;
} TGEOM;

extern TGEOM* tgeom_new(uint32 type, int hasz, int hasm);
extern void tgeom_free(TGEOM *tgeom);
extern TSERIALIZED* tserialized_from_lwgeom(LWGEOM *lwgeom);
extern TGEOM* tgeom_from_lwgeom(LWGEOM *lwgeom);
extern LWGEOM* lwgeom_from_tserialized(TSERIALIZED *t);
extern LWGEOM* lwgeom_from_tgeom(TGEOM *tgeom);
extern int lwgeom_is_solid(LWGEOM *lwgeom);
TSERIALIZED * tgeom_serialize(const TGEOM *tgeom);
TGEOM * tgeom_deserialize(TSERIALIZED *serialized_form);
double tgeom_perimeter2d(TGEOM* tgeom);
double tgeom_perimeter(TGEOM* tgeom);
extern void printTGEOM(TGEOM *tgeom);
