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
			size = lwmpoint_serialize_size((LWMPOINT *)lwgeom);
		case MULTILINETYPE:
			size = lwmline_serialize_size((LWMLINE *)lwgeom);
		case MULTIPOLYGONTYPE:
			size = lwmpoly_serialize_size((LWMPOLY *)lwgeom);
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
	int size;

	switch (type)
	{
		case POINTTYPE:
			lwpoint_serialize_buf((LWPOINT *)lwgeom, buf, &size);
			break;
		case LINETYPE:
			lwline_serialize_buf((LWLINE *)lwgeom, buf, &size);
			break;
		case POLYGONTYPE:
			lwpoly_serialize_buf((LWPOLY *)lwgeom, buf, &size);
			break;
		case MULTIPOINTTYPE:
			lwmpoint_serialize_buf((LWMPOINT *)lwgeom, buf, &size);
			break;
		case MULTILINETYPE:
			lwmline_serialize_buf((LWMLINE *)lwgeom, buf, &size);
			break;
		case MULTIPOLYGONTYPE:
			lwmpoly_serialize_buf((LWMPOLY *)lwgeom, buf, &size);
			break;
		case COLLECTIONTYPE:
			lwcollection_serialize_buf((LWCOLLECTION *)lwgeom, buf, &size);
			break;
		default:
			lwerror("Unknown geometry type: %d", type);
			return;
	}
	*retsize = size;
	return;
}

char *
lwgeom_serialize(LWGEOM *lwgeom, char wantbbox)
{
	size_t size = lwgeom_serialize_size(lwgeom);
	size_t retsize;
	char *loc;

	if ( wantbbox ) size += sizeof(BOX2DFLOAT4);

	char *serialized = lwalloc(size);

	if ( wantbbox ) loc = serialized + sizeof(BOX2DFLOAT4);
	else loc = serialized;

	lwgeom_serialize_buf(lwgeom, loc, &retsize);

#ifdef DEBUG
	if ( wantbbox ) retsize += 4;
	if ( retsize != size )
	{
		lwerror("lwgeom_serialize: computed size %d, returned size %d",
			size, retsize);
	}
#endif

	return serialized;
}

