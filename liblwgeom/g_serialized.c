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

#include "liblwgeom_internal.h"

/***********************************************************************
* GSERIALIZED metadata utility functions.
*/

uint32 gserialized_get_type(const GSERIALIZED *s)
{
	uint32 *ptr;
	assert(s);
	ptr = (uint32*)(s->data);
	ptr += (gbox_serialized_size(s->flags) / sizeof(uint32));
	return *ptr;
}

uint32 gserialized_get_srid(const GSERIALIZED *s)
{
	uint32 srid = 0;
	srid = srid | (s->srid[0] << 16);
	srid = srid | (s->srid[1] << 8);
	srid = srid | s->srid[2];
	return srid;
}

void gserialized_set_srid(GSERIALIZED *s, uint32 srid)
{
	LWDEBUGF(3, "Called with srid = %d", srid);
	s->srid[0] = (srid & 0x000F0000) >> 16;
	s->srid[1] = (srid & 0x0000FF00) >> 8;
	s->srid[2] = (srid & 0x000000FF);
}


/***********************************************************************
* Calculate the GSERIALIZED size for an LWGEOM.
*/

/* Private functions */

static size_t gserialized_from_any_size(const LWGEOM *geom); /* Local prototype */

static size_t gserialized_from_lwpoint_size(const LWPOINT *point)
{
	size_t size = 4; /* Type number. */

	assert(point);

	size += 4; /* Number of points (one or zero (empty)). */
	size += point->point->npoints * FLAGS_NDIMS(point->flags) * sizeof(double);

	LWDEBUGF(3, "point size = %d", size);

	return size;
}

static size_t gserialized_from_lwline_size(const LWLINE *line)
{
	size_t size = 4; /* Type number. */

	assert(line);

	size += 4; /* Number of points (zero => empty). */
	size += line->points->npoints * FLAGS_NDIMS(line->flags) * sizeof(double);

	LWDEBUGF(3, "linestring size = %d", size);

	return size;
}

static size_t gserialized_from_lwtriangle_size(const LWTRIANGLE *triangle)
{
	size_t size = 4; /* Type number. */

	assert(triangle);

	size += 4; /* Number of points (zero => empty). */
	size += triangle->points->npoints * FLAGS_NDIMS(triangle->flags) * sizeof(double);

	LWDEBUGF(3, "triangle size = %d", size);

	return size;
}

static size_t gserialized_from_lwpoly_size(const LWPOLY *poly)
{
	size_t size = 4; /* Type number. */
	int i = 0;

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

static size_t gserialized_from_lwcircstring_size(const LWCIRCSTRING *curve)
{
	size_t size = 4; /* Type number. */

	assert(curve);

	size += 4; /* Number of points (zero => empty). */
	size += curve->points->npoints * FLAGS_NDIMS(curve->flags) * sizeof(double);

	LWDEBUGF(3, "circstring size = %d", size);

	return size;
}

static size_t gserialized_from_lwcollection_size(const LWCOLLECTION *col)
{
	size_t size = 4; /* Type number. */
	int i = 0;

	assert(col);

	size += 4; /* Number of sub-geometries (zero => empty). */

	for ( i = 0; i < col->ngeoms; i++ )
	{
		size_t subsize = gserialized_from_any_size(col->geoms[i]);
		size += subsize;
		LWDEBUGF(3, "lwcollection subgeom(%d) size = %d", i, subsize);
	}

	LWDEBUGF(3, "lwcollection size = %d", size);

	return size;
}

static size_t gserialized_from_any_size(const LWGEOM *geom)
{
	LWDEBUGF(2, "Input type: %s", lwtype_name(geom->type));

	switch (geom->type)
	{
	case POINTTYPE:
		return gserialized_from_lwpoint_size((LWPOINT *)geom);
	case LINETYPE:
		return gserialized_from_lwline_size((LWLINE *)geom);
	case POLYGONTYPE:
		return gserialized_from_lwpoly_size((LWPOLY *)geom);
	case TRIANGLETYPE:
		return gserialized_from_lwtriangle_size((LWTRIANGLE *)geom);
	case CIRCSTRINGTYPE:
		return gserialized_from_lwcircstring_size((LWCIRCSTRING *)geom);
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
		return gserialized_from_lwcollection_size((LWCOLLECTION *)geom);
	default:
		lwerror("Unknown geometry type: %d - %s", geom->type, lwtype_name(geom->type));
		return 0;
	}
}

/* Public function */

size_t gserialized_from_lwgeom_size(const LWGEOM *geom)
{
	size_t size = 8; /* Header overhead. */
	assert(geom);
	size += gserialized_from_any_size(geom);
	LWDEBUGF(3, "g_serialize size = %d", size);
	return size;
}

/***********************************************************************
* Serialize an LWGEOM into GSERIALIZED.
*/

/* Private functions */

static size_t gserialized_from_lwgeom_any(const LWGEOM *geom, uchar *buf);

static size_t gserialized_from_lwpoint(const LWPOINT *point, uchar *buf)
{
	uchar *loc;
	int ptsize = ptarray_point_size(point->point);
	int type = POINTTYPE;

	assert(point);
	assert(buf);

	if ( FLAGS_GET_ZM(point->flags) != FLAGS_GET_ZM(point->point->flags) )
		lwerror("Dimensions mismatch in lwpoint");

	LWDEBUGF(2, "lwpoint_to_gserialized(%p, %p) called", point, buf);

	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32));
	loc += sizeof(uint32);
	/* Write in the number of points (0 => empty). */
	memcpy(loc, &(point->point->npoints), sizeof(uint32));
	loc += sizeof(uint32);

	/* Copy in the ordinates. */
	if ( point->point->npoints > 0 )
	{
		memcpy(loc, getPoint_internal(point->point, 0), ptsize);
		loc += ptsize;
	}

	return (size_t)(loc - buf);
}

