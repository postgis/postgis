/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"


#define CHECK_LWGEOM_ZM 1

void
lwcollection_release(LWCOLLECTION *lwcollection)
{
	lwgeom_release(lwcollection_as_lwgeom(lwcollection));
}


LWCOLLECTION *
lwcollection_construct(uint8_t type, int srid, GBOX *bbox,
                       uint32_t ngeoms, LWGEOM **geoms)
{
	LWCOLLECTION *ret;
	int hasz, hasm;
#ifdef CHECK_LWGEOM_ZM
	char zm;
	uint32_t i;
#endif

	LWDEBUGF(2, "lwcollection_construct called with %d, %d, %p, %d, %p.", type, srid, bbox, ngeoms, geoms);

	if( ! lwtype_is_collection(type) )
		lwerror("Non-collection type specified in collection constructor!");

	hasz = 0;
	hasm = 0;
	if ( ngeoms > 0 )
	{
		hasz = FLAGS_GET_Z(geoms[0]->flags);
		hasm = FLAGS_GET_M(geoms[0]->flags);
#ifdef CHECK_LWGEOM_ZM
		zm = FLAGS_GET_ZM(geoms[0]->flags);

		LWDEBUGF(3, "lwcollection_construct type[0]=%d", geoms[0]->type);

		for (i=1; i<ngeoms; i++)
		{
			LWDEBUGF(3, "lwcollection_construct type=[%d]=%d", i, geoms[i]->type);

			if ( zm != FLAGS_GET_ZM(geoms[i]->flags) )
				lwerror("lwcollection_construct: mixed dimension geometries: %d/%d", zm, FLAGS_GET_ZM(geoms[i]->flags));
		}
#endif
	}


	ret = lwalloc(sizeof(LWCOLLECTION));
	ret->type = type;
	ret->flags = gflags(hasz,hasm,0);
	FLAGS_SET_BBOX(ret->flags, bbox?1:0);
	ret->srid = srid;
	ret->ngeoms = ngeoms;
	ret->maxgeoms = ngeoms;
	ret->geoms = geoms;
	ret->bbox = bbox;

	return ret;
}

LWCOLLECTION *
lwcollection_construct_empty(uint8_t type, int srid, char hasz, char hasm)
{
	LWCOLLECTION *ret;
	if( ! lwtype_is_collection(type) )
		lwerror("Non-collection type specified in collection constructor!");

	ret = lwalloc(sizeof(LWCOLLECTION));
	ret->type = type;
	ret->flags = gflags(hasz,hasm,0);
	ret->srid = srid;
	ret->ngeoms = 0;
	ret->maxgeoms = 1; /* Allocate room for sub-members, just in case. */
	ret->geoms = lwalloc(ret->maxgeoms * sizeof(LWGEOM*));
	ret->bbox = NULL;

	return ret;
}

LWGEOM *
lwcollection_getsubgeom(LWCOLLECTION *col, int gnum)
{
	return (LWGEOM *)col->geoms[gnum];
}

/**
 * @brief Clone #LWCOLLECTION object. #POINTARRAY are not copied.
 * 			Bbox is cloned if present in input.
 */
LWCOLLECTION *
lwcollection_clone(const LWCOLLECTION *g)
{
	uint32_t i;
	LWCOLLECTION *ret = lwalloc(sizeof(LWCOLLECTION));
	memcpy(ret, g, sizeof(LWCOLLECTION));
	if ( g->ngeoms > 0 )
	{
		ret->geoms = lwalloc(sizeof(LWGEOM *)*g->ngeoms);
		for (i=0; i<g->ngeoms; i++)
		{
			ret->geoms[i] = lwgeom_clone(g->geoms[i]);
		}
		if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	}
	else
	{
		ret->bbox = NULL; /* empty collection */
		ret->geoms = NULL;
	}
	return ret;
}

/**
* @brief Deep clone #LWCOLLECTION object. #POINTARRAY are copied.
*/
LWCOLLECTION *
lwcollection_clone_deep(const LWCOLLECTION *g)
{
	uint32_t i;
	LWCOLLECTION *ret = lwalloc(sizeof(LWCOLLECTION));
	memcpy(ret, g, sizeof(LWCOLLECTION));
	if ( g->ngeoms > 0 )
	{
		ret->geoms = lwalloc(sizeof(LWGEOM *)*g->ngeoms);
		for (i=0; i<g->ngeoms; i++)
		{
			ret->geoms[i] = lwgeom_clone_deep(g->geoms[i]);
		}
		if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	}
	else
	{
		ret->bbox = NULL; /* empty collection */
		ret->geoms = NULL;
	}
	return ret;
}

