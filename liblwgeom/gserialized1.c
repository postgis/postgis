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
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2017 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/


#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "lwgeodetic.h"
#include "gserialized1.h"

#include <stddef.h>

/***********************************************************************
* GSERIALIZED metadata utility functions.
*/

static int gserialized1_read_gbox_p(const GSERIALIZED *g, GBOX *gbox);


lwflags_t gserialized1_get_lwflags(const GSERIALIZED *g)
{
	lwflags_t lwflags = 0;
	uint8_t gflags = g->gflags;
	FLAGS_SET_Z(lwflags, G1FLAGS_GET_Z(gflags));
	FLAGS_SET_M(lwflags, G1FLAGS_GET_M(gflags));
	FLAGS_SET_BBOX(lwflags, G1FLAGS_GET_BBOX(gflags));
	FLAGS_SET_GEODETIC(lwflags, G1FLAGS_GET_GEODETIC(gflags));
	FLAGS_SET_SOLID(lwflags, G1FLAGS_GET_SOLID(gflags));
	return lwflags;
}

uint8_t lwflags_get_g1flags(lwflags_t lwflags)
{
	uint8_t gflags = 0;
	G1FLAGS_SET_Z(gflags, FLAGS_GET_Z(lwflags));
	G1FLAGS_SET_M(gflags, FLAGS_GET_M(lwflags));
	G1FLAGS_SET_BBOX(gflags, FLAGS_GET_BBOX(lwflags));
	G1FLAGS_SET_GEODETIC(gflags, FLAGS_GET_GEODETIC(lwflags));
	G1FLAGS_SET_SOLID(gflags, FLAGS_GET_SOLID(lwflags));
	return gflags;
}


static size_t gserialized1_box_size(const GSERIALIZED *g)
{
	if (G1FLAGS_GET_GEODETIC(g->gflags))
		return 6 * sizeof(float);
	else
		return 2 * G1FLAGS_NDIMS(g->gflags) * sizeof(float);
}

/* handle missaligned uint32_t data */
static inline uint32_t gserialized1_get_uint32_t(const uint8_t *loc)
{
	return *((uint32_t*)loc);
}

uint8_t g1flags(int has_z, int has_m, int is_geodetic)
{
	uint8_t gflags = 0;
	if (has_z)
		G1FLAGS_SET_Z(gflags, 1);
	if (has_m)
		G1FLAGS_SET_M(gflags, 1);
	if (is_geodetic)
		G1FLAGS_SET_GEODETIC(gflags, 1);
	return gflags;
}

int gserialized1_has_bbox(const GSERIALIZED *gser)
{
	return G1FLAGS_GET_BBOX(gser->gflags);
}

int gserialized1_has_z(const GSERIALIZED *gser)
{
	return G1FLAGS_GET_Z(gser->gflags);
}

int gserialized1_has_m(const GSERIALIZED *gser)
{
	return G1FLAGS_GET_M(gser->gflags);
}

int gserialized1_ndims(const GSERIALIZED *gser)
{
	return G1FLAGS_NDIMS(gser->gflags);
}

int gserialized1_is_geodetic(const GSERIALIZED *gser)
{
	  return G1FLAGS_GET_GEODETIC(gser->gflags);
}

uint32_t gserialized1_max_header_size(void)
{
	/* GSERIALIZED size + max bbox according gbox_serialized_size (XYZM*2) + extended flags + type */
	return offsetof(GSERIALIZED, data) + 8 * sizeof(float) + sizeof(uint32_t);
}

static uint32_t gserialized1_header_size(const GSERIALIZED *gser)
{
	uint32_t sz = 8; /* varsize (4) + srid(3) + flags (1) */

	if (gserialized1_has_bbox(gser))
		sz += gserialized1_box_size(gser);

	return sz;
}

uint32_t gserialized1_get_type(const GSERIALIZED *g)
{
	uint32_t *ptr;
	ptr = (uint32_t*)(g->data);
	if ( G1FLAGS_GET_BBOX(g->gflags) )
	{
		ptr += (gserialized1_box_size(g) / sizeof(uint32_t));
	}
	return *ptr;
}

int32_t gserialized1_get_srid(const GSERIALIZED *s)
{
	int32_t srid = 0;
	srid = srid | (s->srid[0] << 16);
	srid = srid | (s->srid[1] << 8);
	srid = srid | s->srid[2];
	/* Only the first 21 bits are set. Slide up and back to pull
	   the negative bits down, if we need them. */
	srid = (srid<<11)>>11;

	/* 0 is our internal unknown value. We'll map back and forth here for now */
	if ( srid == 0 )
		return SRID_UNKNOWN;
	else
		return srid;
}

void gserialized1_set_srid(GSERIALIZED *s, int32_t srid)
{
	LWDEBUGF(3, "%s called with srid = %d", __func__, srid);

	srid = clamp_srid(srid);

	/* 0 is our internal unknown value.
	 * We'll map back and forth here for now */
	if ( srid == SRID_UNKNOWN )
		srid = 0;

	s->srid[0] = (srid & 0x001F0000) >> 16;
	s->srid[1] = (srid & 0x0000FF00) >> 8;
	s->srid[2] = (srid & 0x000000FF);
}

static size_t gserialized1_is_empty_recurse(const uint8_t *p, int *isempty);
static size_t gserialized1_is_empty_recurse(const uint8_t *p, int *isempty)
{
	int i;
	int32_t type, num;

	memcpy(&type, p, 4);
	memcpy(&num, p+4, 4);

	if ( lwtype_is_collection(type) )
	{
		size_t lz = 8;
		for ( i = 0; i < num; i++ )
		{
			lz += gserialized1_is_empty_recurse(p+lz, isempty);
			if ( ! *isempty )
				return lz;
		}
		*isempty = LW_TRUE;
		return lz;
	}
	else
	{
		*isempty = (num == 0 ? LW_TRUE : LW_FALSE);
		return 8;
	}
}

int gserialized1_is_empty(const GSERIALIZED *g)
{
	uint8_t *p = (uint8_t*)g;
	int isempty = 0;
	assert(g);

	p += 8; /* Skip varhdr and srid/flags */
	if(gserialized1_has_bbox(g))
		p += gserialized1_box_size(g); /* Skip the box */

	gserialized1_is_empty_recurse(p, &isempty);
	return isempty;
}