static size_t gserialized_from_lwline(const LWLINE *line, uchar *buf)
{
	uchar *loc;
	int ptsize;
	size_t size;
	int type = LINETYPE;

	assert(line);
	assert(buf);

	LWDEBUGF(2, "lwline_to_gserialized(%p, %p) called", line, buf);

	if ( FLAGS_GET_Z(line->flags) != FLAGS_GET_Z(line->points->flags) )
		lwerror("Dimensions mismatch in lwline");

	ptsize = ptarray_point_size(line->points);

	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32));
	loc += sizeof(uint32);

	/* Write in the npoints. */
	memcpy(loc, &(line->points->npoints), sizeof(uint32));
	loc += sizeof(uint32);

	LWDEBUGF(3, "lwline_to_gserialized added npoints (%d)", line->points->npoints);

	/* Copy in the ordinates. */
	if ( line->points->npoints > 0 )
	{
		size = line->points->npoints * ptsize;
		memcpy(loc, getPoint_internal(line->points, 0), size);
		loc += size;
	}
	LWDEBUGF(3, "lwline_to_gserialized copied serialized_pointlist (%d bytes)", ptsize * line->points->npoints);

	return (size_t)(loc - buf);
}

static size_t gserialized_from_lwpoly(const LWPOLY *poly, uchar *buf)
{
	int i;
	uchar *loc;
	int ptsize;
	int type = POLYGONTYPE;

	assert(poly);
	assert(buf);

	LWDEBUG(2, "lwpoly_to_gserialized called");

	ptsize = sizeof(double) * FLAGS_NDIMS(poly->flags);
	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32));
	loc += sizeof(uint32);

	/* Write in the nrings. */
	memcpy(loc, &(poly->nrings), sizeof(uint32));
	loc += sizeof(uint32);

	/* Write in the npoints per ring. */
	for ( i = 0; i < poly->nrings; i++ )
	{
		memcpy(loc, &(poly->rings[i]->npoints), sizeof(uint32));
		loc += sizeof(uint32);
	}

	/* Add in padding if necessary to remain double aligned. */
	if ( poly->nrings % 2 )
	{
		loc += sizeof(uint32);
	}

	/* Copy in the ordinates. */
	for ( i = 0; i < poly->nrings; i++ )
	{
		POINTARRAY *pa = poly->rings[i];
		size_t pasize;

		if ( FLAGS_GET_ZM(poly->flags) != FLAGS_GET_ZM(pa->flags) )
			lwerror("Dimensions mismatch in lwpoly");

		pasize = pa->npoints * ptsize;
		memcpy(loc, getPoint_internal(pa, 0), pasize);
		loc += pasize;
	}
	return (size_t)(loc - buf);
}

static size_t gserialized_from_lwtriangle(const LWTRIANGLE *triangle, uchar *buf)
{
	uchar *loc;
	int ptsize;
	size_t size;
	int type = TRIANGLETYPE;

	assert(triangle);
	assert(buf);

	LWDEBUGF(2, "lwtriangle_to_gserialized(%p, %p) called", triangle, buf);

	if ( FLAGS_GET_ZM(triangle->flags) != FLAGS_GET_ZM(triangle->points->flags) )
		lwerror("Dimensions mismatch in lwtriangle");

	ptsize = ptarray_point_size(triangle->points);

	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32));
	loc += sizeof(uint32);

	/* Write in the npoints. */
	memcpy(loc, &(triangle->points->npoints), sizeof(uint32));
	loc += sizeof(uint32);

	LWDEBUGF(3, "lwtriangle_to_gserialized added npoints (%d)", triangle->points->npoints);

	/* Copy in the ordinates. */
	if ( triangle->points->npoints > 0 )
	{
		size = triangle->points->npoints * ptsize;
		memcpy(loc, getPoint_internal(triangle->points, 0), size);
		loc += size;
	}
	LWDEBUGF(3, "lwtriangle_to_gserialized copied serialized_pointlist (%d bytes)", ptsize * triangle->points->npoints);

	return (size_t)(loc - buf);
}