/**
 * Ensure the collection can hold up at least ngeoms
 */
void lwcollection_reserve(LWCOLLECTION *col, uint32_t ngeoms)
{
	if ( ngeoms <= col->maxgeoms ) return;

	/* Allocate more space if we need it */
	do { col->maxgeoms *= 2; } while ( col->maxgeoms < ngeoms );
	col->geoms = lwrealloc(col->geoms, sizeof(LWGEOM*) * col->maxgeoms);
}

/**
* Appends geom to the collection managed by col. Does not copy or
* clone, simply takes a reference on the passed geom.
*/
LWCOLLECTION* lwcollection_add_lwgeom(LWCOLLECTION *col, const LWGEOM *geom)
{
	if (!col || !geom) return NULL;

	if (!col->geoms && (col->ngeoms || col->maxgeoms))
	{
		lwerror("Collection is in inconsistent state. Null memory but non-zero collection counts.");
		return NULL;
	}

	/* Check type compatibility */
	if ( ! lwcollection_allows_subtype(col->type, geom->type) ) {
		lwerror("%s cannot contain %s element", lwtype_name(col->type), lwtype_name(geom->type));
		return NULL;
	}

	/* In case this is a truly empty, make some initial space  */
	if (!col->geoms)
	{
		col->maxgeoms = 2;
		col->ngeoms = 0;
		col->geoms = lwalloc(col->maxgeoms * sizeof(LWGEOM*));
	}

	/* Allocate more space if we need it */
	lwcollection_reserve(col, col->ngeoms + 1);

#if PARANOIA_LEVEL > 1
	/* See http://trac.osgeo.org/postgis/ticket/2933 */
	/* Make sure we don't already have a reference to this geom */
	{
		uint32_t i = 0;
		for (i = 0; i < col->ngeoms; i++)
		{
			if (col->geoms[i] == geom)
			{
				lwerror("%s [%d] found duplicate geometry in collection %p == %p", __FILE__, __LINE__, col->geoms[i], geom);
				return col;
			}
		}
	}
#endif

	col->geoms[col->ngeoms] = (LWGEOM*)geom;
	col->ngeoms++;
	return col;
}

/**
 * Appends all geometries from col2 to col1 in place.
 * Caller is responsible to release col2.
 */
LWCOLLECTION *
lwcollection_concat_in_place(LWCOLLECTION *col1, const LWCOLLECTION *col2)
{
	uint32_t i;
	if (!col1 || !col2) return NULL;
	for (i = 0; i < col2->ngeoms; i++)
		col1 = lwcollection_add_lwgeom(col1, col2->geoms[i]);
	return col1;
}

LWCOLLECTION*
lwcollection_segmentize2d(const LWCOLLECTION* col, double dist)
{
	uint32_t i, j;
	LWGEOM** newgeoms;

	if (!col->ngeoms) return lwcollection_clone(col);

	newgeoms = lwalloc(sizeof(LWGEOM*) * col->ngeoms);
	for (i = 0; i < col->ngeoms; i++)
	{
		newgeoms[i] = lwgeom_segmentize2d(col->geoms[i], dist);
		if (!newgeoms[i])
		{
			for (j = 0; j < i; j++)
				lwgeom_free(newgeoms[j]);
			lwfree(newgeoms);
			return NULL;
		}
	}

	return lwcollection_construct(
	    col->type, col->srid, NULL, col->ngeoms, newgeoms);
}

/** @brief check for same geometry composition
 *
 */
char
lwcollection_same(const LWCOLLECTION *c1, const LWCOLLECTION *c2)
{
	uint32_t i;

	LWDEBUG(2, "lwcollection_same called");

	if ( c1->type != c2->type ) return LW_FALSE;
	if ( c1->ngeoms != c2->ngeoms ) return LW_FALSE;

	for ( i = 0; i < c1->ngeoms; i++ )
	{
		if ( ! lwgeom_same(c1->geoms[i], c2->geoms[i]) )
			return LW_FALSE;
	}

	/* Former method allowed out-of-order equality between collections

		hit = lwalloc(sizeof(uint32_t)*c1->ngeoms);
		memset(hit, 0, sizeof(uint32_t)*c1->ngeoms);

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
			if ( ! found ) return LW_FALSE;
		}
	*/

	return LW_TRUE;
}