/* Prototype for lookup3.c */
/* key = the key to hash */
/* length = length of the key */
/* pc = IN: primary initval, OUT: primary hash */
/* pb = IN: secondary initval, OUT: secondary hash */
void hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);

int32_t
gserialized1_hash(const GSERIALIZED *g1)
{
	int32_t hval;
	int32_t pb = 0, pc = 0;
	/* Point to just the type/coordinate part of buffer */
	size_t hsz1 = gserialized1_header_size(g1);
	uint8_t *b1 = (uint8_t*)g1 + hsz1;
	/* Calculate size of type/coordinate buffer */
	size_t sz1 = LWSIZE_GET(g1->size);
	size_t bsz1 = sz1 - hsz1;
	/* Calculate size of srid/type/coordinate buffer */
	int32_t srid = gserialized1_get_srid(g1);
	size_t bsz2 = bsz1 + sizeof(int);
	uint8_t *b2 = lwalloc(bsz2);
	/* Copy srid into front of combined buffer */
	memcpy(b2, &srid, sizeof(int));
	/* Copy type/coordinates into rest of combined buffer */
	memcpy(b2+sizeof(int), b1, bsz1);
	/* Hash combined buffer */
	hashlittle2(b2, bsz2, (uint32_t *)&pb, (uint32_t *)&pc);
	lwfree(b2);
	hval = pb ^ pc;
	return hval;
}

int gserialized1_read_gbox_p(const GSERIALIZED *g, GBOX *gbox)
{

	/* Null input! */
	if ( ! ( g && gbox ) ) return LW_FAILURE;

	/* Initialize the flags on the box */
	gbox->flags = gserialized1_get_lwflags(g);

	/* Has pre-calculated box */
	if ( G1FLAGS_GET_BBOX(g->gflags) )
	{
		int i = 0;
		float *fbox = (float*)(g->data);
		gbox->xmin = fbox[i++];
		gbox->xmax = fbox[i++];
		gbox->ymin = fbox[i++];
		gbox->ymax = fbox[i++];

		/* Geodetic? Read next dimension (geocentric Z) and return */
		if ( G1FLAGS_GET_GEODETIC(g->gflags) )
		{
			gbox->zmin = fbox[i++];
			gbox->zmax = fbox[i++];
			return LW_SUCCESS;
		}
		/* Cartesian? Read extra dimensions (if there) and return */
		if ( G1FLAGS_GET_Z(g->gflags) )
		{
			gbox->zmin = fbox[i++];
			gbox->zmax = fbox[i++];
		}
		if ( G1FLAGS_GET_M(g->gflags) )
		{
			gbox->mmin = fbox[i++];
			gbox->mmax = fbox[i++];
		}
		return LW_SUCCESS;
	}
	return LW_FAILURE;
}

/*
* Populate a bounding box *without* allocating an LWGEOM. Useful
* for some performance purposes.
*/
int
gserialized1_peek_gbox_p(const GSERIALIZED *g, GBOX *gbox)
{
	uint32_t type = gserialized1_get_type(g);

	/* Peeking doesn't help if you already have a box or are geodetic */
	if ( G1FLAGS_GET_GEODETIC(g->gflags) || G1FLAGS_GET_BBOX(g->gflags) )
	{
		return LW_FAILURE;
	}

	/* Boxes of points are easy peasy */
	if ( type == POINTTYPE )
	{
		int i = 1; /* Start past <pointtype><padding> */
		double *dptr = (double*)(g->data);

		/* Read the empty flag */
		int32_t *iptr = (int32_t *)(g->data);
		int isempty = (iptr[1] == 0);

		/* EMPTY point has no box */
		if ( isempty ) return LW_FAILURE;

		gbox->xmin = gbox->xmax = dptr[i++];
		gbox->ymin = gbox->ymax = dptr[i++];
		gbox->flags = gserialized1_get_lwflags(g);
		if ( G1FLAGS_GET_Z(g->gflags) )
		{
			gbox->zmin = gbox->zmax = dptr[i++];
		}
		if ( G1FLAGS_GET_M(g->gflags) )
		{
			gbox->mmin = gbox->mmax = dptr[i++];
		}
		gbox_float_round(gbox);
		return LW_SUCCESS;
	}
	/* We can calculate the box of a two-point cartesian line trivially */
	else if ( type == LINETYPE )
	{
		int ndims = G1FLAGS_NDIMS(g->gflags);
		int i = 0; /* Start at <linetype><npoints> */
		double *dptr = (double*)(g->data);
		int32_t *iptr = (int32_t *)(g->data);
		int npoints = iptr[1]; /* Read the npoints */

		/* This only works with 2-point lines */
		if ( npoints != 2 )
			return LW_FAILURE;

		/* Advance to X */
		/* Past <linetype><npoints> */
		i++;
		gbox->xmin = FP_MIN(dptr[i], dptr[i+ndims]);
		gbox->xmax = FP_MAX(dptr[i], dptr[i+ndims]);

		/* Advance to Y */
		i++;
		gbox->ymin = FP_MIN(dptr[i], dptr[i+ndims]);
		gbox->ymax = FP_MAX(dptr[i], dptr[i+ndims]);

		gbox->flags = gserialized1_get_lwflags(g);
		if ( G1FLAGS_GET_Z(g->gflags) )
		{
			/* Advance to Z */
			i++;
			gbox->zmin = FP_MIN(dptr[i], dptr[i+ndims]);
			gbox->zmax = FP_MAX(dptr[i], dptr[i+ndims]);
		}
		if ( G1FLAGS_GET_M(g->gflags) )
		{
			/* Advance to M */
			i++;
			gbox->mmin = FP_MIN(dptr[i], dptr[i+ndims]);
			gbox->mmax = FP_MAX(dptr[i], dptr[i+ndims]);
		}
		gbox_float_round(gbox);
		return LW_SUCCESS;
	}
	/* We can also do single-entry multi-points */
	else if ( type == MULTIPOINTTYPE )
	{
		int i = 0; /* Start at <multipointtype><ngeoms> */
		double *dptr = (double*)(g->data);
		int32_t *iptr = (int32_t *)(g->data);
		int ngeoms = iptr[1]; /* Read the ngeoms */
		int npoints;

		/* This only works with single-entry multipoints */
		if ( ngeoms != 1 )
			return LW_FAILURE;

		/* Npoints is at <multipointtype><ngeoms><pointtype><npoints> */
		npoints = iptr[3];

		/* The check below is necessary because we can have a MULTIPOINT
		 * that contains a single, empty POINT (ngeoms = 1, npoints = 0) */
		if ( npoints != 1 )
			return LW_FAILURE;

		/* Move forward two doubles (four ints) */
		/* Past <multipointtype><ngeoms> */
		/* Past <pointtype><npoints> */
		i += 2;

		/* Read the doubles from the one point */
		gbox->xmin = gbox->xmax = dptr[i++];
		gbox->ymin = gbox->ymax = dptr[i++];
		gbox->flags = gserialized1_get_lwflags(g);
		if ( G1FLAGS_GET_Z(g->gflags) )
		{
			gbox->zmin = gbox->zmax = dptr[i++];
		}
		if ( G1FLAGS_GET_M(g->gflags) )
		{
			gbox->mmin = gbox->mmax = dptr[i++];
		}
		gbox_float_round(gbox);
		return LW_SUCCESS;
	}
	/* And we can do single-entry multi-lines with two vertices (!!!) */
	else if ( type == MULTILINETYPE )
	{
		int ndims = G1FLAGS_NDIMS(g->gflags);
		int i = 0; /* Start at <multilinetype><ngeoms> */
		double *dptr = (double*)(g->data);
		int32_t *iptr = (int32_t *)(g->data);
		int ngeoms = iptr[1]; /* Read the ngeoms */
		int npoints;

		/* This only works with 1-line multilines */
		if ( ngeoms != 1 )
			return LW_FAILURE;

		/* Npoints is at <multilinetype><ngeoms><linetype><npoints> */
		npoints = iptr[3];

		if ( npoints != 2 )
			return LW_FAILURE;

		/* Advance to X */
		/* Move forward two doubles (four ints) */
		/* Past <multilinetype><ngeoms> */
		/* Past <linetype><npoints> */
		i += 2;
		gbox->xmin = FP_MIN(dptr[i], dptr[i+ndims]);
		gbox->xmax = FP_MAX(dptr[i], dptr[i+ndims]);

		/* Advance to Y */
		i++;
		gbox->ymin = FP_MIN(dptr[i], dptr[i+ndims]);
		gbox->ymax = FP_MAX(dptr[i], dptr[i+ndims]);

		gbox->flags = gserialized1_get_lwflags(g);
		if ( G1FLAGS_GET_Z(g->gflags) )
		{
			/* Advance to Z */
			i++;
			gbox->zmin = FP_MIN(dptr[i], dptr[i+ndims]);
			gbox->zmax = FP_MAX(dptr[i], dptr[i+ndims]);
		}
		if ( G1FLAGS_GET_M(g->gflags) )
		{
			/* Advance to M */
			i++;
			gbox->mmin = FP_MIN(dptr[i], dptr[i+ndims]);
			gbox->mmax = FP_MAX(dptr[i], dptr[i+ndims]);
		}
		gbox_float_round(gbox);
		return LW_SUCCESS;
	}

	return LW_FAILURE;
}

