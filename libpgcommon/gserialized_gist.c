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


#include "postgres.h"
#include "access/gist.h"    /* For GiST */
#include "access/itup.h"
#include "access/skey.h"

#include "../postgis_config.h"

#include "liblwgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "gserialized_gist.h"



/**
* Given a #GSERIALIZED datum, as quickly as possible (peaking into the top
* of the memory) return the gbox extents. Does not deserialize the geometry,
* but <em>WARNING</em> returns a slightly larger bounding box than actually
* encompasses the objects. For geography objects returns geocentric bounding
* box, for geometry objects returns cartesian bounding box.
*/
int 
gserialized_datum_get_gbox_p(Datum gsdatum, GBOX *gbox)
{
	char gboxmem[GIDX_MAX_SIZE];
	GIDX *gidx = (GIDX*)gboxmem;
	
	if( LW_FAILURE == gserialized_datum_get_gidx_p(gsdatum, gidx) )
		return LW_FAILURE;
		
	gbox_from_gidx(gidx, gbox);
	return LW_SUCCESS;
}


/**
* Update the bounding box of a #GSERIALIZED, allocating a fresh one
* if there is not enough space to just write the new box in. 
* <em>WARNING</em> if a new object needs to be created, the 
* input pointer will have to be freed by the caller! Check
* to see if input == output. Returns null if there's a problem
* like mismatched dimensions.
*/
GSERIALIZED* gserialized_set_gidx(GSERIALIZED *g, GIDX *gidx)
{
	int g_ndims = (FLAGS_GET_GEODETIC(g->flags) ? 3 : FLAGS_NDIMS(g->flags));
	int box_ndims = GIDX_NDIMS(gidx);
	GSERIALIZED *g_out = NULL;
	size_t box_size = 2 * g_ndims * sizeof(float);

	/* The dimensionality of the inputs has to match or we are SOL. */
	if ( g_ndims != box_ndims )
	{
		return NULL;
	}

	/* Serialized already has room for a box. */
	if ( FLAGS_GET_BBOX(g->flags) )
	{
		g_out = g;
	}
	/* Serialized has no box. We need to allocate enough space for the old
	   data plus the box, and leave a gap in the memory segment to write
	   the new values into.
	*/
	else
	{
		size_t varsize_new = VARSIZE(g) + box_size;
		uint8_t *ptr;
		g_out = palloc(varsize_new);
		/* Copy the head of g into place */
		memcpy(g_out, g, 8);
		/* Copy the body of g into place after leaving space for the box */
		ptr = g_out->data;
		ptr += box_size;
		memcpy(ptr, g->data, VARSIZE(g) - 8);
		FLAGS_SET_BBOX(g_out->flags, 1);
		SET_VARSIZE(g_out, varsize_new);
	}

	/* Now write the gidx values into the memory segement */
	memcpy(g_out->data, gidx->c, box_size);

	return g_out;
}


/**
* Remove the bounding box from a #GSERIALIZED. Returns a freshly
* allocated #GSERIALIZED every time.
*/
GSERIALIZED* gserialized_drop_gidx(GSERIALIZED *g)
{
	int g_ndims = (FLAGS_GET_GEODETIC(g->flags) ? 3 : FLAGS_NDIMS(g->flags));
	size_t box_size = 2 * g_ndims * sizeof(float);
	size_t g_out_size = VARSIZE(g) - box_size;
	GSERIALIZED *g_out = palloc(g_out_size);

	/* Copy the contents while omitting the box */
	if ( FLAGS_GET_BBOX(g->flags) )
	{
		uint8_t *outptr = (uint8_t*)g_out;
		uint8_t *inptr = (uint8_t*)g;
		/* Copy the header (size+type) of g into place */
		memcpy(outptr, inptr, 8);
		outptr += 8;
		inptr += 8 + box_size;
		/* Copy parts after the box into place */
		memcpy(outptr, inptr, g_out_size - 8);
		FLAGS_SET_BBOX(g_out->flags, 0);
		SET_VARSIZE(g_out, g_out_size);
	}
	/* No box? Nothing to do but copy and return. */
	else
	{
		memcpy(g_out, g, g_out_size);
	}

	return g_out;
}


