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
#include "lwgeom_log.h"
#include <math.h>
#include <stdlib.h>

GBOX* gbox_new(uint8_t flags)
{
	GBOX *g = (GBOX*)lwalloc(sizeof(GBOX));
	gbox_init(g);
	g->flags = flags;
	return g;
}

void gbox_init(GBOX *gbox)
{
	memset(gbox, 0, sizeof(GBOX));
}


/* TODO to be removed */
BOX3D* box3d_from_gbox(const GBOX *gbox)
{
	BOX3D *b;
	assert(gbox);
	
	b = lwalloc(sizeof(BOX3D));

	b->xmin = gbox->xmin;
	b->xmax = gbox->xmax;
	b->ymin = gbox->ymin;
	b->ymax = gbox->ymax;

	if ( FLAGS_GET_Z(gbox->flags) )
	{
		b->zmin = gbox->zmin;
		b->zmax = gbox->zmax;
	}
	else
	{
		b->zmin = b->zmax = 0.0;
	}

 	return b;	
}

/* TODO to be removed */
GBOX* box3d_to_gbox(const BOX3D *b3d)
{
	GBOX *b;
	assert(b3d);
	
	b = lwalloc(sizeof(GBOX));

	b->xmin = b3d->xmin;
	b->xmax = b3d->xmax;
	b->ymin = b3d->ymin;
	b->ymax = b3d->ymax;
	b->zmin = b3d->zmin;
	b->zmax = b3d->zmax;

 	return b;
}

void gbox_expand(GBOX *g, double d)
{
	g->xmin -= d;
	g->xmax += d;
	g->ymin -= d;
	g->ymax += d;
	if ( FLAGS_GET_Z(g->flags) )
	{
		g->zmin -= d;
		g->zmax += d;
	}
	if ( FLAGS_GET_M(g->flags) )
	{
		g->mmin -= d;
		g->mmax += d;
	}
}

int gbox_union(const GBOX *g1, const GBOX *g2, GBOX *gout)
{
	if ( (g1 == NULL) && (g2 == NULL) )
		return LW_FALSE;

	if  (g1 == NULL)
	{
		memcpy(gout, g2, sizeof(GBOX));
		return LW_TRUE;
	}
	if (g2 == NULL)
	{
		memcpy(gout, g1, sizeof(GBOX));
		return LW_TRUE;
	}

	if (g1->xmin < g2->xmin) gout->xmin = g1->xmin;
	else gout->xmin = g2->xmin;

	if (g1->ymin < g2->ymin) gout->ymin = g1->ymin;
	else gout->ymin = g2->ymin;

	if (g1->xmax > g2->xmax) gout->xmax = g1->xmax;
	else gout->xmax = g2->xmax;

	if (g1->ymax > g2->ymax) gout->ymax = g1->ymax;
	else gout->ymax = g2->ymax;

	return LW_TRUE;    
}

int gbox_same(const GBOX *g1, const GBOX *g2)
{
	if (FLAGS_GET_ZM(g1->flags) != FLAGS_GET_ZM(g2->flags))
		return LW_FALSE;

	if ( g1->xmin != g2->xmin || g1->ymin != g2->ymin ||
	     g1->xmax != g2->ymax || g1->ymax != g2->ymax ) return LW_FALSE;

	if (FLAGS_GET_Z(g1->flags) && (g1->zmin != g2->zmin || g1->zmax != g2->zmax))
		return LW_FALSE;
	if (FLAGS_GET_M(g1->flags) && (g1->mmin != g2->mmin || g1->mmax != g2->mmax))
		return LW_FALSE;

	return LW_TRUE;
}

int gbox_merge_point3d(const POINT3D *p, GBOX *gbox)
{
	if ( gbox->xmin > p->x ) gbox->xmin = p->x;
	if ( gbox->ymin > p->y ) gbox->ymin = p->y;
	if ( gbox->zmin > p->z ) gbox->zmin = p->z;
	if ( gbox->xmax < p->x ) gbox->xmax = p->x;
	if ( gbox->ymax < p->y ) gbox->ymax = p->y;
	if ( gbox->zmax < p->z ) gbox->zmax = p->z;
	return LW_SUCCESS;
}

