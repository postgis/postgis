#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

//#define PGIS_DEBUG_CALLS 1

#define CHECK_LWGEOM_ZM 1

LWCOLLECTION *
lwcollection_construct(unsigned int type, int SRID, BOX2DFLOAT4 *bbox,
	unsigned int ngeoms, LWGEOM **geoms)
{
	LWCOLLECTION *ret;
	int hasz, hasm;
#ifdef CHECK_LWGEOM_ZM
	char zm;
	unsigned int i;
#endif

	hasz = 0;
	hasm = 0;
	if ( ngeoms > 0 )
	{
		hasz = TYPE_HASZ(geoms[0]->type);
		hasm = TYPE_HASM(geoms[0]->type);
#ifdef CHECK_LWGEOM_ZM
		zm = TYPE_GETZM(geoms[0]->type);
		for (i=1; i<ngeoms; i++)
		{
			if ( zm != TYPE_GETZM(geoms[i]->type) )
				lwerror("lwcollection_construct: mixed dimension geometries");
		}
#endif
	}


	ret = lwalloc(sizeof(LWCOLLECTION));
	ret->type = lwgeom_makeType_full(hasz, hasm, (SRID!=-1),
		type, 0);
	ret->SRID = SRID;
	ret->ngeoms = ngeoms;
	ret->geoms = geoms;
	ret->bbox = bbox;

	return ret;
}

LWCOLLECTION *
lwcollection_construct_empty(int SRID, char hasz, char hasm)
{
	LWCOLLECTION *ret;

	ret = lwalloc(sizeof(LWCOLLECTION));
	ret->type = lwgeom_makeType_full(hasz, hasm, (SRID!=-1),
		COLLECTIONTYPE, 0);
	ret->SRID = SRID;
	ret->ngeoms = 0;
	ret->geoms = NULL;
	ret->bbox = NULL;

	return ret;
}


LWCOLLECTION *
lwcollection_deserialize(uchar *srl)
{
	LWCOLLECTION *result;
	LWGEOM_INSPECTED *insp;
	char typefl = srl[0];
	int type = lwgeom_getType(typefl);
	int i;

	if ( type != COLLECTIONTYPE ) 
	{
		lwerror("lwcollection_deserialize called on NON geometrycollection: %d", type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWCOLLECTION));
	result->type = typefl;
	result->SRID = insp->SRID;
	result->ngeoms = insp->ngeometries;
	result->geoms = lwalloc(sizeof(LWGEOM *)*insp->ngeometries);

	if (lwgeom_hasBBOX(srl[0]))
	{
		//result->bbox = (BOX2DFLOAT4 *)(srl+1);
		result->bbox = lwalloc(sizeof(BOX2DFLOAT4));
		memcpy(result->bbox, srl+1, sizeof(BOX2DFLOAT4));
	}
	else result->bbox = NULL;


	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwgeom_deserialize(insp->sub_geoms[i]);
	}

	return result;
}

LWGEOM *
lwcollection_getsubgeom(LWCOLLECTION *col, int gnum)
{
	return (LWGEOM *)col->geoms[gnum];
}

// find serialized size of this collection
size_t
lwcollection_serialize_size(LWCOLLECTION *col)
{
	size_t size = 5; // type + nsubgeoms
	int i;

	if ( col->SRID != -1 ) size += 4; // SRID
	if ( col->bbox ) size += sizeof(BOX2DFLOAT4);

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwcollection_serialize_size[%p]: start size: %d", col, size);
#endif


	for (i=0; i<col->ngeoms; i++)
	{
		size += lwgeom_serialize_size(col->geoms[i]);
#ifdef PGIS_DEBUG_CALLS
		lwnotice("lwcollection_serialize_size[%p]: with geom%d: %d", col, i, size);
#endif
	}

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwcollection_serialize_size[%p]:  returning %d", col, size);
#endif

	return size; 
}

// convert this collectoin into its serialize form writing it into
// the given buffer, and returning number of bytes written into
// the given int pointer.
void
lwcollection_serialize_buf(LWCOLLECTION *coll, uchar *buf, size_t *retsize)
{
	size_t size=1; // type 
	size_t subsize=0;
	char hasSRID;
	uchar *loc;
	int i;

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwcollection_serialize_buf called (%d with %d elems)",
		lwgeom_typename(TYPE_GETTYPE(coll->type)), coll->ngeoms);
#endif

	hasSRID = (coll->SRID != -1);

	buf[0] = lwgeom_makeType_full(
		TYPE_HASZ(coll->type), TYPE_HASM(coll->type),
		hasSRID, TYPE_GETTYPE(coll->type), coll->bbox ? 1 : 0);
	loc = buf+1;

	// Add BBOX if requested
	if ( coll->bbox )
	{
		memcpy(loc, coll->bbox, sizeof(BOX2DFLOAT4));
		size += sizeof(BOX2DFLOAT4);
		loc += sizeof(BOX2DFLOAT4);
	}

	// Add SRID if requested
	if (hasSRID)
	{
		memcpy(loc, &coll->SRID, 4);
		size += 4; 
		loc += 4;
	}

	// Write number of subgeoms
	memcpy(loc, &coll->ngeoms, 4);
	size += 4;
	loc += 4;

	// Serialize subgeoms
	for (i=0; i<coll->ngeoms; i++)
	{
		lwgeom_serialize_buf(coll->geoms[i], loc, &subsize);
		size += subsize;
		loc += subsize;
	}

	if (retsize) *retsize = size;