static size_t gserialized_from_lwcircstring(const LWCIRCSTRING *curve, uchar *buf)
{
	uchar *loc;
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
	memcpy(loc, &type, sizeof(uint32));
	loc += sizeof(uint32);

	/* Write in the npoints. */
	memcpy(loc, &curve->points->npoints, sizeof(uint32));
	loc += sizeof(uint32);

	/* Copy in the ordinates. */
	if ( curve->points->npoints > 0 )
	{
		size = curve->points->npoints * ptsize;
		memcpy(loc, getPoint_internal(curve->points, 0), size);
		loc += size;
	}

	return (size_t)(loc - buf);
}

static size_t gserialized_from_lwcollection(const LWCOLLECTION *coll, uchar *buf)
{
	size_t subsize = 0;
	uchar *loc;
	int i;
	int type;

	assert(coll);
	assert(buf);

	type = coll->type;
	loc = buf;

	/* Write in the type. */
	memcpy(loc, &type, sizeof(uint32));
	loc += sizeof(uint32);

	/* Write in the number of subgeoms. */
	memcpy(loc, &coll->ngeoms, sizeof(uint32));
	loc += sizeof(uint32);

	/* Serialize subgeoms. */
	for ( i=0; i<coll->ngeoms; i++ )
	{
		if (FLAGS_GET_ZM(coll->flags) != FLAGS_GET_ZM(coll->geoms[i]->flags))
			lwerror("Dimensions mismatch in lwcollection");
		subsize = gserialized_from_lwgeom_any(coll->geoms[i], loc);
		loc += subsize;
	}

	return (size_t)(loc - buf);
}

static size_t gserialized_from_lwgeom_any(const LWGEOM *geom, uchar *buf)
{
	assert(geom);
	assert(buf);

	LWDEBUGF(2, "Input type (%d) %s, hasz: %d hasm: %d",
		geom->type, lwtype_name(geom->type),
		FLAGS_GET_Z(geom->flags), FLAGS_GET_M(geom->flags));
	LWDEBUGF(2, "LWGEOM(%p) uchar(%p)", geom, buf);

	switch (geom->type)
	{
	case POINTTYPE:
		return gserialized_from_lwpoint((LWPOINT *)geom, buf);
	case LINETYPE:
		return gserialized_from_lwline((LWLINE *)geom, buf);
	case POLYGONTYPE:
		return gserialized_from_lwpoly((LWPOLY *)geom, buf);
	case TRIANGLETYPE:
		return gserialized_from_lwtriangle((LWTRIANGLE *)geom, buf);
	case CIRCSTRINGTYPE:
		return gserialized_from_lwcircstring((LWCIRCSTRING *)geom, buf);
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
		return gserialized_from_lwcollection((LWCOLLECTION *)geom, buf);
	default:
		lwerror("Unknown geometry type: %d - %s", geom->type, lwtype_name(geom->type));
		return 0;
	}
	return 0;
}

