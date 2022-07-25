#ifndef _LWGEOM_UNION_H
#define _LWGEOM_UNION_H 1

#include "postgres.h"
#include "liblwgeom.h"

typedef struct UnionState
{
	float8 gridSize; /* gridSize argument */
	List *list; /* list of GSERIALIZED* */
	int32 size; /* total size of GSERIAZLIZED values in list in bytes */
} UnionState;

#endif /* _LWGEOM_UNION_H */