int lwcollection_ngeoms(const LWCOLLECTION *col)
{
	uint32_t i;
	int ngeoms = 0;

	if ( ! col )
	{
		lwerror("Null input geometry.");
		return 0;
	}

	for ( i = 0; i < col->ngeoms; i++ )
	{
		if ( col->geoms[i])
		{
			switch (col->geoms[i]->type)
			{
			case POINTTYPE:
			case LINETYPE:
			case CIRCSTRINGTYPE:
			case POLYGONTYPE:
				ngeoms += 1;
				break;
			case MULTIPOINTTYPE:
			case MULTILINETYPE:
			case MULTICURVETYPE:
			case MULTIPOLYGONTYPE:
				ngeoms += col->ngeoms;
				break;
			case COLLECTIONTYPE:
				ngeoms += lwcollection_ngeoms((LWCOLLECTION*)col->geoms[i]);
				break;
			}
		}
	}
	return ngeoms;
}

void lwcollection_free(LWCOLLECTION *col)
{
	uint32_t i;
	if ( ! col ) return;

	if ( col->bbox )
	{
		lwfree(col->bbox);
	}
	for ( i = 0; i < col->ngeoms; i++ )
	{
		LWDEBUGF(4,"freeing geom[%d]", i);
		if ( col->geoms && col->geoms[i] )
			lwgeom_free(col->geoms[i]);
	}
	if ( col->geoms )
	{
		lwfree(col->geoms);
	}
	lwfree(col);
}


/**
* Takes a potentially heterogeneous collection and returns a homogeneous
* collection consisting only of the specified type.
* WARNING: the output will contain references to geometries in the input,
* so the result must be carefully released, not freed.
*/
LWCOLLECTION*
lwcollection_extract(LWCOLLECTION* col, int type)
{
	uint32_t i = 0;
	LWGEOM** geomlist;
	LWCOLLECTION* outcol;
	int geomlistsize = 16;
	int geomlistlen = 0;
	uint8_t outtype;

	if (!col) return NULL;

	switch (type)
	{
	case POINTTYPE:
		outtype = MULTIPOINTTYPE;
		break;
	case LINETYPE:
		outtype = MULTILINETYPE;
		break;
	case POLYGONTYPE:
		outtype = MULTIPOLYGONTYPE;
		break;
	default:
		lwerror(
		    "Only POLYGON, LINESTRING and POINT are supported by "
		    "lwcollection_extract. %s requested.",
		    lwtype_name(type));
		return NULL;
	}

	geomlist = lwalloc(sizeof(LWGEOM*) * geomlistsize);

	/* Process each sub-geometry */
	for (i = 0; i < col->ngeoms; i++)
	{
		int subtype = col->geoms[i]->type;
		/* Don't bother adding empty sub-geometries */
		if (lwgeom_is_empty(col->geoms[i])) continue;
		/* Copy our sub-types into the output list */
		if (subtype == type)
		{
			/* We've over-run our buffer, double the memory segment
			 */
			if (geomlistlen == geomlistsize)
			{
				geomlistsize *= 2;
				geomlist = lwrealloc(
				    geomlist, sizeof(LWGEOM*) * geomlistsize);
			}
			geomlist[geomlistlen] = lwgeom_clone(col->geoms[i]);
			geomlistlen++;
		}
		/* Recurse into sub-collections */
		if (lwtype_is_collection(subtype))
		{
			uint32_t j = 0;
			LWCOLLECTION* tmpcol = lwcollection_extract(
			    (LWCOLLECTION*)col->geoms[i], type);
			for (j = 0; j < tmpcol->ngeoms; j++)
			{
				/* We've over-run our buffer, double the memory
				 * segment */
				if (geomlistlen == geomlistsize)
				{
					geomlistsize *= 2;
					geomlist = lwrealloc(geomlist,
							     sizeof(LWGEOM*) *
								 geomlistsize);
				}
				geomlist[geomlistlen] = tmpcol->geoms[j];
				geomlistlen++;
			}
			if (tmpcol->ngeoms) lwfree(tmpcol->geoms);
			if (tmpcol->bbox) lwfree(tmpcol->bbox);
			lwfree(tmpcol);
		}
	}

	if (geomlistlen > 0)
	{
		GBOX gbox;
		outcol = lwcollection_construct(
		    outtype, col->srid, NULL, geomlistlen, geomlist);
		lwgeom_calculate_gbox((LWGEOM*)outcol, &gbox);
		outcol->bbox = gbox_copy(&gbox);
	}
	else
	{
		lwfree(geomlist);
		outcol = lwcollection_construct_empty(outtype,
						      col->srid,
						      FLAGS_GET_Z(col->flags),
						      FLAGS_GET_M(col->flags));
	}

	return outcol;
}

