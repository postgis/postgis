#include <stdio.h>
#include <string.h>

#include "lwgeom_pg.h"
#include "liblwgeom.h"

char *lwcollection_summary(LWCOLLECTION *collection, int offset);
char *lwpoly_summary(LWPOLY *poly, int offset);
char *lwline_summary(LWLINE *line, int offset);
char *lwpoint_summary(LWPOINT *point, int offset);

char *
lwgeom_summary(LWGEOM *lwgeom, int offset)
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
	result = lwalloc(128);
	char pad[offset+1];
	memset(pad, ' ', offset);
	pad[offset] = '\0';

	sprintf(result, "%s%s[%c%c%d]\n",
		pad, lwgeom_typename(point->type),
		point->hasbbox ? 'B' : '_',
		point->SRID != -1 ? 'S' : '_',
		point->ndims);
	return result;
}

char *
lwline_summary(LWLINE *line, int offset)
{
	char *result;
	result = lwalloc(128+offset);
	char pad[offset+1];
	memset(pad, ' ', offset);
	pad[offset] = '\0';

	sprintf(result, "%s%s[%c%c%d] with %d points\n",
		pad, lwgeom_typename(line->type),
		line->hasbbox ? 'B' : '_',
		line->SRID != -1 ? 'S' : '_',
		line->ndims,
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
	char pad[offset+1];
	memset(pad, ' ', offset);
	pad[offset] = '\0';

#ifdef DEBUG_CALLS
	lwnotice("lwcollection_summary called");
#endif

	result = (char *)lwalloc(size);

	sprintf(result, "%s%s[%c%c%d] with %d elements\n",
		pad, lwgeom_typename(col->type),
		col->hasbbox ? 'B' : '_',
		col->SRID != -1 ? 'S' : '_',
		col->ndims,
		col->ngeoms);

	for (i=0; i<col->ngeoms; i++)
	{
		tmp = lwgeom_summary(col->geoms[i], offset+2);
		size += strlen(tmp)+1;
		result = lwrealloc(result, size);
		//lwnotice("Reallocated %d bytes for result", size);
		strcat(result, tmp);
		lwfree(tmp);
	}

#ifdef DEBUG_CALLS
	lwnotice("lwcollection_summary returning");
#endif

	return result;
}

char *
lwpoly_summary(LWPOLY *poly, int offset)
{
	char tmp[256];
	size_t size = 64*(poly->nrings+1)+128;
	char *result;
	int i;
	char pad[offset+1];
	memset(pad, ' ', offset);
	pad[offset] = '\0';

#ifdef DEBUG_CALLS
	lwnotice("lwpoly_summary called");
#endif

	result = lwalloc(size);

	sprintf(result, "%s%s[%c%c%d] with %i rings\n",
		pad, lwgeom_typename(poly->type),
		poly->hasbbox ? 'B' : '_',
		poly->SRID != -1 ? 'S' : '_',
		poly->ndims,
		poly->nrings);

	for (i=0; i<poly->nrings;i++)
	{
		sprintf(tmp,"%s   ring %i has %i points\n",
			pad, i, poly->rings[i]->npoints);
		strcat(result,tmp);
	}

#ifdef DEBUG_CALLS
	lwnotice("lwpoly_summary returning");
#endif

	return result;
}