int gbox_contains_point3d(const GBOX *gbox, const POINT3D *pt)
{
	if ( gbox->xmin > pt->x || gbox->ymin > pt->y || gbox->zmin > pt->z ||
	        gbox->xmax < pt->x || gbox->ymax < pt->y || gbox->zmax < pt->z )
	{
		return LW_FALSE;
	}
	return LW_TRUE;
}

int gbox_merge(const GBOX *new_box, GBOX *merge_box)
{
	assert(merge_box);

	if ( FLAGS_GET_ZM(merge_box->flags) != FLAGS_GET_ZM(new_box->flags) )
		return LW_FAILURE;

	if ( new_box->xmin < merge_box->xmin) merge_box->xmin = new_box->xmin;
	if ( new_box->ymin < merge_box->ymin) merge_box->ymin = new_box->ymin;
	if ( new_box->xmax > merge_box->xmax) merge_box->xmax = new_box->xmax;
	if ( new_box->ymax > merge_box->ymax) merge_box->ymax = new_box->ymax;

	if ( FLAGS_GET_Z(merge_box->flags) || FLAGS_GET_GEODETIC(merge_box->flags) )
	{
		if ( new_box->zmin < merge_box->zmin) merge_box->zmin = new_box->zmin;
		if ( new_box->zmax > merge_box->zmax) merge_box->zmax = new_box->zmax;
	}
	if ( FLAGS_GET_M(merge_box->flags) )
	{
		if ( new_box->mmin < merge_box->mmin) merge_box->mmin = new_box->mmin;
		if ( new_box->mmax > merge_box->mmax) merge_box->mmax = new_box->mmax;
	}

	return LW_SUCCESS;
}

int gbox_overlaps(const GBOX *g1, const GBOX *g2)
{

	/* Make sure our boxes are consistent */
	if ( FLAGS_GET_GEODETIC(g1->flags) != FLAGS_GET_GEODETIC(g2->flags) )
		lwerror("gbox_overlaps: cannot compare geodetic and non-geodetic boxes");

	/* Check X/Y first */
	if ( g1->xmax < g2->xmin || g1->ymax < g2->ymin ||
	     g1->xmin > g2->xmax || g1->ymin > g2->ymax )
		return LW_FALSE;
		
	/* If both geodetic or both have Z, check Z */
	if ( (FLAGS_GET_Z(g1->flags) && FLAGS_GET_Z(g2->flags)) || 
	     (FLAGS_GET_GEODETIC(g1->flags) && FLAGS_GET_GEODETIC(g2->flags)) )
	{
		if ( g1->zmax < g2->zmin || g1->zmin > g2->zmax )
			return LW_FALSE;
	}
	
	/* If both have M, check M */
	if ( FLAGS_GET_M(g1->flags) && FLAGS_GET_M(g2->flags) )
	{
		if ( g1->mmax < g2->mmin || g1->mmin > g2->mmax )
			return LW_FALSE;
	}
	
	return LW_TRUE;
}

/**
* Warning, this function is only good for x/y/z boxes, used
* in unit testing of geodetic box generation.
*/
GBOX* gbox_from_string(const char *str)
{
	const char *ptr = str;
	char *nextptr;
	char *gbox_start = strstr(str, "GBOX((");
	GBOX *gbox = gbox_new(gflags(0,0,1));
	if ( ! gbox_start ) return NULL; /* No header found */
	ptr += 6;
	gbox->xmin = strtod(ptr, &nextptr);
	if ( ptr == nextptr ) return NULL; /* No double found */
	ptr = nextptr + 1;
	gbox->ymin = strtod(ptr, &nextptr);
	if ( ptr == nextptr ) return NULL; /* No double found */
	ptr = nextptr + 1;
	gbox->zmin = strtod(ptr, &nextptr);
	if ( ptr == nextptr ) return NULL; /* No double found */
	ptr = nextptr + 3;
	gbox->xmax = strtod(ptr, &nextptr);
	if ( ptr == nextptr ) return NULL; /* No double found */
	ptr = nextptr + 1;
	gbox->ymax = strtod(ptr, &nextptr);
	if ( ptr == nextptr ) return NULL; /* No double found */
	ptr = nextptr + 1;
	gbox->zmax = strtod(ptr, &nextptr);
	if ( ptr == nextptr ) return NULL; /* No double found */
	return gbox;
}