static size_t gserialized_from_gbox(const GBOX *gbox, uchar *buf)
{
	uchar *loc;
	float f;
	size_t return_size;

	assert(buf);

	loc = buf;

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

GSERIALIZED* gserialized_from_lwgeom(const LWGEOM *geom, int is_geodetic, size_t *size)
{
	size_t expected_box_size = 0;
	size_t expected_size = 0;
	size_t return_size = 0;
	uchar *serialized = NULL;
	uchar *ptr = NULL;
	GSERIALIZED *g = NULL;
	GBOX gbox;
	assert(geom);

	gbox.flags = gflags(FLAGS_GET_Z(geom->flags), FLAGS_GET_M(geom->flags), is_geodetic);

	/*
	** We need room for a bounding box in the serialized form.
	** Calculate the box and allocate enough size for it.
	*/
	if ( ! lwgeom_is_empty(geom) && lwgeom_needs_bbox(geom) )
	{
		int result = LW_SUCCESS;
		LWDEBUG(3, "calculating bbox");
		if ( is_geodetic )
			result = lwgeom_calculate_gbox_geodetic(geom, &gbox);
		else
			result = lwgeom_calculate_gbox(geom, &gbox);
		if ( result == LW_SUCCESS )
		{
			FLAGS_SET_BBOX(gbox.flags, 1);
			expected_box_size = gbox_serialized_size(gbox.flags);
		}
	}

	/* Set up the uchar buffer into which we are going to write the serialized geometry. */
	expected_size = gserialized_from_lwgeom_size(geom) + expected_box_size;
	serialized = lwalloc(expected_size);
	ptr = serialized;

	/* Move past size, srid and flags. */
	ptr += 8;

	/* Write in the serialized form of the gbox, if necessary. */
	if ( FLAGS_GET_BBOX(gbox.flags) )
		ptr += gserialized_from_gbox(&gbox, ptr);

	/* Write in the serialized form of the geometry. */
	ptr += gserialized_from_lwgeom_any(geom, ptr);

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

	if ( geom->srid == 0 || geom->srid == (uint32)(-1) ) /* Zero is the no-SRID value now. */
		gserialized_set_srid(g, 0);
	else
		gserialized_set_srid(g, geom->srid);

	g->flags = gbox.flags;

	return g;
}

/***********************************************************************
* De-serialize GSERIALIZED into an LWGEOM.
*/

static LWGEOM* lwgeom_from_gserialized_buffer(uchar *data_ptr, uchar g_flags, size_t *g_size);

static LWPOINT* lwpoint_from_gserialized_buffer(uchar *data_ptr, uchar g_flags, size_t *g_size)
{
	uchar *start_ptr = data_ptr;
	LWPOINT *point;
	uint32 npoints = 0;

	assert(data_ptr);

	point = (LWPOINT*)lwalloc(sizeof(LWPOINT));
	point->srid = -1; /* Default */
	point->bbox = NULL;
	point->type = POINTTYPE;
	point->flags = g_flags;

	data_ptr += 4; /* Skip past the type. */
	npoints = lw_get_uint32(data_ptr); /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
		point->point = ptarray_construct_reference_data(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), 1, data_ptr);
	else
		point->point = ptarray_construct(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), 0); /* Empty point */

	data_ptr += npoints * FLAGS_NDIMS(g_flags) * sizeof(double);

	if ( g_size )
		*g_size = data_ptr - start_ptr;

	return point;
}

static LWLINE* lwline_from_gserialized_buffer(uchar *data_ptr, uchar g_flags, size_t *g_size)
{
	uchar *start_ptr = data_ptr;
	LWLINE *line;
	uint32 npoints = 0;

	assert(data_ptr);

	line = (LWLINE*)lwalloc(sizeof(LWLINE));
	line->srid = -1; /* Default */
	line->bbox = NULL;
	line->type = LINETYPE;
	line->flags = g_flags;

	data_ptr += 4; /* Skip past the type. */
	npoints = lw_get_uint32(data_ptr); /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
		line->points = ptarray_construct_reference_data(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), npoints, data_ptr);
		
	else
		line->points = ptarray_construct(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), 0); /* Empty linestring */

	data_ptr += FLAGS_NDIMS(g_flags) * npoints * sizeof(double);

	if ( g_size )
		*g_size = data_ptr - start_ptr;

	return line;
}

static LWPOLY* lwpoly_from_gserialized_buffer(uchar *data_ptr, uchar g_flags, size_t *g_size)
{
	uchar *start_ptr = data_ptr;
	LWPOLY *poly;
	uchar *ordinate_ptr;
	uint32 nrings = 0;
	int i = 0;

	assert(data_ptr);

	poly = (LWPOLY*)lwalloc(sizeof(LWPOLY));
	poly->srid = -1; /* Default */
	poly->bbox = NULL;
	poly->type = POLYGONTYPE;
	poly->flags = g_flags;

	data_ptr += 4; /* Skip past the polygontype. */
	nrings = lw_get_uint32(data_ptr); /* Zero => empty geometry */
	poly->nrings = nrings;
	LWDEBUGF(4, "nrings = %d", nrings);
	data_ptr += 4; /* Skip past the nrings. */

	ordinate_ptr = data_ptr; /* Start the ordinate pointer. */
	if ( nrings > 0)
	{
		poly->rings = (POINTARRAY**)lwalloc( sizeof(POINTARRAY*) * nrings );
		ordinate_ptr += nrings * 4; /* Move past all the npoints values. */
		if ( nrings % 2 ) /* If there is padding, move past that too. */
			ordinate_ptr += 4;
	}
	else /* Empty polygon */
	{
		poly->rings = NULL;
	}

	for ( i = 0; i < nrings; i++ )
	{
		uint32 npoints = 0;

		/* Read in the number of points. */
		npoints = lw_get_uint32(data_ptr);
		data_ptr += 4;

		/* Make a point array for the ring, and move the ordinate pointer past the ring ordinates. */
		poly->rings[i] = ptarray_construct_reference_data(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), npoints, ordinate_ptr);
		
		ordinate_ptr += sizeof(double) * FLAGS_NDIMS(g_flags) * npoints;
	}

	if ( g_size )
		*g_size = ordinate_ptr - start_ptr;

	return poly;
}