static inline void
gserialized1_copy_point(double *dptr, lwflags_t flags, POINT4D *out_point)
{
	uint8_t dim = 0;
	out_point->x = dptr[dim++];
	out_point->y = dptr[dim++];

	if (G1FLAGS_GET_Z(flags))
	{
		out_point->z = dptr[dim++];
	}
	if (G1FLAGS_GET_M(flags))
	{
		out_point->m = dptr[dim];
	}
}

int
gserialized1_peek_first_point(const GSERIALIZED *g, POINT4D *out_point)
{
	uint8_t *geometry_start = ((uint8_t *)g->data);
	if (gserialized1_has_bbox(g))
	{
		geometry_start += gserialized1_box_size(g);
	}

	uint32_t isEmpty = (((uint32_t *)geometry_start)[1]) == 0;
	if (isEmpty)
	{
		return LW_FAILURE;
	}

	uint32_t type = (((uint32_t *)geometry_start)[0]);
	/* Setup double_array_start depending on the geometry type */
	double *double_array_start = NULL;
	switch (type)
	{
	case (POINTTYPE):
		/* For points we only need to jump over the type and npoints 32b ints */
		double_array_start = (double *)(geometry_start + 2 * sizeof(uint32_t));
		break;

	default:
		lwerror("%s is currently not implemented for type %d", __func__, type);
		return LW_FAILURE;
	}

	gserialized1_copy_point(double_array_start, g->gflags, out_point);
	return LW_SUCCESS;
}

/**
* Read the bounding box off a serialization and calculate one if
* it is not already there.
*/
int gserialized1_get_gbox_p(const GSERIALIZED *g, GBOX *box)
{
	/* Try to just read the serialized box. */
	if ( gserialized1_read_gbox_p(g, box) == LW_SUCCESS )
	{
		return LW_SUCCESS;
	}
	/* No box? Try to peek into simpler geometries and */
	/* derive a box without creating an lwgeom */
	else if ( gserialized1_peek_gbox_p(g, box) == LW_SUCCESS )
	{
		return LW_SUCCESS;
	}
	/* Damn! Nothing for it but to create an lwgeom... */
	/* See http://trac.osgeo.org/postgis/ticket/1023 */
	else
	{
		LWGEOM *lwgeom = lwgeom_from_gserialized(g);
		int ret = lwgeom_calculate_gbox(lwgeom, box);
		gbox_float_round(box);
		lwgeom_free(lwgeom);
		return ret;
	}
}

/**
* Read the bounding box off a serialization and fail if
* it is not already there.
*/
int gserialized1_fast_gbox_p(const GSERIALIZED *g, GBOX *box)
{
	/* Try to just read the serialized box. */
	if ( gserialized1_read_gbox_p(g, box) == LW_SUCCESS )
	{
		return LW_SUCCESS;
	}
	/* No box? Try to peek into simpler geometries and */
	/* derive a box without creating an lwgeom */
	else if ( gserialized1_peek_gbox_p(g, box) == LW_SUCCESS )
	{
		return LW_SUCCESS;
	}
	else
	{
		return LW_FAILURE;
	}
}




/***********************************************************************
* Calculate the GSERIALIZED size for an LWGEOM.
*/

/* Private functions */

static size_t gserialized1_from_any_size(const LWGEOM *geom); /* Local prototype */