/* Convert a double-based GBOX into a float-based GIDX,
   ensuring the float box is larger than the double box */
static int gidx_from_gbox_p(GBOX box, GIDX *a)
{
	int ndims;

	ndims = (FLAGS_GET_GEODETIC(box.flags) ? 3 : FLAGS_NDIMS(box.flags));
	SET_VARSIZE(a, VARHDRSZ + ndims * 2 * sizeof(float));

	GIDX_SET_MIN(a,0,next_float_down(box.xmin));
	GIDX_SET_MAX(a,0,next_float_up(box.xmax));
	GIDX_SET_MIN(a,1,next_float_down(box.ymin));
	GIDX_SET_MAX(a,1,next_float_up(box.ymax));

	/* Geodetic indexes are always 3d, geocentric x/y/z */
	if ( FLAGS_GET_GEODETIC(box.flags) )
	{
		GIDX_SET_MIN(a,2,next_float_down(box.zmin));
		GIDX_SET_MAX(a,2,next_float_up(box.zmax));
	}
	else
	{
		/* Cartesian with Z implies Z is third dimension */
		if ( FLAGS_GET_Z(box.flags) )
		{
			GIDX_SET_MIN(a,2,next_float_down(box.zmin));
			GIDX_SET_MAX(a,2,next_float_up(box.zmax));
			if ( FLAGS_GET_M(box.flags) )
			{
				GIDX_SET_MIN(a,3,next_float_down(box.mmin));
				GIDX_SET_MAX(a,3,next_float_up(box.mmax));
			}
		}
		/* Unless there's no Z, in which case M is third dimension */
		else if ( FLAGS_GET_M(box.flags) )
		{
			GIDX_SET_MIN(a,2,next_float_down(box.mmin));
			GIDX_SET_MAX(a,2,next_float_up(box.mmax));
		}
	}

	POSTGIS_DEBUGF(5, "converted %s to %s", gbox_to_string(&box), gidx_to_string(a));

	return LW_SUCCESS;
}


GIDX* gidx_from_gbox(GBOX box)
{
	int	ndims;
	GIDX *a;

	ndims = (FLAGS_GET_GEODETIC(box.flags) ? 3 : FLAGS_NDIMS(box.flags));
	a = gidx_new(ndims);
	gidx_from_gbox_p(box, a);
	return a;
}


void gbox_from_gidx(GIDX *a, GBOX *gbox)
{
	gbox->xmin = (double)GIDX_GET_MIN(a,0);
	gbox->ymin = (double)GIDX_GET_MIN(a,1);
	gbox->zmin = (double)GIDX_GET_MIN(a,2);
	gbox->xmax = (double)GIDX_GET_MAX(a,0);
	gbox->ymax = (double)GIDX_GET_MAX(a,1);
	gbox->zmax = (double)GIDX_GET_MAX(a,2);
}