static LWTRIANGLE* lwtriangle_from_gserialized_buffer(uchar *data_ptr, uchar g_flags, size_t *g_size)
{
	uchar *start_ptr = data_ptr;
	LWTRIANGLE *triangle;
	uint32 npoints = 0;

	assert(data_ptr);

	triangle = (LWTRIANGLE*)lwalloc(sizeof(LWTRIANGLE));
	triangle->srid = -1; /* Default */
	triangle->bbox = NULL;
	triangle->type = TRIANGLETYPE;
	triangle->flags = g_flags;

	data_ptr += 4; /* Skip past the type. */
	npoints = lw_get_uint32(data_ptr); /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
		triangle->points = ptarray_construct_reference_data(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), npoints, data_ptr);		
	else
		triangle->points = ptarray_construct(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), 0); /* Empty triangle */

	data_ptr += FLAGS_NDIMS(g_flags) * npoints * sizeof(double);

	if ( g_size )
		*g_size = data_ptr - start_ptr;

	return triangle;
}

static LWCIRCSTRING* lwcircstring_from_gserialized_buffer(uchar *data_ptr, uchar g_flags, size_t *g_size)
{
	uchar *start_ptr = data_ptr;
	LWCIRCSTRING *circstring;
	uint32 npoints = 0;

	assert(data_ptr);

	circstring = (LWCIRCSTRING*)lwalloc(sizeof(LWCIRCSTRING));
	circstring->srid = -1; /* Default */
	circstring->bbox = NULL;
	circstring->type = CIRCSTRINGTYPE;
	circstring->flags = g_flags;

	data_ptr += 4; /* Skip past the circstringtype. */
	npoints = lw_get_uint32(data_ptr); /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the npoints. */

	if ( npoints > 0 )
		circstring->points = ptarray_construct_reference_data(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), npoints, data_ptr);		
	else
		circstring->points = ptarray_construct(FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags), 0); /* Empty circularstring */

	data_ptr += FLAGS_NDIMS(g_flags) * npoints * sizeof(double);

	if ( g_size )
		*g_size = data_ptr - start_ptr;

	return circstring;
}

static int lwcollection_from_gserialized_allowed_types(int collectiontype, int subtype)
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

static LWCOLLECTION* lwcollection_from_gserialized_buffer(uchar *data_ptr, uchar g_flags, size_t *g_size)
{
	uint32 type;
	uchar *start_ptr = data_ptr;
	LWCOLLECTION *collection;
	uint32 ngeoms = 0;
	int i = 0;

	assert(data_ptr);

	type = lw_get_uint32(data_ptr);
	data_ptr += 4; /* Skip past the type. */

	collection = (LWCOLLECTION*)lwalloc(sizeof(LWCOLLECTION));
	collection->srid = -1; /* Default */
	collection->bbox = NULL;
	collection->type = type;
	collection->flags = g_flags;

	ngeoms = lw_get_uint32(data_ptr);
	collection->ngeoms = ngeoms; /* Zero => empty geometry */
	data_ptr += 4; /* Skip past the ngeoms. */

	if ( ngeoms > 0 )
		collection->geoms = lwalloc(sizeof(LWGEOM*) * ngeoms);
	else
		collection->geoms = NULL;

	for ( i = 0; i < ngeoms; i++ )
	{
		uint32 subtype = lw_get_uint32(data_ptr);
		size_t subsize = 0;

		if ( ! lwcollection_from_gserialized_allowed_types(type, subtype) )
		{
			lwerror("Invalid subtype (%s) for collection type (%s)", lwtype_name(subtype), lwtype_name(type));
			lwfree(collection);
			return NULL;
		}
		collection->geoms[i] = lwgeom_from_gserialized_buffer(data_ptr, g_flags, &subsize);
		data_ptr += subsize;
	}

	if ( g_size )
		*g_size = data_ptr - start_ptr;

	return collection;
}