static size_t gserialized1_from_lwpoint_size(const LWPOINT *point)
{
	size_t size = 4; /* Type number. */

	assert(point);

	size += 4; /* Number of points (one or zero (empty)). */
	size += point->point->npoints * FLAGS_NDIMS(point->flags) * sizeof(double);

	LWDEBUGF(3, "point size = %d", size);

	return size;
}

static size_t gserialized1_from_lwline_size(const LWLINE *line)
{
	size_t size = 4; /* Type number. */

	assert(line);

	size += 4; /* Number of points (zero => empty). */
	size += line->points->npoints * FLAGS_NDIMS(line->flags) * sizeof(double);

	LWDEBUGF(3, "linestring size = %d", size);

	return size;
}

static size_t gserialized1_from_lwtriangle_size(const LWTRIANGLE *triangle)
{
	size_t size = 4; /* Type number. */

	assert(triangle);

	size += 4; /* Number of points (zero => empty). */
	size += triangle->points->npoints * FLAGS_NDIMS(triangle->flags) * sizeof(double);

	LWDEBUGF(3, "triangle size = %d", size);

	return size;
}

static size_t gserialized1_from_lwpoly_size(const LWPOLY *poly)
{
	size_t size = 4; /* Type number. */
	uint32_t i = 0;

	assert(poly);

	size += 4; /* Number of rings (zero => empty). */
	if ( poly->nrings % 2 )
		size += 4; /* Padding to double alignment. */

	for ( i = 0; i < poly->nrings; i++ )
	{
		size += 4; /* Number of points in ring. */
		size += poly->rings[i]->npoints * FLAGS_NDIMS(poly->flags) * sizeof(double);
	}

	LWDEBUGF(3, "polygon size = %d", size);

	return size;
}

static size_t gserialized1_from_lwcircstring_size(const LWCIRCSTRING *curve)
{
	size_t size = 4; /* Type number. */

	assert(curve);

	size += 4; /* Number of points (zero => empty). */
	size += curve->points->npoints * FLAGS_NDIMS(curve->flags) * sizeof(double);

	LWDEBUGF(3, "circstring size = %d", size);

	return size;
}

static size_t gserialized1_from_lwcollection_size(const LWCOLLECTION *col)
{
	size_t size = 4; /* Type number. */
	uint32_t i = 0;

	assert(col);

	size += 4; /* Number of sub-geometries (zero => empty). */

	for ( i = 0; i < col->ngeoms; i++ )
	{
		size_t subsize = gserialized1_from_any_size(col->geoms[i]);
		size += subsize;
		LWDEBUGF(3, "lwcollection subgeom(%d) size = %d", i, subsize);
	}

	LWDEBUGF(3, "lwcollection size = %d", size);

	return size;
}

static size_t gserialized1_from_any_size(const LWGEOM *geom)
{
	LWDEBUGF(2, "Input type: %s", lwtype_name(geom->type));

	switch (geom->type)
	{
	case POINTTYPE:
		return gserialized1_from_lwpoint_size((LWPOINT *)geom);
	case LINETYPE:
		return gserialized1_from_lwline_size((LWLINE *)geom);
	case POLYGONTYPE:
		return gserialized1_from_lwpoly_size((LWPOLY *)geom);
	case TRIANGLETYPE:
		return gserialized1_from_lwtriangle_size((LWTRIANGLE *)geom);
	case CIRCSTRINGTYPE:
		return gserialized1_from_lwcircstring_size((LWCIRCSTRING *)geom);
	case CURVEPOLYTYPE:
	case COMPOUNDTYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTICURVETYPE:
	case MULTIPOLYGONTYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return gserialized1_from_lwcollection_size((LWCOLLECTION *)geom);
	default:
		lwerror("Unknown geometry type: %d - %s", geom->type, lwtype_name(geom->type));
		return 0;
	}
}

/* Public function */

size_t gserialized1_from_lwgeom_size(const LWGEOM *geom)
{
	size_t size = 8; /* Header overhead. */
	assert(geom);

	if (geom->bbox)
		size += gbox_serialized_size(geom->flags);

	size += gserialized1_from_any_size(geom);
	LWDEBUGF(3, "%s size = %d", __func__, size);

	return size;
}

/***********************************************************************
* Serialize an LWGEOM into GSERIALIZED.
*/

/* Private functions */

static size_t gserialized1_from_lwgeom_any(const LWGEOM *geom, uint8_t *buf);

static size_t gserialized1_from_lwpoint(const LWPOINT *point, uint8_t *buf)
{
	uint8_t *loc;
	int ptsize = ptarray_point_size(point->point);
	int type = POINTTYPE;

	assert(point);
	assert(buf);

	if ( FLAGS_GET_ZM(point->flags) != FLAGS_GET_ZM(point->point->flags) )
		lwerror("Dimensions mismatch in lwpoint");

	LWDEBUGF(2, "%s (%p, %p) called", __func__, point, buf);

	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32_t));
	loc += sizeof(uint32_t);
	/* Write in the number of points (0 => empty). */
	memcpy(loc, &(point->point->npoints), sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Copy in the ordinates. */
	if ( point->point->npoints > 0 )
	{
		memcpy(loc, getPoint_internal(point->point, 0), ptsize);
		loc += ptsize;
	}

	return (size_t)(loc - buf);
}

static size_t gserialized1_from_lwline(const LWLINE *line, uint8_t *buf)
{
	uint8_t *loc;
	int ptsize;
	size_t size;
	int type = LINETYPE;

	assert(line);
	assert(buf);

	LWDEBUGF(2, "%s (%p, %p) called", __func__, line, buf);

	if ( FLAGS_GET_Z(line->flags) != FLAGS_GET_Z(line->points->flags) )
		lwerror("Dimensions mismatch in lwline");

	ptsize = ptarray_point_size(line->points);

	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Write in the npoints. */
	memcpy(loc, &(line->points->npoints), sizeof(uint32_t));
	loc += sizeof(uint32_t);

	LWDEBUGF(3, "%s added npoints (%d)", __func__, line->points->npoints);

	/* Copy in the ordinates. */
	if ( line->points->npoints > 0 )
	{
		size = line->points->npoints * ptsize;
		memcpy(loc, getPoint_internal(line->points, 0), size);
		loc += size;
	}
	LWDEBUGF(3, "%s copied serialized_pointlist (%d bytes)", __func__, ptsize * line->points->npoints);

	return (size_t)(loc - buf);
}

