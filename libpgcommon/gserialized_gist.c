/**********************************************************************
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

/*#define POSTGIS_DEBUG_LEVEL 4*/

#include "liblwgeom.h"         /* For standard geometry types. */
#include "liblwgeom_internal.h"
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "gserialized_gist.h"

#define FLAGS_NDIMS_GIDX(f) ( FLAGS_GET_GEODETIC(f) ? 3 : \
                              FLAGS_GET_M(f) ? 4 : \
                              FLAGS_GET_Z(f) ? 3 : 2 )


/* Generate human readable form for GIDX. */
char* gidx_to_string(GIDX *a)
{
	static const int precision = 12;
	char tmp[8 + 8 * (OUT_MAX_BYTES_DOUBLE + 1)] = {'G', 'I', 'D', 'X', '(', 0};
	int len = 5;
	int ndims;

	if (a == NULL)
		return pstrdup("<NULLPTR>");

	ndims = GIDX_NDIMS(a);

	for (int i = 0; i < ndims; i++)
	{
		tmp[len++] = ' ';
		len += lwprint_double(GIDX_GET_MIN(a, i), precision, &tmp[len]);
	}
	tmp[len++] = ',';

	for (int i = 0; i < ndims; i++)
	{
		tmp[len++] = ' ';
		len += lwprint_double(GIDX_GET_MAX(a, i), precision, &tmp[len]);
	}

	tmp[len++] = ')';

	return pstrdup(tmp);
}

/* Allocates a new GIDX on the heap of the requested dimensionality */
GIDX* gidx_new(int ndims)
{
	size_t size = GIDX_SIZE(ndims);
	GIDX *g = (GIDX*)palloc(size);
	Assert( (ndims <= GIDX_MAX_DIM) && (size <= GIDX_MAX_SIZE) );
	POSTGIS_DEBUGF(5,"created new gidx of %d dimensions, size %d", ndims, (int)size);
	SET_VARSIZE(g, size);
	return g;
}


/* Convert a double-based GBOX into a float-based GIDX,
   ensuring the float box is larger than the double box */
static int gidx_from_gbox_p(GBOX box, GIDX *a)
{
	int ndims;

	ndims = FLAGS_NDIMS_GIDX(box.flags);
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
		}
		/* M is always fourth dimension, we pad if needed */
		if ( FLAGS_GET_M(box.flags) )
		{
			if ( ! FLAGS_GET_Z(box.flags) )
			{
				GIDX_SET_MIN(a,2,-1*FLT_MAX);
				GIDX_SET_MAX(a,2,FLT_MAX);
			}
			GIDX_SET_MIN(a,3,next_float_down(box.mmin));
			GIDX_SET_MAX(a,3,next_float_up(box.mmax));
		}
	}

	POSTGIS_DEBUGF(5, "converted %s to %s", gbox_to_string(&box), gidx_to_string(a));

	return LW_SUCCESS;
}

/* Convert a gidx to a gbox */
void gbox_from_gidx(GIDX *a, GBOX *gbox, int flags)
{
	gbox->xmin = (double)GIDX_GET_MIN(a,0);
	gbox->xmax = (double)GIDX_GET_MAX(a,0);

	gbox->ymin = (double)GIDX_GET_MIN(a,1);
	gbox->ymax = (double)GIDX_GET_MAX(a,1);

	if ( FLAGS_GET_Z(flags) ) {
		gbox->zmin = (double)GIDX_GET_MIN(a,2);
		gbox->zmax = (double)GIDX_GET_MAX(a,2);
	}

	if ( FLAGS_GET_M(flags) ) {
		gbox->mmin = (double)GIDX_GET_MIN(a,3);
		gbox->mmax = (double)GIDX_GET_MAX(a,3);
	}
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
	GSERIALIZED *gpart = NULL;
	int need_detoast = PG_GSERIALIZED_DATUM_NEEDS_DETOAST((struct varlena *)gsdatum);
	if (need_detoast)
	{
		gpart = (GSERIALIZED *)PG_DETOAST_DATUM_SLICE(gsdatum, 0, gserialized_max_header_size());
	}
	else
	{
		gpart = (GSERIALIZED *)gsdatum;
	}

	/* Do we even have a serialized bounding box? */
	if (gserialized_has_bbox(gpart))
	{
		/* Yes! Copy it out into the GIDX! */
		lwflags_t lwflags = gserialized_get_lwflags(gpart);
		size_t size = gbox_serialized_size(lwflags);
		size_t ndims, dim;
		const float *f = gserialized_get_float_box_p(gpart, &ndims);
		if (!f) return LW_FAILURE;
		for (dim = 0; dim < ndims; dim++)
		{
			GIDX_SET_MIN(gidx, dim, f[2*dim]);
			GIDX_SET_MAX(gidx, dim, f[2*dim+1]);
		}
		/* if M is present but Z is not, pad Z and shift M */
		if (gserialized_has_m(gpart) && ! gserialized_has_z(gpart))
		{
			size += 2 * sizeof(float);
			GIDX_SET_MIN(gidx,3,GIDX_GET_MIN(gidx,2));
			GIDX_SET_MAX(gidx,3,GIDX_GET_MAX(gidx,2));
			GIDX_SET_MIN(gidx,2,-1*FLT_MAX);
			GIDX_SET_MAX(gidx,2,FLT_MAX);
		}
		SET_VARSIZE(gidx, VARHDRSZ + size);
	}
	else
	{
		/* No, we need to calculate it from the full object. */
		LWGEOM *lwgeom;
		GBOX gbox;
		if (need_detoast && LWSIZE_GET(gpart->size) >= gserialized_max_header_size())
		{
			/* If we haven't, read the whole gserialized object */
			POSTGIS_FREE_IF_COPY_P(gpart, gsdatum);
			gpart = (GSERIALIZED *)PG_DETOAST_DATUM(gsdatum);
		}

		lwgeom = lwgeom_from_gserialized(gpart);
		if (lwgeom_calculate_gbox(lwgeom, &gbox) == LW_FAILURE)
		{
			POSTGIS_DEBUG(4, "could not calculate bbox, returning failure");
			lwgeom_free(lwgeom);
			POSTGIS_FREE_IF_COPY_P(gpart, gsdatum);
			return LW_FAILURE;
		}
		lwgeom_free(lwgeom);
		gidx_from_gbox_p(gbox, gidx);
	}
	POSTGIS_FREE_IF_COPY_P(gpart, gsdatum);
	POSTGIS_DEBUGF(4, "got gidx %s", gidx_to_string(gidx));
	return LW_SUCCESS;
}