LWGEOM* lwgeom_from_gserialized_buffer(uchar *data_ptr, uchar g_flags, size_t *g_size)
{
	uint32 type;

	assert(data_ptr);

	type = lw_get_uint32(data_ptr);

	LWDEBUGF(2, "Got type %d (%s), hasz: %d hasm: %d", type, lwtype_name(type),
		FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags));

	switch (type)
	{
	case POINTTYPE:
		return (LWGEOM *)lwpoint_from_gserialized_buffer(data_ptr, g_flags, g_size);
	case LINETYPE:
		return (LWGEOM *)lwline_from_gserialized_buffer(data_ptr, g_flags, g_size);
	case CIRCSTRINGTYPE:
		return (LWGEOM *)lwcircstring_from_gserialized_buffer(data_ptr, g_flags, g_size);
	case POLYGONTYPE:
		return (LWGEOM *)lwpoly_from_gserialized_buffer(data_ptr, g_flags, g_size);
	case TRIANGLETYPE:
		return (LWGEOM *)lwtriangle_from_gserialized_buffer(data_ptr, g_flags, g_size);
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
		return (LWGEOM *)lwcollection_from_gserialized_buffer(data_ptr, g_flags, g_size);
	default:
		lwerror("Unknown geometry type: %d - %s", type, lwtype_name(type));
		return NULL;
	}
}


LWGEOM* lwgeom_from_gserialized(const GSERIALIZED *g)
{
	uchar g_flags = 0;
	uchar has_srid = 0;
	uchar *data_ptr = NULL;
	uint32 g_srid = 0;
	uint32 g_type = 0;
	LWGEOM *lwgeom = NULL;
	size_t g_size = 0;

	assert(g);

	g_srid = gserialized_get_srid(g);
	LWDEBUGF(4, "Got srid %d", g_srid);
	g_flags = g->flags;
	if ( g_srid > 0 )
		has_srid = 1;
	g_type = gserialized_get_type(g);
	LWDEBUGF(4, "Got type %d, hasz: %d hasm: %d", g_type,
		FLAGS_GET_Z(g_flags), FLAGS_GET_M(g_flags));

	data_ptr = (uchar*)g->data;
	if ( FLAGS_GET_BBOX(g_flags) )
		data_ptr += gbox_serialized_size(g_flags);

	lwgeom = lwgeom_from_gserialized_buffer(data_ptr, g_flags, &g_size);

	if ( ! lwgeom ) return NULL; /* Ooops! */

	lwgeom->type = g_type;
	lwgeom->flags = g_flags;

	if ( FLAGS_GET_BBOX(g_flags) && ! FLAGS_GET_GEODETIC(g_flags) )
	{
		float *ptr = (float*)(g->data);
		int i;

		LWDEBUGF(4, "Retrieve Serialized GBOX. hasz: %d  hasm: %d",
			FLAGS_GET_Z(g_flags)?1:0, FLAGS_GET_M(g_flags)?1:0);

		lwgeom->bbox = lwalloc(sizeof(GBOX));

		lwgeom->bbox->xmin = ptr[0];
		lwgeom->bbox->xmax = ptr[1];
		lwgeom->bbox->ymin = ptr[2];
		lwgeom->bbox->ymax = ptr[3];

		i = 4;
		if (FLAGS_GET_Z(g_flags))
		{
			lwgeom->bbox->zmin = ptr[i];
			lwgeom->bbox->zmax = ptr[i+1];
			i = 6;
		}
		if (FLAGS_GET_M(g_flags))
		{
			lwgeom->bbox->mmin = ptr[i];
			lwgeom->bbox->mmax = ptr[i+1];
		}
	}
	else
	{
		lwgeom->bbox = NULL;
	}

	if ( has_srid )
		lwgeom->srid = g_srid;
	else
		lwgeom->srid = -1;

	return lwgeom;
}

/***********************************************************************
* Calculate geocentric bounding box from geodetic coordinates
* of GSERIALIZED. To be used in index calculations to get the box
* of smaller features on the fly, and in feature creation, to
* calculate the box that will be added to the feature.
*/

static int gserialized_calculate_gbox_geocentric_from_any(uchar *data_ptr, size_t *g_size, GBOX *gbox);

static int gserialized_calculate_gbox_geocentric_from_point(uchar *data_ptr, size_t *g_size, GBOX *gbox)
{
	uchar *start_ptr = data_ptr;
	int npoints = 0;
	POINTARRAY *pa;

	assert(data_ptr);

	data_ptr += 4; /* Move past type integer. */
	npoints = lw_get_uint32(data_ptr);
	data_ptr += 4; /* Move past npoints. */

	if ( npoints == 0 ) /* Empty point */
	{
		if (g_size) *g_size = data_ptr - start_ptr;
		return LW_FAILURE;
	}
	
	pa = ptarray_construct_reference_data(FLAGS_GET_Z(gbox->flags), FLAGS_GET_M(gbox->flags), npoints, data_ptr);
	
	if ( ptarray_calculate_gbox_geodetic(pa, gbox) == LW_FAILURE )
		return LW_FAILURE;

	/* Move past all the double ordinates. */
	data_ptr += sizeof(double) * FLAGS_NDIMS(gbox->flags);

	if (g_size)
		*g_size = data_ptr - start_ptr;

	lwfree(pa);

	return LW_SUCCESS;
}