/**
* Peak into a #GSERIALIZED datum to find the bounding box. If the
* box is there, copy it out and return it. If not, calculate the box from the
* full object and return the box based on that. If no box is available,
* return #LW_FAILURE, otherwise #LW_SUCCESS.
*/
int 
gserialized_datum_get_gidx_p(Datum gsdatum, GIDX *gidx)
{
	GSERIALIZED *gpart;
	int result = LW_SUCCESS;

	POSTGIS_DEBUG(4, "entered function");

	/*
	** The most info we need is the 8 bytes of serialized header plus the 32 bytes
	** of floats necessary to hold the 8 floats of the largest XYZM index
	** bounding box, so 40 bytes.
	*/
	gpart = (GSERIALIZED*)PG_DETOAST_DATUM_SLICE(gsdatum, 0, 40);

	POSTGIS_DEBUGF(4, "got flags %d", gpart->flags);

	/* Do we even have a serialized bounding box? */
	if ( FLAGS_GET_BBOX(gpart->flags) )
	{
		/* Yes! Copy it out into the GIDX! */
		const size_t size = gbox_serialized_size(gpart->flags);
		POSTGIS_DEBUG(4, "copying box out of serialization");
		memcpy(gidx->c, gpart->data, size);
		SET_VARSIZE(gidx, VARHDRSZ + size);
		result = LW_SUCCESS;
	}
	else
	{
		/* No, we need to calculate it from the full object. */
		GSERIALIZED *g = (GSERIALIZED*)PG_DETOAST_DATUM(gsdatum);
		LWGEOM *lwgeom = lwgeom_from_gserialized(g);
		GBOX gbox;
		if ( lwgeom_calculate_gbox(lwgeom, &gbox) == LW_FAILURE )
		{
			POSTGIS_DEBUG(4, "could not calculate bbox, returning failure");
			lwgeom_free(lwgeom);
			return LW_FAILURE;
		}
		lwgeom_free(lwgeom);
		result = gidx_from_gbox_p(gbox, gidx);
	}
	
	if ( result == LW_SUCCESS )
	{
		POSTGIS_DEBUGF(4, "got gidx %s", gidx_to_string(gidx));
	}

	return result;
}


/*
** Peak into a geography to find the bounding box. If the
** box is there, copy it out and return it. If not, calculate the box from the
** full geography and return the box based on that. If no box is available,
** return LW_FAILURE, otherwise LW_SUCCESS.
*/
int gserialized_get_gidx_p(GSERIALIZED *g, GIDX *gidx)
{
	int result = LW_SUCCESS;

	POSTGIS_DEBUG(4, "entered function");

	POSTGIS_DEBUGF(4, "got flags %d", g->flags);

	if ( FLAGS_GET_BBOX(g->flags) )
	{
		int ndims = FLAGS_NDIMS_BOX(g->flags);
		const size_t size = 2 * ndims * sizeof(float);
		POSTGIS_DEBUG(4, "copying box out of serialization");
		memcpy(gidx->c, g->data, size);
		SET_VARSIZE(gidx, VARHDRSZ + size);
	}
	else
	{
		/* No, we need to calculate it from the full object. */
		LWGEOM *lwgeom = lwgeom_from_gserialized(g);
		GBOX gbox;
		if ( lwgeom_calculate_gbox(lwgeom, &gbox) == LW_FAILURE )
		{
			POSTGIS_DEBUG(4, "could not calculate bbox, returning failure");
			lwgeom_free(lwgeom);
			return LW_FAILURE;
		}
		lwgeom_free(lwgeom);
		result = gidx_from_gbox_p(gbox, gidx);
	}
	if ( result == LW_SUCCESS )
	{
		POSTGIS_DEBUGF(4, "got gidx %s", gidx_to_string(gidx));
	}

	return result;
}


/*
** GIDX expansion, make d units bigger in all dimensions.
*/
void gidx_expand(GIDX *a, float d)
{
	int i;

	POSTGIS_DEBUG(5, "entered function");

	if ( a == NULL ) return;

	for (i = 0; i < GIDX_NDIMS(a); i++)
	{
		GIDX_SET_MIN(a, i, GIDX_GET_MIN(a, i) - d);
		GIDX_SET_MAX(a, i, GIDX_GET_MAX(a, i) + d);
	}
}


/* Allocates a new GIDX on the heap of the requested dimensionality */
GIDX* gidx_new(int ndims)
{
	size_t size = GIDX_SIZE(ndims);
	GIDX *g = (GIDX*)palloc(size);
	POSTGIS_DEBUGF(5,"created new gidx of %d dimensions, size %d", ndims, (int)size);
	SET_VARSIZE(g, size);
	return g;
}