static size_t gserialized1_from_lwpoly(const LWPOLY *poly, uint8_t *buf)
{
	uint32_t i;
	uint8_t *loc;
	int ptsize;
	int type = POLYGONTYPE;

	assert(poly);
	assert(buf);

	LWDEBUGF(2, "%s called", __func__);

	ptsize = sizeof(double) * FLAGS_NDIMS(poly->flags);
	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Write in the nrings. */
	memcpy(loc, &(poly->nrings), sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Write in the npoints per ring. */
	for ( i = 0; i < poly->nrings; i++ )
	{
		memcpy(loc, &(poly->rings[i]->npoints), sizeof(uint32_t));
		loc += sizeof(uint32_t);
	}

	/* Add in padding if necessary to remain double aligned. */
	if ( poly->nrings % 2 )
	{
		memset(loc, 0, sizeof(uint32_t));
		loc += sizeof(uint32_t);
	}

	/* Copy in the ordinates. */
	for ( i = 0; i < poly->nrings; i++ )
	{
		POINTARRAY *pa = poly->rings[i];
		size_t pasize;

		if ( FLAGS_GET_ZM(poly->flags) != FLAGS_GET_ZM(pa->flags) )
			lwerror("Dimensions mismatch in lwpoly");

		pasize = pa->npoints * ptsize;
		if ( pa->npoints > 0 )
			memcpy(loc, getPoint_internal(pa, 0), pasize);
		loc += pasize;
	}
	return (size_t)(loc - buf);
}

static size_t gserialized1_from_lwtriangle(const LWTRIANGLE *triangle, uint8_t *buf)
{
	uint8_t *loc;
	int ptsize;
	size_t size;
	int type = TRIANGLETYPE;

	assert(triangle);
	assert(buf);

	LWDEBUGF(2, "%s (%p, %p) called", __func__, triangle, buf);

	if ( FLAGS_GET_ZM(triangle->flags) != FLAGS_GET_ZM(triangle->points->flags) )
		lwerror("Dimensions mismatch in lwtriangle");

	ptsize = ptarray_point_size(triangle->points);

	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Write in the npoints. */
	memcpy(loc, &(triangle->points->npoints), sizeof(uint32_t));
	loc += sizeof(uint32_t);

	LWDEBUGF(3, "%s added npoints (%d)", __func__, triangle->points->npoints);

	/* Copy in the ordinates. */
	if ( triangle->points->npoints > 0 )
	{
		size = triangle->points->npoints * ptsize;
		memcpy(loc, getPoint_internal(triangle->points, 0), size);
		loc += size;
	}
	LWDEBUGF(3, "%s copied serialized_pointlist (%d bytes)", __func__, ptsize * triangle->points->npoints);

	return (size_t)(loc - buf);
}

static size_t gserialized1_from_lwcircstring(const LWCIRCSTRING *curve, uint8_t *buf)
{
	uint8_t *loc;
	int ptsize;
	size_t size;
	int type = CIRCSTRINGTYPE;

	assert(curve);
	assert(buf);

	if (FLAGS_GET_ZM(curve->flags) != FLAGS_GET_ZM(curve->points->flags))
		lwerror("Dimensions mismatch in lwcircstring");


	ptsize = ptarray_point_size(curve->points);
	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Write in the npoints. */
	memcpy(loc, &curve->points->npoints, sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Copy in the ordinates. */
	if ( curve->points->npoints > 0 )
	{
		size = curve->points->npoints * ptsize;
		memcpy(loc, getPoint_internal(curve->points, 0), size);
		loc += size;
	}

	return (size_t)(loc - buf);
}

static size_t gserialized1_from_lwcollection(const LWCOLLECTION *coll, uint8_t *buf)
{
	size_t subsize = 0;
	uint8_t *loc;
	uint32_t i;
	int type;

	assert(coll);
	assert(buf);

	type = coll->type;
	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Write in the number of subgeoms. */
	memcpy(loc, &coll->ngeoms, sizeof(uint32_t));
	loc += sizeof(uint32_t);

	/* Serialize subgeoms. */
	for ( i=0; i<coll->ngeoms; i++ )
	{
		if (FLAGS_GET_ZM(coll->flags) != FLAGS_GET_ZM(coll->geoms[i]->flags))
			lwerror("Dimensions mismatch in lwcollection");
		subsize = gserialized1_from_lwgeom_any(coll->geoms[i], loc);
		loc += subsize;
	}

	return (size_t)(loc - buf);
}

static size_t gserialized1_from_lwgeom_any(const LWGEOM *geom, uint8_t *buf)
{
	assert(geom);
	assert(buf);

	LWDEBUGF(2, "Input type (%d) %s, hasz: %d hasm: %d",
		geom->type, lwtype_name(geom->type),
		FLAGS_GET_Z(geom->flags), FLAGS_GET_M(geom->flags));
	LWDEBUGF(2, "LWGEOM(%p) uint8_t(%p)", geom, buf);

	switch (geom->type)
	{
	case POINTTYPE:
		return gserialized1_from_lwpoint((LWPOINT *)geom, buf);
	case LINETYPE:
		return gserialized1_from_lwline((LWLINE *)geom, buf);
	case POLYGONTYPE:
		return gserialized1_from_lwpoly((LWPOLY *)geom, buf);
	case TRIANGLETYPE:
		return gserialized1_from_lwtriangle((LWTRIANGLE *)geom, buf);
	case CIRCSTRINGTYPE:
		return gserialized1_from_lwcircstring((LWCIRCSTRING *)geom, buf);
	case CURVEPOLYTYPE:
	case COMPOUNDTYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTICURVETYPE:
	case MULTIPOLYGONTYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return gserialized1_from_lwcollection((LWCOLLECTION *)geom, buf);
	default:
		lwerror("Unknown geometry type: %d - %s", geom->type, lwtype_name(geom->type));
		return 0;
	}
	return 0;
}

static size_t gserialized1_from_gbox(const GBOX *gbox, uint8_t *buf)
{
	uint8_t *loc = buf;
	float f;
	size_t return_size;

	assert(buf);

	f = next_float_down(gbox->xmin);
	memcpy(loc, &f, sizeof(float));
	loc += sizeof(float);

	f = next_float_up(gbox->xmax);
	memcpy(loc, &f, sizeof(float));
	loc += sizeof(float);

	f = next_float_down(gbox->ymin);
	memcpy(loc, &f, sizeof(float));
	loc += sizeof(float);

	f = next_float_up(gbox->ymax);
	memcpy(loc, &f, sizeof(float));
	loc += sizeof(float);

	if ( FLAGS_GET_GEODETIC(gbox->flags) )
	{
		f = next_float_down(gbox->zmin);
		memcpy(loc, &f, sizeof(float));
		loc += sizeof(float);

		f = next_float_up(gbox->zmax);
		memcpy(loc, &f, sizeof(float));
		loc += sizeof(float);

		return_size = (size_t)(loc - buf);
		LWDEBUGF(4, "returning size %d", return_size);
		return return_size;
	}

	if ( FLAGS_GET_Z(gbox->flags) )
	{
		f = next_float_down(gbox->zmin);
		memcpy(loc, &f, sizeof(float));
		loc += sizeof(float);

		f = next_float_up(gbox->zmax);
		memcpy(loc, &f, sizeof(float));
		loc += sizeof(float);

	}

	if ( FLAGS_GET_M(gbox->flags) )
	{
		f = next_float_down(gbox->mmin);
		memcpy(loc, &f, sizeof(float));
		loc += sizeof(float);

		f = next_float_up(gbox->mmax);
		memcpy(loc, &f, sizeof(float));
		loc += sizeof(float);
	}
	return_size = (size_t)(loc - buf);
	LWDEBUGF(4, "returning size %d", return_size);
	return return_size;
}

/* Public function */

GSERIALIZED* gserialized1_from_lwgeom(LWGEOM *geom, size_t *size)
{
	size_t expected_size = 0;
	size_t return_size = 0;
	uint8_t *serialized = NULL;
	uint8_t *ptr = NULL;
	GSERIALIZED *g = NULL;
	assert(geom);

	/*
	** See if we need a bounding box, add one if we don't have one.
	*/
	if ( (! geom->bbox) && lwgeom_needs_bbox(geom) && (!lwgeom_is_empty(geom)) )
	{
		lwgeom_add_bbox(geom);
	}

	/*
	** Harmonize the flags to the state of the lwgeom
	*/
	if ( geom->bbox )
		FLAGS_SET_BBOX(geom->flags, 1);
	else
		FLAGS_SET_BBOX(geom->flags, 0);

	/* Set up the uint8_t buffer into which we are going to write the serialized geometry. */
	expected_size = gserialized1_from_lwgeom_size(geom);
	serialized = lwalloc(expected_size);
	ptr = serialized;

	/* Move past size, srid and flags. */
	ptr += 8;

	/* Write in the serialized form of the gbox, if necessary. */
	if ( geom->bbox )
		ptr += gserialized1_from_gbox(geom->bbox, ptr);

	/* Write in the serialized form of the geometry. */
	ptr += gserialized1_from_lwgeom_any(geom, ptr);

	/* Calculate size as returned by data processing functions. */
	return_size = ptr - serialized;

	if ( expected_size != return_size ) /* Uh oh! */
	{
		lwerror("Return size (%d) not equal to expected size (%d)!", return_size, expected_size);
		return NULL;
	}

	if ( size ) /* Return the output size to the caller if necessary. */
		*size = return_size;

	g = (GSERIALIZED*)serialized;

	/*
	** We are aping PgSQL code here, PostGIS code should use
	** VARSIZE to set this for real.
	*/
	g->size = return_size << 2;

	/* Set the SRID! */
	gserialized1_set_srid(g, geom->srid);

	g->gflags = lwflags_get_g1flags(geom->flags);

	return g;
}

/***********************************************************************
* De-serialize GSERIALIZED into an LWGEOM.
*/

static LWGEOM* lwgeom_from_gserialized1_buffer(uint8_t *data_ptr, lwflags_t lwflags, size_t *size);

static LWPOINT* lwpoint_from_gserialized1_buffer(uint8_t *data_ptr, lwflags_t lwflags, size_t *size)
{
	uint8_t *start_ptr = data_ptr;
	LWPOINT *point;
	uint32_t npoints = 0;

	assert(data_ptr);

	point = (LWPOINT*)lwalloc(sizeof(LWPOINT));
	point->srid = SRID_UNKNOWN; /* Default */
	point->bbox = NULL;
	point->type = POINTTYPE;
	point->flags = lwflags;

	data_ptr += 4; /* Skip past the type. */
	npoints = gserialized1_get_uint32_t(data_ptr); /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
		point->point = ptarray_construct_reference_data(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), 1, data_ptr);
	else
		point->point = ptarray_construct(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), 0); /* Empty point */

	data_ptr += npoints * FLAGS_NDIMS(lwflags) * sizeof(double);

	if ( size )
		*size = data_ptr - start_ptr;

	return point;
}