static int gserialized_calculate_gbox_geocentric_from_line(uchar *data_ptr, size_t *g_size, GBOX *gbox)
{
	uchar *start_ptr = data_ptr;
	int npoints = 0;
	POINTARRAY *pa;

	assert(data_ptr);

	data_ptr += 4; /* Move past type integer. */
	npoints = lw_get_uint32(data_ptr);
	data_ptr += 4; /* Move past npoints. */

	if ( npoints == 0 ) /* Empty linestring */
	{
		if (g_size) *g_size = data_ptr - start_ptr;
		return LW_FAILURE;
	}

	pa = ptarray_construct_reference_data(FLAGS_GET_Z(gbox->flags), FLAGS_GET_M(gbox->flags), npoints, data_ptr);

	if ( ptarray_calculate_gbox_geodetic(pa, gbox) == LW_FAILURE )
		return LW_FAILURE;

	/* Move past all the double ordinates. */
	data_ptr += sizeof(double) * FLAGS_NDIMS(gbox->flags) * npoints;

	if (g_size)
		*g_size = data_ptr - start_ptr;

	lwfree(pa);

	return LW_SUCCESS;
}

static int gserialized_calculate_gbox_geocentric_from_polygon(uchar *data_ptr, size_t *g_size, GBOX *gbox)
{
	uchar *start_ptr = data_ptr;
	int npoints0 = 0; /* Points in exterior ring. */
	int npoints = 0; /* Points in all rings. */
	int nrings = 0;
	POINTARRAY *pa;
	int i;

	assert(data_ptr);

	data_ptr += 4; /* Move past type integer. */

	nrings = lw_get_uint32(data_ptr);
	data_ptr += 4; /* Move past nrings. */

	if ( nrings <= 0 )
	{
		if (g_size) *g_size = data_ptr - start_ptr;
		return LW_FAILURE; /* Empty polygon */
	}

	npoints0 = lw_get_uint32(data_ptr); /* NPoints in first (exterior) ring. */

	for ( i = 0; i < nrings; i++ )
	{
		npoints += lw_get_uint32(data_ptr);
		data_ptr += 4; /* Move past this npoints value. */
	}

	if ( nrings % 2 ) /* Move past optional padding. */
		data_ptr += 4;

	pa = ptarray_construct_reference_data(FLAGS_GET_Z(gbox->flags), FLAGS_GET_M(gbox->flags), npoints, data_ptr);
		
	/* Bounds of exterior ring is bounds of whole polygon. */
	if ( ptarray_calculate_gbox_geodetic(pa, gbox) == LW_FAILURE )
		return LW_FAILURE;

	/* Move past all the double ordinates. */
	data_ptr += sizeof(double) * FLAGS_NDIMS(gbox->flags) * npoints;

	if (g_size)
		*g_size = data_ptr - start_ptr;

	lwfree(pa);

	return LW_SUCCESS;
}

static int gserialized_calculate_gbox_geocentric_from_triangle(uchar *data_ptr, size_t *g_size, GBOX *gbox)
{
	uchar *start_ptr = data_ptr;
	int npoints = 0;
	POINTARRAY *pa;

	assert(data_ptr);

	data_ptr += 4; /* Move past type integer. */
	npoints = lw_get_uint32(data_ptr);
	data_ptr += 4; /* Move past npoints. */

	if ( npoints == 0 ) /* Empty triangle */
	{
		if (g_size) *g_size = data_ptr - start_ptr;
		return LW_FAILURE;
	}

	pa = ptarray_construct_reference_data(FLAGS_GET_Z(gbox->flags), FLAGS_GET_M(gbox->flags), npoints, data_ptr);
	
	if ( ptarray_calculate_gbox_geodetic(pa, gbox) == LW_FAILURE )
		return LW_FAILURE;

	/* Move past all the double ordinates. */
	data_ptr += sizeof(double) * FLAGS_NDIMS(gbox->flags) * npoints;

	if (g_size)
		*g_size = data_ptr - start_ptr;

	lwfree(pa);

	return LW_SUCCESS;
}

