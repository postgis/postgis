#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lwgeom_pg.h"
#include "liblwgeom.h"

LWGEOM *
lwgeom_deserialize(char *srl)
{
	int type = lwgeom_getType(srl[0]);

	switch (type)
	{
		case POINTTYPE:
			return (LWGEOM *)lwpoint_deserialize(srl);
		case LINETYPE:
			return (LWGEOM *)lwline_deserialize(srl);
		case POLYGONTYPE:
			return (LWGEOM *)lwpoly_deserialize(srl);
		case MULTIPOINTTYPE:
			return (LWGEOM *)lwmpoint_deserialize(srl);
		case MULTILINETYPE:
			return (LWGEOM *)lwmline_deserialize(srl);
		case MULTIPOLYGONTYPE:
			return (LWGEOM *)lwmpoly_deserialize(srl);
		case COLLECTIONTYPE:
			return (LWGEOM *)lwcollection_deserialize(srl);
		default:
			lwerror("Unknown geometry type: %d", type);
			return NULL;
	}

}

size_t
lwgeom_serialize_size(LWGEOM *lwgeom)
{
	int type = lwgeom->type;
	size_t size;

	switch (type)
	{
		case POINTTYPE:
			size = lwpoint_serialize_size((LWPOINT *)lwgeom);
		case LINETYPE:
			size = lwline_serialize_size((LWLINE *)lwgeom);
		case POLYGONTYPE:
			size = lwpoly_serialize_size((LWPOLY *)lwgeom);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			size = lwcollection_serialize_size((LWCOLLECTION *)lwgeom);
		default:
			lwerror("Unknown geometry type: %d", type);
			return 0;
	}

	return size;
}

void
lwgeom_serialize_buf(LWGEOM *lwgeom, char *buf, int *retsize)
{
	int type = lwgeom->type;

	switch (type)
	{
		case POINTTYPE:
			lwpoint_serialize_buf((LWPOINT *)lwgeom, buf, retsize);
			break;
		case LINETYPE:
			lwline_serialize_buf((LWLINE *)lwgeom, buf, retsize);
			break;
		case POLYGONTYPE:
			lwpoly_serialize_buf((LWPOLY *)lwgeom, buf, retsize);
			break;
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			lwcollection_serialize_buf((LWCOLLECTION *)lwgeom, buf,
				retsize);
			break;
		default:
			lwerror("Unknown geometry type: %d", type);
			return;
	}
	return;
}

char *
lwgeom_serialize(LWGEOM *lwgeom)
{
	size_t size = lwgeom_serialize_size(lwgeom);
	size_t retsize;
	char *serialized = lwalloc(size);

	lwgeom_serialize_buf(lwgeom, serialized, &retsize);

#ifdef DEBUG
	if ( retsize != size )
	{
		lwerror("lwgeom_serialize: computed size %d, returned size %d",
			size, retsize);
	}
#endif

	return serialized;
}

// Force Right-hand-rule on LWGEOM polygons
void
lwgeom_forceRHR(LWGEOM *lwgeom)
{
	LWCOLLECTION *coll;
	int i;

	switch (lwgeom->type)
	{
		case POLYGONTYPE:
			lwpoly_reverse((LWPOLY *)lwgeom);
			return;

		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			coll = (LWCOLLECTION *)lwgeom;
			for (i=0; i<coll->ngeoms; i++)
				lwgeom_forceRHR(coll->geoms[i]);
			return;
	}
}

// Reverse vertex order of LWGEOM
void
lwgeom_reverse(LWGEOM *lwgeom)
{
	int i;
	LWCOLLECTION *col;

	switch (lwgeom->type)
	{
		case LINETYPE:
			lwline_reverse((LWLINE *)lwgeom);
			return;
		case POLYGONTYPE:
			lwpoly_reverse((LWPOLY *)lwgeom);
			return;
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			col = (LWCOLLECTION *)lwgeom;
			for (i=0; i<col->ngeoms; i++)
				lwgeom_reverse(col->geoms[i]);
			return;
	}
}

int
lwgeom_compute_bbox_p(LWGEOM *lwgeom, BOX2DFLOAT4 *buf)
{
	switch(lwgeom->type)
	{
		case POINTTYPE:
			return lwpoint_compute_bbox_p((LWPOINT *)lwgeom, buf);
		case LINETYPE:
			return lwline_compute_bbox_p((LWLINE *)lwgeom, buf);
		case POLYGONTYPE:
			return lwpoly_compute_bbox_p((LWPOLY *)lwgeom, buf);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_compute_bbox_p((LWCOLLECTION *)lwgeom, buf);
	}
	return 0;
}

//dont forget to lwfree() result
BOX2DFLOAT4 *
lwgeom_compute_bbox(LWGEOM *lwgeom)
{
	BOX2DFLOAT4 *result = lwalloc(sizeof(BOX2DFLOAT4));
	if ( lwgeom_compute_bbox_p(lwgeom, result) ) return result;
	else return NULL;
}