static LWLINE* lwline_from_gserialized1_buffer(uint8_t *data_ptr, lwflags_t lwflags, size_t *size)
{
	uint8_t *start_ptr = data_ptr;
	LWLINE *line;
	uint32_t npoints = 0;

	assert(data_ptr);

	line = (LWLINE*)lwalloc(sizeof(LWLINE));
	line->srid = SRID_UNKNOWN; /* Default */
	line->bbox = NULL;
	line->type = LINETYPE;
	line->flags = lwflags;

	data_ptr += 4; /* Skip past the type. */
	npoints = gserialized1_get_uint32_t(data_ptr); /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
		line->points = ptarray_construct_reference_data(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), npoints, data_ptr);

	else
		line->points = ptarray_construct(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), 0); /* Empty linestring */

	data_ptr += FLAGS_NDIMS(lwflags) * npoints * sizeof(double);

	if ( size )
		*size = data_ptr - start_ptr;

	return line;
}

static LWPOLY* lwpoly_from_gserialized1_buffer(uint8_t *data_ptr, lwflags_t lwflags, size_t *size)
{
	uint8_t *start_ptr = data_ptr;
	LWPOLY *poly;
	uint8_t *ordinate_ptr;
	uint32_t nrings = 0;
	uint32_t i = 0;

	assert(data_ptr);

	poly = (LWPOLY*)lwalloc(sizeof(LWPOLY));
	poly->srid = SRID_UNKNOWN; /* Default */
	poly->bbox = NULL;
	poly->type = POLYGONTYPE;
	poly->flags = lwflags;

	data_ptr += 4; /* Skip past the polygontype. */
	nrings = gserialized1_get_uint32_t(data_ptr); /* Zero => empty geometry */
	poly->nrings = nrings;
	LWDEBUGF(4, "nrings = %d", nrings);
	data_ptr += 4; /* Skip past the nrings. */

	ordinate_ptr = data_ptr; /* Start the ordinate pointer. */
	if ( nrings > 0)
	{
		poly->rings = (POINTARRAY**)lwalloc( sizeof(POINTARRAY*) * nrings );
		poly->maxrings = nrings;
		ordinate_ptr += nrings * 4; /* Move past all the npoints values. */
		if ( nrings % 2 ) /* If there is padding, move past that too. */
			ordinate_ptr += 4;
	}
	else /* Empty polygon */
	{
		poly->rings = NULL;
		poly->maxrings = 0;
	}

	for ( i = 0; i < nrings; i++ )
	{
		uint32_t npoints = 0;

		/* Read in the number of points. */
		npoints = gserialized1_get_uint32_t(data_ptr);
		data_ptr += 4;

		/* Make a point array for the ring, and move the ordinate pointer past the ring ordinates. */
		poly->rings[i] = ptarray_construct_reference_data(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), npoints, ordinate_ptr);

		ordinate_ptr += sizeof(double) * FLAGS_NDIMS(lwflags) * npoints;
	}

	if ( size )
		*size = ordinate_ptr - start_ptr;

	return poly;
}