char* gbox_to_string(const GBOX *gbox)
{
	static int sz = 128;
	char *str = NULL;

	if ( ! gbox )
		return strdup("NULL POINTER");

	str = (char*)lwalloc(sz);

	if ( FLAGS_GET_GEODETIC(gbox->flags) )
	{
		snprintf(str, sz, "GBOX((%.8g,%.8g,%.8g),(%.8g,%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->zmin, gbox->xmax, gbox->ymax, gbox->zmax);
		return str;
	}
	if ( FLAGS_GET_Z(gbox->flags) && FLAGS_GET_M(gbox->flags) )
	{
		snprintf(str, sz, "GBOX((%.8g,%.8g,%.8g,%.8g),(%.8g,%.8g,%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->zmin, gbox->mmin, gbox->xmax, gbox->ymax, gbox->zmax, gbox->mmax);
		return str;
	}
	if ( FLAGS_GET_Z(gbox->flags) )
	{
		snprintf(str, sz, "GBOX((%.8g,%.8g,%.8g),(%.8g,%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->zmin, gbox->xmax, gbox->ymax, gbox->zmax);
		return str;
	}
	if ( FLAGS_GET_M(gbox->flags) )
	{
		snprintf(str, sz, "GBOX((%.8g,%.8g,%.8g),(%.8g,%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->mmin, gbox->xmax, gbox->ymax, gbox->mmax);
		return str;
	}
	snprintf(str, sz, "GBOX((%.8g,%.8g),(%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->xmax, gbox->ymax);
	return str;
}

GBOX* gbox_copy(const GBOX *box)
{
	GBOX *copy = (GBOX*)lwalloc(sizeof(GBOX));
	memcpy(copy, box, sizeof(GBOX));
	return copy;
}

void gbox_duplicate(const GBOX *original, GBOX *duplicate)
{
	assert(duplicate);
	memcpy(duplicate, original, sizeof(GBOX));
}

size_t gbox_serialized_size(uint8_t flags)
{
	if ( FLAGS_GET_GEODETIC(flags) )
		return 6 * sizeof(float);
	else
		return 2 * FLAGS_NDIMS(flags) * sizeof(float);
}


/* ********************************************************************************
** Compute cartesian bounding GBOX boxes from LWGEOM.
*/

static int lwcircle_calculate_gbox_cartesian(const POINT4D *p1, const POINT4D *p2, const POINT4D *p3, GBOX *gbox)
{
	POINT2D xmin, ymin, xmax, ymax;
	POINT4D center;
	int p2_side;
	double radius;

	LWDEBUG(2, "lwcircle_calculate_gbox called.");

	radius = lwcircle_center(p1, p2, p3, &center);
	
	/* Negative radius signals straight line, p1/p2/p3 are colinear */
	if (radius < 0.0)
	{
        gbox->xmin = FP_MIN(p1->x, p3->x);
        gbox->ymin = FP_MIN(p1->y, p3->y);
        gbox->zmin = FP_MIN(p1->z, p3->z);
        gbox->xmax = FP_MAX(p1->x, p3->x);
        gbox->ymax = FP_MAX(p1->y, p3->y);
        gbox->zmax = FP_MAX(p1->z, p3->z);
	    return LW_SUCCESS;
	}
	
	/* Matched start/end points imply circle */
	if ( p1->x == p3->x && p1->y == p3->y )
	{
		gbox->xmin = center.x - radius;
		gbox->ymin = center.y - radius;
		gbox->zmin = FP_MIN(p1->z,p2->z);
		gbox->mmin = FP_MIN(p1->m,p2->m);
		gbox->xmax = center.x + radius;
		gbox->ymax = center.y + radius;
		gbox->zmax = FP_MAX(p1->z,p2->z);
		gbox->mmax = FP_MAX(p1->m,p2->m);
		return LW_SUCCESS;
	}

	/* First approximation, bounds of start/end points */
    gbox->xmin = FP_MIN(p1->x, p3->x);
    gbox->ymin = FP_MIN(p1->y, p3->y);
    gbox->zmin = FP_MIN(p1->z, p3->z);
    gbox->mmin = FP_MIN(p1->m, p3->m);
    gbox->xmax = FP_MAX(p1->x, p3->x);
    gbox->ymax = FP_MAX(p1->y, p3->y);
    gbox->zmax = FP_MAX(p1->z, p3->z);
    gbox->mmax = FP_MAX(p1->m, p3->m);

	/* Create points for the possible extrema */
	xmin.x = center.x - radius;
	xmin.y = center.y;
	ymin.x = center.x;
	ymin.y = center.y - radius;
	xmax.x = center.x + radius;
	xmax.y = center.y;
	ymax.x = center.x;
	ymax.y = center.y + radius;
	
	
	/* Divide the circle into two parts, one on each side of a line
	   joining p1 and p3. The circle extrema on the same side of that line
	   as p2 is on, are also the extrema of the bbox. */
	
	p2_side = signum(lw_segment_side((POINT2D*)p1, (POINT2D*)p3, (POINT2D*)p2));

	if ( p2_side == signum(lw_segment_side((POINT2D*)p1, (POINT2D*)p3, &xmin)) )
		gbox->xmin = xmin.x;

	if ( p2_side == signum(lw_segment_side((POINT2D*)p1, (POINT2D*)p3, &ymin)) )
		gbox->ymin = ymin.y;

	if ( p2_side == signum(lw_segment_side((POINT2D*)p1, (POINT2D*)p3, &xmax)) )
		gbox->xmax = xmax.x;

	if ( p2_side == signum(lw_segment_side((POINT2D*)p1, (POINT2D*)p3, &ymax)) )
		gbox->ymax = ymax.y;

	return LW_SUCCESS;
}

int ptarray_calculate_gbox_cartesian(const POINTARRAY *pa, GBOX *gbox )
{
	int i;
	POINT4D p;
	int has_z, has_m;

	if ( ! pa ) return LW_FAILURE;
	if ( ! gbox ) return LW_FAILURE;
	if ( pa->npoints < 1 ) return LW_FAILURE;

	has_z = FLAGS_GET_Z(pa->flags);
	has_m = FLAGS_GET_M(pa->flags);
	gbox->flags = gflags(has_z, has_m, 0);
	LWDEBUGF(4, "ptarray_calculate_gbox Z: %d M: %d", has_z, has_m);

	getPoint4d_p(pa, 0, &p);
	gbox->xmin = gbox->xmax = p.x;
	gbox->ymin = gbox->ymax = p.y;
	if ( has_z )
		gbox->zmin = gbox->zmax = p.z;
	if ( has_m )
		gbox->mmin = gbox->mmax = p.m;

	for ( i = 1 ; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &p);
		gbox->xmin = FP_MIN(gbox->xmin, p.x);
		gbox->xmax = FP_MAX(gbox->xmax, p.x);
		gbox->ymin = FP_MIN(gbox->ymin, p.y);
		gbox->ymax = FP_MAX(gbox->ymax, p.y);
		if ( has_z )
		{
			gbox->zmin = FP_MIN(gbox->zmin, p.z);
			gbox->zmax = FP_MAX(gbox->zmax, p.z);
		}
		if ( has_m )
		{
			gbox->mmin = FP_MIN(gbox->mmin, p.m);
			gbox->mmax = FP_MAX(gbox->mmax, p.m);
		}
	}
	return LW_SUCCESS;
}

static int lwcircstring_calculate_gbox_cartesian(LWCIRCSTRING *curve, GBOX *gbox)
{
	uint8_t flags = gflags(FLAGS_GET_Z(curve->flags), FLAGS_GET_M(curve->flags), 0);
	GBOX tmp;
	POINT4D p1, p2, p3;
	int i;

	if ( ! curve ) return LW_FAILURE;
	if ( curve->points->npoints < 3 ) return LW_FAILURE;

	tmp.flags = flags;

	/* Initialize */
	gbox->xmin = gbox->ymin = gbox->zmin = gbox->mmin = MAXFLOAT;
	gbox->xmax = gbox->ymax = gbox->zmax = gbox->mmax = -1 * MAXFLOAT;

	for ( i = 2; i < curve->points->npoints; i += 2 )
	{
		getPoint4d_p(curve->points, i-2, &p1);
		getPoint4d_p(curve->points, i-1, &p2);
		getPoint4d_p(curve->points, i, &p3);

		if (lwcircle_calculate_gbox_cartesian(&p1, &p2, &p3, &tmp) == LW_FAILURE)
			continue;

		gbox_merge(&tmp, gbox);
	}

	return LW_SUCCESS;
}

static int lwpoint_calculate_gbox_cartesian(LWPOINT *point, GBOX *gbox)
{
	if ( ! point ) return LW_FAILURE;
	return ptarray_calculate_gbox_cartesian( point->point, gbox );
}

static int lwline_calculate_gbox_cartesian(LWLINE *line, GBOX *gbox)
{
	if ( ! line ) return LW_FAILURE;
	return ptarray_calculate_gbox_cartesian( line->points, gbox );
}

static int lwtriangle_calculate_gbox_cartesian(LWTRIANGLE *triangle, GBOX *gbox)
{
	if ( ! triangle ) return LW_FAILURE;
	return ptarray_calculate_gbox_cartesian( triangle->points, gbox );
}

static int lwpoly_calculate_gbox_cartesian(LWPOLY *poly, GBOX *gbox)
{
	if ( ! poly ) return LW_FAILURE;
	if ( poly->nrings == 0 ) return LW_FAILURE;
	/* Just need to check outer ring */
	return ptarray_calculate_gbox_cartesian( poly->rings[0], gbox );
}

static int lwcollection_calculate_gbox_cartesian(LWCOLLECTION *coll, GBOX *gbox)
{
	GBOX subbox;
	int i;
	int result = LW_FAILURE;
	int first = LW_TRUE;
	assert(coll);
	if ( (coll->ngeoms == 0) || !gbox)
		return LW_FAILURE;

	subbox.flags = coll->flags;

	for ( i = 0; i < coll->ngeoms; i++ )
	{
		if ( lwgeom_calculate_gbox_cartesian((LWGEOM*)(coll->geoms[i]), &subbox) == LW_SUCCESS )
		{
			/* Keep a copy of the sub-bounding box for later 
			if ( coll->geoms[i]->bbox ) 
				lwfree(coll->geoms[i]->bbox);
			coll->geoms[i]->bbox = gbox_copy(&subbox); */
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
	}
	return result;
}

int lwgeom_calculate_gbox_cartesian(const LWGEOM *lwgeom, GBOX *gbox)
{
	if ( ! lwgeom ) return LW_FAILURE;
	LWDEBUGF(4, "lwgeom_calculate_gbox got type (%d) - %s", lwgeom->type, lwtype_name(lwgeom->type));

	switch (lwgeom->type)
	{
	case POINTTYPE:
		return lwpoint_calculate_gbox_cartesian((LWPOINT *)lwgeom, gbox);
	case LINETYPE:
		return lwline_calculate_gbox_cartesian((LWLINE *)lwgeom, gbox);
	case CIRCSTRINGTYPE:
		return lwcircstring_calculate_gbox_cartesian((LWCIRCSTRING *)lwgeom, gbox);
	case POLYGONTYPE:
		return lwpoly_calculate_gbox_cartesian((LWPOLY *)lwgeom, gbox);
	case TRIANGLETYPE:
		return lwtriangle_calculate_gbox_cartesian((LWTRIANGLE *)lwgeom, gbox);
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTICURVETYPE:
	case MULTIPOLYGONTYPE:
	case MULTISURFACETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return lwcollection_calculate_gbox_cartesian((LWCOLLECTION *)lwgeom, gbox);
	}
	/* Never get here, please. */
	lwerror("unsupported type (%d) - %s", lwgeom->type, lwtype_name(lwgeom->type));
	return LW_FAILURE;
}