LWCOLLECTION*
lwcollection_force_dims(const LWCOLLECTION *col, int hasz, int hasm)
{
	LWCOLLECTION *colout;

	/* Return 2D empty */
	if( lwcollection_is_empty(col) )
	{
		colout = lwcollection_construct_empty(col->type, col->srid, hasz, hasm);
	}
	else
	{
		uint32_t i;
		LWGEOM **geoms = NULL;
		geoms = lwalloc(sizeof(LWGEOM*) * col->ngeoms);
		for( i = 0; i < col->ngeoms; i++ )
		{
			geoms[i] = lwgeom_force_dims(col->geoms[i], hasz, hasm);
		}
		colout = lwcollection_construct(col->type, col->srid, NULL, col->ngeoms, geoms);
	}
	return colout;
}

int lwcollection_is_empty(const LWCOLLECTION *col)
{
	uint32_t i;
	if ( (col->ngeoms == 0) || (!col->geoms) )
		return LW_TRUE;
	for( i = 0; i < col->ngeoms; i++ )
	{
		if ( ! lwgeom_is_empty(col->geoms[i]) ) return LW_FALSE;
	}
	return LW_TRUE;
}


uint32_t lwcollection_count_vertices(LWCOLLECTION *col)
{
	uint32_t i = 0;
	uint32_t v = 0; /* vertices */
	assert(col);
	for ( i = 0; i < col->ngeoms; i++ )
	{
		v += lwgeom_count_vertices(col->geoms[i]);
	}
	return v;
}


int lwcollection_allows_subtype(int collectiontype, int subtype)
{
	if ( collectiontype == COLLECTIONTYPE )
		return LW_TRUE;
	if ( collectiontype == MULTIPOINTTYPE &&
	        subtype == POINTTYPE )
		return LW_TRUE;
	if ( collectiontype == MULTILINETYPE &&
	        subtype == LINETYPE )
		return LW_TRUE;
	if ( collectiontype == MULTIPOLYGONTYPE &&
	        subtype == POLYGONTYPE )
		return LW_TRUE;
	if ( collectiontype == COMPOUNDTYPE &&
	        (subtype == LINETYPE || subtype == CIRCSTRINGTYPE) )
		return LW_TRUE;
	if ( collectiontype == CURVEPOLYTYPE &&
	        (subtype == CIRCSTRINGTYPE || subtype == LINETYPE || subtype == COMPOUNDTYPE) )
		return LW_TRUE;
	if ( collectiontype == MULTICURVETYPE &&
	        (subtype == CIRCSTRINGTYPE || subtype == LINETYPE || subtype == COMPOUNDTYPE) )
		return LW_TRUE;
	if ( collectiontype == MULTISURFACETYPE &&
	        (subtype == POLYGONTYPE || subtype == CURVEPOLYTYPE) )
		return LW_TRUE;
	if ( collectiontype == POLYHEDRALSURFACETYPE &&
	        subtype == POLYGONTYPE )
		return LW_TRUE;
	if ( collectiontype == TINTYPE &&
	        subtype == TRIANGLETYPE )
		return LW_TRUE;

	/* Must be a bad combination! */
	return LW_FALSE;
}

int
lwcollection_startpoint(const LWCOLLECTION* col, POINT4D* pt)
{
	if ( col->ngeoms < 1 )
		return LW_FAILURE;

	return lwgeom_startpoint(col->geoms[0], pt);
}


