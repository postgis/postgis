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
		uchar *ptr;
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
		uchar *outptr = (uchar*)g_out;
		uchar *inptr = (uchar*)g;
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