static LWTRIANGLE* lwtriangle_from_gserialized1_buffer(uint8_t *data_ptr, lwflags_t lwflags, size_t *size)
{
	uint8_t *start_ptr = data_ptr;
	LWTRIANGLE *triangle;
	uint32_t npoints = 0;

	assert(data_ptr);

	triangle = (LWTRIANGLE*)lwalloc(sizeof(LWTRIANGLE));
	triangle->srid = SRID_UNKNOWN; /* Default */
	triangle->bbox = NULL;
	triangle->type = TRIANGLETYPE;
	triangle->flags = lwflags;

	data_ptr += 4; /* Skip past the type. */
	npoints = gserialized1_get_uint32_t(data_ptr); /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
		triangle->points = ptarray_construct_reference_data(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), npoints, data_ptr);
	else
		triangle->points = ptarray_construct(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), 0); /* Empty triangle */

	data_ptr += FLAGS_NDIMS(lwflags) * npoints * sizeof(double);

	if ( size )
		*size = data_ptr - start_ptr;

	return triangle;
}

static LWCIRCSTRING* lwcircstring_from_gserialized1_buffer(uint8_t *data_ptr, lwflags_t lwflags, size_t *size)
{
	uint8_t *start_ptr = data_ptr;
	LWCIRCSTRING *circstring;
	uint32_t npoints = 0;

	assert(data_ptr);

	circstring = (LWCIRCSTRING*)lwalloc(sizeof(LWCIRCSTRING));
	circstring->srid = SRID_UNKNOWN; /* Default */
	circstring->bbox = NULL;
	circstring->type = CIRCSTRINGTYPE;
	circstring->flags = lwflags;

	data_ptr += 4; /* Skip past the circstringtype. */
	npoints = gserialized1_get_uint32_t(data_ptr); /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
		circstring->points = ptarray_construct_reference_data(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), npoints, data_ptr);
	else
		circstring->points = ptarray_construct(FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), 0); /* Empty circularstring */

	data_ptr += FLAGS_NDIMS(lwflags) * npoints * sizeof(double);

	if ( size )
		*size = data_ptr - start_ptr;

	return circstring;
}

static LWCOLLECTION* lwcollection_from_gserialized1_buffer(uint8_t *data_ptr, lwflags_t lwflags, size_t *size)
{
	uint32_t type;
	uint8_t *start_ptr = data_ptr;
	LWCOLLECTION *collection;
	uint32_t ngeoms = 0;
	uint32_t i = 0;

	assert(data_ptr);

	type = gserialized1_get_uint32_t(data_ptr);
	data_ptr += 4; /* Skip past the type. */

	collection = (LWCOLLECTION*)lwalloc(sizeof(LWCOLLECTION));
	collection->srid = SRID_UNKNOWN; /* Default */
	collection->bbox = NULL;
	collection->type = type;
	collection->flags = lwflags;

	ngeoms = gserialized1_get_uint32_t(data_ptr);
	collection->ngeoms = ngeoms; /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the ngeoms. */

	if ( ngeoms > 0 )
	{
		collection->geoms = lwalloc(sizeof(LWGEOM*) * ngeoms);
		collection->maxgeoms = ngeoms;
	}
	else
	{
		collection->geoms = NULL;
		collection->maxgeoms = 0;
	}

	/* Sub-geometries are never de-serialized with boxes (#1254) */
	FLAGS_SET_BBOX(lwflags, 0);

	for ( i = 0; i < ngeoms; i++ )
	{
		uint32_t subtype = gserialized1_get_uint32_t(data_ptr);
		size_t subsize = 0;

		if ( ! lwcollection_allows_subtype(type, subtype) )
		{
			lwerror("Invalid subtype (%s) for collection type (%s)", lwtype_name(subtype), lwtype_name(type));
			lwfree(collection);
			return NULL;
		}
		collection->geoms[i] = lwgeom_from_gserialized1_buffer(data_ptr, lwflags, &subsize);
		data_ptr += subsize;
	}

	if ( size )
		*size = data_ptr - start_ptr;

	return collection;
}

LWGEOM* lwgeom_from_gserialized1_buffer(uint8_t *data_ptr, lwflags_t lwflags, size_t *g_size)
{
	uint32_t type;

	assert(data_ptr);

	type = gserialized1_get_uint32_t(data_ptr);

	LWDEBUGF(2, "Got type %d (%s), hasz=%d hasm=%d geodetic=%d hasbox=%d", type, lwtype_name(type),
		FLAGS_GET_Z(lwflags), FLAGS_GET_M(lwflags), FLAGS_GET_GEODETIC(lwflags), FLAGS_GET_BBOX(lwflags));

	switch (type)
	{
	case POINTTYPE:
		return (LWGEOM *)lwpoint_from_gserialized1_buffer(data_ptr, lwflags, g_size);
	case LINETYPE:
		return (LWGEOM *)lwline_from_gserialized1_buffer(data_ptr, lwflags, g_size);
	case CIRCSTRINGTYPE:
		return (LWGEOM *)lwcircstring_from_gserialized1_buffer(data_ptr, lwflags, g_size);
	case POLYGONTYPE:
		return (LWGEOM *)lwpoly_from_gserialized1_buffer(data_ptr, lwflags, g_size);
	case TRIANGLETYPE:
		return (LWGEOM *)lwtriangle_from_gserialized1_buffer(data_ptr, lwflags, g_size);
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM *)lwcollection_from_gserialized1_buffer(data_ptr, lwflags, g_size);
	default:
		lwerror("Unknown geometry type: %d - %s", type, lwtype_name(type));
		return NULL;
	}
}