static int gserialized_calculate_gbox_geocentric_from_collection(uchar *data_ptr, size_t *g_size, GBOX *gbox)
{
	uchar *start_ptr = data_ptr;
	int ngeoms = 0;
	int i;
	int first = LW_TRUE;
	int result = LW_FAILURE;

	assert(data_ptr);

	data_ptr += 4; /* Move past type integer. */
	ngeoms = lw_get_uint32(data_ptr);
	data_ptr += 4; /* Move past ngeoms. */

	if ( ngeoms <= 0 ) return LW_FAILURE; /* Empty collection */

	for ( i = 0; i < ngeoms; i++ )
	{
		size_t subgeom_size = 0;
		GBOX subbox;
		subbox.flags = gbox->flags;
		if ( gserialized_calculate_gbox_geocentric_from_any(data_ptr, &subgeom_size, &subbox) != LW_FAILURE )
		{
			if ( first )
			{
				gbox_duplicate(&subbox, gbox);
				first = LW_FALSE;
			}
			else
			{
				gbox_merge(&subbox, gbox);
			}
			result = LW_SUCCESS;
		}
		data_ptr += subgeom_size;
	}

	if (g_size)
		*g_size = data_ptr - start_ptr;

	return result;
}

static int gserialized_calculate_gbox_geocentric_from_any(uchar *data_ptr, size_t *g_size, GBOX *gbox)
{

	uint32 type;

	assert(data_ptr);

	type = lw_get_uint32(data_ptr);

	LWDEBUGF(2, "Got type %d (%s)", type, lwtype_name(type));
	LWDEBUGF(3, "Got gbox pointer (%p)", gbox);

	switch (type)
	{
	case POINTTYPE:
		return gserialized_calculate_gbox_geocentric_from_point(data_ptr, g_size, gbox);
	case LINETYPE:
		return gserialized_calculate_gbox_geocentric_from_line(data_ptr, g_size, gbox);
	case POLYGONTYPE:
		return gserialized_calculate_gbox_geocentric_from_polygon(data_ptr, g_size, gbox);
	case TRIANGLETYPE:
		return gserialized_calculate_gbox_geocentric_from_triangle(data_ptr, g_size, gbox);
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case COLLECTIONTYPE:
		return gserialized_calculate_gbox_geocentric_from_collection(data_ptr, g_size, gbox);
	default:
		lwerror("Unsupported geometry type: %d - %s", type, lwtype_name(type));
		return LW_FAILURE;
	}
}

int gserialized_calculate_gbox_geocentric_p(const GSERIALIZED *g, GBOX *g_box)
{
	uchar *data_ptr = NULL;
	size_t g_size = 0;
	int result = LW_SUCCESS;

	assert(g);

	/* This function only works for geodetics. */
	if ( ! FLAGS_GET_GEODETIC(g->flags) )
	{
		lwerror("Function only accepts geodetic inputs.");
		return LW_FAILURE;
	}

	LWDEBUGF(4, "Got input %s", gserialized_to_string(g));

	data_ptr = (uchar*)g->data;
	g_box->flags = g->flags;

	/* If the serialized form already has a box, skip past it. */
	if ( FLAGS_GET_BBOX(g->flags) )
	{
		int ndims = FLAGS_GET_GEODETIC(g->flags) ? 3 : FLAGS_NDIMS(g->flags);
		data_ptr += 2 * ndims * sizeof(float); /* Copy the bounding box and return. */
		LWDEBUG(3,"Serialized form has box already, skipping past...");
	}

	LWDEBUG(3,"Calculating box...");
	/* Calculate the bounding box from the geometry. */
	result = gserialized_calculate_gbox_geocentric_from_any(data_ptr, &g_size, g_box);

	if ( result == LW_FAILURE )
	{
		LWDEBUG(3,"Unable to calculate geocentric bounding box.");
		return LW_FAILURE; /* Ooops! */
	}

	LWDEBUGF(3,"Returning box: %s", gbox_to_string(g_box));
	return result;
}

GBOX* gserialized_calculate_gbox_geocentric(const GSERIALIZED *g)
{
	GBOX g_box;
	int result = LW_SUCCESS;

	result = gserialized_calculate_gbox_geocentric_p(g, &g_box);

	if ( result == LW_FAILURE )
		return NULL; /* Ooops! */

	LWDEBUGF(3,"Returning box: %s", gbox_to_string(&g_box));
	return gbox_copy(&g_box);
}


GSERIALIZED* gserialized_copy(const GSERIALIZED *g)
{
	GSERIALIZED *g_out = NULL;
	assert(g);
	g_out = (GSERIALIZED*)lwalloc(SIZE_GET(g->size));
	memcpy((uchar*)g_out,(uchar*)g,SIZE_GET(g->size));
	return g_out;
}

char* gserialized_to_string(const GSERIALIZED *g)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	LWGEOM *lwgeom = lwgeom_from_gserialized(g);
	uchar *serialized_lwgeom;
	int result;

	assert(g);

	if ( ! lwgeom )
	{
		lwerror("Unable to create lwgeom from gserialized");
		return NULL;
	}

	serialized_lwgeom = lwgeom_serialize(lwgeom);
	lwgeom_release(lwgeom);

	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE);
	lwfree(serialized_lwgeom);

	return lwg_unparser_result.wkoutput;

}