#ifdef PGIS_DEBUG_CALLS
	lwnotice("lwcollection_serialize_buf returning");
#endif
}

int
lwcollection_compute_box2d_p(LWCOLLECTION *col, BOX2DFLOAT4 *box)
{
	BOX2DFLOAT4 boxbuf;
	uint32 i;

	if ( ! col->ngeoms ) return 0;
	if ( ! lwgeom_compute_box2d_p(col->geoms[0], box) ) return 0;
	for (i=1; i<col->ngeoms; i++)
	{
		if ( ! lwgeom_compute_box2d_p(col->geoms[i], &boxbuf) )
			return 0;
		if ( ! box2d_union_p(box, &boxbuf, box) ) return 0;
	}
	return 1;
}

// Clone LWCOLLECTION object. POINTARRAY are not copied.
LWCOLLECTION *
lwcollection_clone(const LWCOLLECTION *g)
{
	uint32 i;
	LWCOLLECTION *ret = lwalloc(sizeof(LWCOLLECTION));
	memcpy(ret, g, sizeof(LWCOLLECTION));
	for (i=0; i<g->ngeoms; i++)
	{
		ret->geoms[i] = lwgeom_clone(g->geoms[i]);
	}
	if ( g->bbox ) ret->bbox = box2d_clone(g->bbox);
	return ret;
}

// Add 'what' to this collection at position 'where'.
// where=0 == prepend
// where=-1 == append
// Returns a GEOMETRYCOLLECTION
LWGEOM *
lwcollection_add(const LWCOLLECTION *to, uint32 where, const LWGEOM *what)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	uint32 i;

	if ( where == -1 ) where = to->ngeoms;
	else if ( where < -1 || where > to->ngeoms )
	{
		lwerror("lwcollection_add: add position out of range %d..%d",
			-1, to->ngeoms);
		return NULL;
	}

	// dimensions compatibility are checked by caller

	// Construct geoms array
	geoms = lwalloc(sizeof(LWGEOM *)*(to->ngeoms+1));
	for (i=0; i<where; i++)
	{
		geoms[i] = lwgeom_clone(to->geoms[i]);
		lwgeom_dropSRID(geoms[i]);
		lwgeom_dropBBOX(geoms[i]);
	}
	geoms[where] = lwgeom_clone(what);
	lwgeom_dropSRID(geoms[where]);
	lwgeom_dropBBOX(geoms[where]);
	for (i=where; i<to->ngeoms; i++)
	{
		geoms[i+1] = lwgeom_clone(to->geoms[i]);
		lwgeom_dropSRID(geoms[i+1]);
		lwgeom_dropBBOX(geoms[i+1]);
	}

	col = lwcollection_construct(COLLECTIONTYPE,
		to->SRID, NULL,
		to->ngeoms+1, geoms);
	
	return (LWGEOM *)col;

}

LWCOLLECTION *
lwcollection_segmentize2d(LWCOLLECTION *col, double dist)
{
	unsigned int i;
	LWGEOM **newgeoms;

	if ( ! col->ngeoms ) return col;

	newgeoms = lwalloc(sizeof(LWGEOM *)*col->ngeoms);
	for (i=0; i<col->ngeoms; i++)
		newgeoms[i] = lwgeom_segmentize2d(col->geoms[i], dist);

	return lwcollection_construct(col->type, col->SRID, col->bbox,
		col->ngeoms, newgeoms);
}

// check for same geometry composition
char
lwcollection_same(const LWCOLLECTION *c1, const LWCOLLECTION *c2)
{
	unsigned int i, j;
	unsigned int *hit;

#if PGIS_DEBUG_CALLS
	lwnotice("lwcollection_same called");
#endif // PGIS_DEBUG_CALLS

	if ( TYPE_GETTYPE(c1->type) != TYPE_GETTYPE(c2->type) ) return 0;
	if ( c1->ngeoms != c2->ngeoms ) return 0;

	hit = lwalloc(sizeof(unsigned int)*c1->ngeoms);
	memset(hit, 0, sizeof(unsigned int)*c1->ngeoms);

	for (i=0; i<c1->ngeoms; i++)
	{
		char found=0;
		for (j=0; j<c2->ngeoms; j++)
		{
			if ( hit[j] ) continue;
			if ( lwgeom_same(c1->geoms[i], c2->geoms[j]) )
			{
				hit[j] = 1;
				found=1;
				break;
			}
		}
		if ( ! found ) return 0;
	}
	return 1;
}