LWGEOM* lwgeom_from_gserialized1(const GSERIALIZED *g)
{
	lwflags_t lwflags = 0;
	int32_t srid = 0;
	uint32_t lwtype = 0;
	uint8_t *data_ptr = NULL;
	LWGEOM *lwgeom = NULL;
	GBOX bbox;
	size_t size = 0;

	assert(g);

	srid = gserialized1_get_srid(g);
	lwtype = gserialized1_get_type(g);
	lwflags = gserialized1_get_lwflags(g);

	LWDEBUGF(4, "Got type %d (%s), srid=%d", lwtype, lwtype_name(lwtype), srid);

	data_ptr = (uint8_t*)g->data;
	if (FLAGS_GET_BBOX(lwflags))
		data_ptr += gbox_serialized_size(lwflags);

	lwgeom = lwgeom_from_gserialized1_buffer(data_ptr, lwflags, &size);

	if ( ! lwgeom )
		lwerror("%s: unable create geometry", __func__); /* Ooops! */

	lwgeom->type = lwtype;
	lwgeom->flags = lwflags;

	if ( gserialized1_read_gbox_p(g, &bbox) == LW_SUCCESS )
	{
		lwgeom->bbox = gbox_copy(&bbox);
	}
	else if ( lwgeom_needs_bbox(lwgeom) && (lwgeom_calculate_gbox(lwgeom, &bbox) == LW_SUCCESS) )
	{
		lwgeom->bbox = gbox_copy(&bbox);
	}
	else
	{
		lwgeom->bbox = NULL;
	}

	lwgeom_set_srid(lwgeom, srid);

	return lwgeom;
}

const float * gserialized1_get_float_box_p(const GSERIALIZED *g, size_t *ndims)
{
	if (ndims)
		*ndims = G1FLAGS_NDIMS_BOX(g->gflags);
	if (!g) return NULL;
	if (!G1FLAGS_GET_BBOX(g->gflags)) return NULL;
	return (const float *)(g->data);
}

/**
* Update the bounding box of a #GSERIALIZED, allocating a fresh one
* if there is not enough space to just write the new box in.
* <em>WARNING</em> if a new object needs to be created, the
* input pointer will have to be freed by the caller! Check
* to see if input == output. Returns null if there's a problem
* like mismatched dimensions.
*/
GSERIALIZED* gserialized1_set_gbox(GSERIALIZED *g, GBOX *gbox)
{

	int g_ndims = G1FLAGS_NDIMS_BOX(g->gflags);
	int box_ndims = FLAGS_NDIMS_BOX(gbox->flags);
	GSERIALIZED *g_out = NULL;
	size_t box_size = 2 * g_ndims * sizeof(float);
	float *fbox;
	int fbox_pos = 0;

	/* The dimensionality of the inputs has to match or we are SOL. */
	if ( g_ndims != box_ndims )
	{
		return NULL;
	}

	/* Serialized already has room for a box. */
	if (G1FLAGS_GET_BBOX(g->gflags))
	{
		g_out = g;
	}
	/* Serialized has no box. We need to allocate enough space for the old
	   data plus the box, and leave a gap in the memory segment to write
	   the new values into.
	*/
	else
	{
		size_t varsize_new = LWSIZE_GET(g->size) + box_size;
		uint8_t *ptr;
		g_out = lwalloc(varsize_new);
		/* Copy the head of g into place */
		memcpy(g_out, g, 8);
		/* Copy the body of g into place after leaving space for the box */
		ptr = g_out->data;
		ptr += box_size;
		memcpy(ptr, g->data, LWSIZE_GET(g->size) - 8);
		G1FLAGS_SET_BBOX(g_out->gflags, 1);
		LWSIZE_SET(g_out->size, varsize_new);
	}

	/* Move bounds to nearest float values */
	gbox_float_round(gbox);
	/* Now write the float box values into the memory segement */
	fbox = (float*)(g_out->data);
	/* Copy in X/Y */
	fbox[fbox_pos++] = gbox->xmin;
	fbox[fbox_pos++] = gbox->xmax;
	fbox[fbox_pos++] = gbox->ymin;
	fbox[fbox_pos++] = gbox->ymax;
	/* Optionally copy in higher dims */
	if(gserialized1_has_z(g) || gserialized1_is_geodetic(g))
	{
		fbox[fbox_pos++] = gbox->zmin;
		fbox[fbox_pos++] = gbox->zmax;
	}
	if(gserialized1_has_m(g) && ! gserialized1_is_geodetic(g))
	{
		fbox[fbox_pos++] = gbox->mmin;
		fbox[fbox_pos++] = gbox->mmax;
	}

	return g_out;
}


/**
* Remove the bounding box from a #GSERIALIZED. Returns a freshly
* allocated #GSERIALIZED every time.
*/
GSERIALIZED* gserialized1_drop_gbox(GSERIALIZED *g)
{
	int g_ndims = G1FLAGS_NDIMS_BOX(g->gflags);
	size_t box_size = 2 * g_ndims * sizeof(float);
	size_t g_out_size = LWSIZE_GET(g->size) - box_size;
	GSERIALIZED *g_out = lwalloc(g_out_size);

	/* Copy the contents while omitting the box */
	if ( G1FLAGS_GET_BBOX(g->gflags) )
	{
		uint8_t *outptr = (uint8_t*)g_out;
		uint8_t *inptr = (uint8_t*)g;
		/* Copy the header (size+type) of g into place */
		memcpy(outptr, inptr, 8);
		outptr += 8;
		inptr += 8 + box_size;
		/* Copy parts after the box into place */
		memcpy(outptr, inptr, g_out_size - 8);
		G1FLAGS_SET_BBOX(g_out->gflags, 0);
		LWSIZE_SET(g_out->size, g_out_size);
	}
	/* No box? Nothing to do but copy and return. */
	else
	{
		memcpy(g_out, g, g_out_size);
	}

	return g_out;
}

