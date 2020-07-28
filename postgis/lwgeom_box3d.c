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
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright 2009-2017 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2018 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/geo_decls.h"
#include "gserialized_spgist_3d.h"

#include "../postgis_config.h"
#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwgeom_box3d.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>


/**
 *  BOX3D_in - takes a string rep of BOX3D and returns internal rep
 *
 *  example:
 *     "BOX3D(x1 y1 z1,x2 y2 z2)"
 * or  "BOX3D(x1 y1,x2 y2)"   z1 and z2 = 0.0
 *
 */

PG_FUNCTION_INFO_V1(BOX3D_in);
Datum BOX3D_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	int nitems;
	BOX3D *box = (BOX3D *)palloc(sizeof(BOX3D));
	box->zmin = 0;
	box->zmax = 0;

	if (strstr(str, "BOX3D(") != str)
	{
		pfree(box);
		elog(ERROR, "BOX3D parser - doesn't start with BOX3D(");
		PG_RETURN_NULL();
	}

	nitems = sscanf(str,
			"BOX3D(%le %le %le ,%le %le %le)",
			&box->xmin,
			&box->ymin,
			&box->zmin,
			&box->xmax,
			&box->ymax,
			&box->zmax);
	if (nitems != 6)
	{
		nitems = sscanf(str, "BOX3D(%le %le ,%le %le)", &box->xmin, &box->ymin, &box->xmax, &box->ymax);
		if (nitems != 4)
		{
			pfree(box);
			elog(
			    ERROR,
			    "BOX3D parser - couldn't parse.  It should look like: BOX3D(xmin ymin zmin,xmax ymax zmax) or BOX3D(xmin ymin,xmax ymax)");
			PG_RETURN_NULL();
		}
	}

	if (box->xmin > box->xmax)
	{
		float tmp = box->xmin;
		box->xmin = box->xmax;
		box->xmax = tmp;
	}
	if (box->ymin > box->ymax)
	{
		float tmp = box->ymin;
		box->ymin = box->ymax;
		box->ymax = tmp;
	}
	if (box->zmin > box->zmax)
	{
		float tmp = box->zmin;
		box->zmin = box->zmax;
		box->zmax = tmp;
	}
	box->srid = SRID_UNKNOWN;
	PG_RETURN_POINTER(box);
}

/**
 *  Takes an internal rep of a BOX3D and returns a string rep.
 *
 *  example:
 *     "BOX3D(xmin ymin zmin, xmin ymin zmin)"
 */
PG_FUNCTION_INFO_V1(BOX3D_out);
Datum BOX3D_out(PG_FUNCTION_ARGS)
{
	BOX3D *bbox = (BOX3D *)PG_GETARG_POINTER(0);
	static const int precision = 15;
	static int size = OUT_MAX_BYTES_DOUBLE * 6 + 5 + 2 + 4 + 5 + 1; /* double * 6 + "BOX3D"+ "()" + commas + null */
	int i = 0;
	char *result;

	if (bbox == NULL)
	{
		result = palloc(5);
		strcat(result, "NULL");
		PG_RETURN_CSTRING(result);
	}

	result = (char *)palloc(size);
	result[i++] = 'B';
	result[i++] = 'O';
	result[i++] = 'X';
	result[i++] = '3';
	result[i++] = 'D';
	result[i++] = '(';
	i += lwprint_double(bbox->xmin, precision, &result[i]);
	result[i++] = ' ';
	i += lwprint_double(bbox->ymin, precision, &result[i]);
	result[i++] = ' ';
	i += lwprint_double(bbox->zmin, precision, &result[i]);
	result[i++] = ',';
	i += lwprint_double(bbox->xmax, precision, &result[i]);
	result[i++] = ' ';
	i += lwprint_double(bbox->ymax, precision, &result[i]);
	result[i++] = ' ';
	i += lwprint_double(bbox->zmax, precision, &result[i]);
	result[i++] = ')';
	result[i++] = '\0';

	PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(BOX3D_to_BOX2D);
Datum BOX3D_to_BOX2D(PG_FUNCTION_ARGS)
{
	BOX3D *in = (BOX3D *)PG_GETARG_POINTER(0);
	GBOX *out = box3d_to_gbox(in);
	PG_RETURN_POINTER(out);
}

static void
box3d_to_box_p(BOX3D *box, BOX *out)
{
	if (!box)
		return;

	out->low.x = box->xmin;
	out->low.y = box->ymin;

	out->high.x = box->xmax;
	out->high.y = box->ymax;
}

PG_FUNCTION_INFO_V1(BOX3D_to_BOX);
Datum BOX3D_to_BOX(PG_FUNCTION_ARGS)
{
	BOX3D *in = (BOX3D *)PG_GETARG_POINTER(0);
	BOX *box = palloc(sizeof(BOX));

	box3d_to_box_p(in, box);
	PG_RETURN_POINTER(box);
}

PG_FUNCTION_INFO_V1(BOX3D_to_LWGEOM);
Datum BOX3D_to_LWGEOM(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	POINTARRAY *pa;
	GSERIALIZED *result;
	POINT4D pt;

	/**
	 * Alter BOX3D cast so that a valid geometry is always
	 * returned depending upon the size of the BOX3D. The
	 * code makes the following assumptions:
	 *     - If the BOX3D is a single point then return a POINT geometry
	 *     - If the BOX3D represents a line in any of X, Y or Z dimension,
	 *       return a LINESTRING geometry
	 *     - If the BOX3D represents a plane in the X, Y, or Z dimension,
	 *       return a POLYGON geometry
	 *     - Otherwise return a POLYHEDRALSURFACE geometry
	 */

	pa = ptarray_construct_empty(LW_TRUE, LW_FALSE, 5);

	/* BOX3D is a point */
	if ((box->xmin == box->xmax) && (box->ymin == box->ymax) && (box->zmin == box->zmax))
	{
		LWPOINT *lwpt = lwpoint_construct(SRID_UNKNOWN, NULL, pa);

		pt.x = box->xmin;
		pt.y = box->ymin;
		pt.z = box->zmin;
		ptarray_append_point(pa, &pt, LW_TRUE);

		result = geometry_serialize(lwpoint_as_lwgeom(lwpt));
		lwpoint_free(lwpt);
	}
	/* BOX3D is a line */
	else if (((box->xmin == box->xmax || box->ymin == box->ymax) && box->zmin == box->zmax) ||
		 ((box->xmin == box->xmax || box->zmin == box->zmax) && box->ymin == box->ymax) ||
		 ((box->ymin == box->ymax || box->zmin == box->zmax) && box->xmin == box->xmax))
	{
		LWLINE *lwline = lwline_construct(SRID_UNKNOWN, NULL, pa);

		pt.x = box->xmin;
		pt.y = box->ymin;
		pt.z = box->zmin;
		ptarray_append_point(pa, &pt, LW_TRUE);
		pt.x = box->xmax;
		pt.y = box->ymax;
		pt.z = box->zmax;
		ptarray_append_point(pa, &pt, LW_TRUE);

		result = geometry_serialize(lwline_as_lwgeom(lwline));
		lwline_free(lwline);
	}
	/* BOX3D is a polygon in the X plane */
	else if (box->xmin == box->xmax)
	{
		POINT4D points[4];
		LWPOLY *lwpoly;

		/* Initialize the 4 vertices of the polygon */
		points[0] = (POINT4D){box->xmin, box->ymin, box->zmin, 0.0};
		points[1] = (POINT4D){box->xmin, box->ymax, box->zmin, 0.0};
		points[2] = (POINT4D){box->xmin, box->ymax, box->zmax, 0.0};
		points[3] = (POINT4D){box->xmin, box->ymin, box->zmax, 0.0};

		lwpoly = lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[0], &points[1], &points[2], &points[3]);
		result = geometry_serialize(lwpoly_as_lwgeom(lwpoly));
		lwpoly_free(lwpoly);
	}
	/* BOX3D is a polygon in the Y plane */
	else if (box->ymin == box->ymax)
	{
		POINT4D points[4];
		LWPOLY *lwpoly;

		/* Initialize the 4 vertices of the polygon */
		points[0] = (POINT4D){box->xmin, box->ymin, box->zmin, 0.0};
		points[1] = (POINT4D){box->xmax, box->ymin, box->zmin, 0.0};
		points[2] = (POINT4D){box->xmax, box->ymin, box->zmax, 0.0};
		points[3] = (POINT4D){box->xmin, box->ymin, box->zmax, 0.0};

		lwpoly = lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[0], &points[1], &points[2], &points[3]);
		result = geometry_serialize(lwpoly_as_lwgeom(lwpoly));
		lwpoly_free(lwpoly);
	}
	/* BOX3D is a polygon in the Z plane */
	else if (box->zmin == box->zmax)
	{
		POINT4D points[4];
		LWPOLY *lwpoly;

		/* Initialize the 4 vertices of the polygon */
		points[0] = (POINT4D){box->xmin, box->ymin, box->zmin, 0.0};
		points[1] = (POINT4D){box->xmin, box->ymax, box->zmin, 0.0};
		points[2] = (POINT4D){box->xmax, box->ymax, box->zmin, 0.0};
		points[3] = (POINT4D){box->xmax, box->ymin, box->zmin, 0.0};

		lwpoly = lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[0], &points[1], &points[2], &points[3]);
		result = geometry_serialize(lwpoly_as_lwgeom(lwpoly));
		lwpoly_free(lwpoly);
	}
	/* BOX3D is a polyhedron */
	else
	{
		POINT4D points[8];
		static const int ngeoms = 6;
		LWGEOM **geoms = (LWGEOM **)lwalloc(sizeof(LWGEOM *) * ngeoms);
		LWGEOM *geom = NULL;

		/* Initialize the 8 vertices of the box */
		points[0] = (POINT4D){box->xmin, box->ymin, box->zmin, 0.0};
		points[1] = (POINT4D){box->xmin, box->ymax, box->zmin, 0.0};
		points[2] = (POINT4D){box->xmax, box->ymax, box->zmin, 0.0};
		points[3] = (POINT4D){box->xmax, box->ymin, box->zmin, 0.0};
		points[4] = (POINT4D){box->xmin, box->ymin, box->zmax, 0.0};
		points[5] = (POINT4D){box->xmin, box->ymax, box->zmax, 0.0};
		points[6] = (POINT4D){box->xmax, box->ymax, box->zmax, 0.0};
		points[7] = (POINT4D){box->xmax, box->ymin, box->zmax, 0.0};

		/* add bottom polygon */
		geoms[0] = lwpoly_as_lwgeom(
		    lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[0], &points[1], &points[2], &points[3]));
		/* add top polygon */
		geoms[1] = lwpoly_as_lwgeom(
		    lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[4], &points[7], &points[6], &points[5]));
		/* add left polygon */
		geoms[2] = lwpoly_as_lwgeom(
		    lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[0], &points[4], &points[5], &points[1]));
		/* add right polygon */
		geoms[3] = lwpoly_as_lwgeom(
		    lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[3], &points[2], &points[6], &points[7]));
		/* add front polygon */
		geoms[4] = lwpoly_as_lwgeom(
		    lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[0], &points[3], &points[7], &points[4]));
		/* add back polygon */
		geoms[5] = lwpoly_as_lwgeom(
		    lwpoly_construct_rectangle(LW_TRUE, LW_FALSE, &points[1], &points[5], &points[6], &points[2]));

		geom = (LWGEOM *)lwcollection_construct(POLYHEDRALSURFACETYPE, SRID_UNKNOWN, NULL, ngeoms, geoms);

		FLAGS_SET_SOLID(geom->flags, 1);

		result = geometry_serialize(geom);
		lwcollection_free((LWCOLLECTION *)geom);
	}

	gserialized_set_srid(result, box->srid);

	PG_RETURN_POINTER(result);
}

/** Expand given box of 'd' units in all directions */
void
expand_box3d(BOX3D *box, double d)
{
	box->xmin -= d;
	box->ymin -= d;
	box->zmin -= d;

	box->xmax += d;
	box->ymax += d;
	box->zmax += d;
}

static void
expand_box3d_xyz(BOX3D *box, double dx, double dy, double dz)
{
	box->xmin -= dx;
	box->xmax += dx;
	box->ymin -= dy;
	box->ymax += dy;
	box->zmin -= dz;
	box->zmax += dz;
}

PG_FUNCTION_INFO_V1(BOX3D_expand);
Datum BOX3D_expand(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	BOX3D *result = (BOX3D *)palloc(sizeof(BOX3D));
	memcpy(result, box, sizeof(BOX3D));

	if (PG_NARGS() == 2)
	{
		/* Expand the box the same amount in all directions */
		double d = PG_GETARG_FLOAT8(1);
		expand_box3d(result, d);
	}
	else
	{
		double dx = PG_GETARG_FLOAT8(1);
		double dy = PG_GETARG_FLOAT8(2);
		double dz = PG_GETARG_FLOAT8(3);

		expand_box3d_xyz(result, dx, dy, dz);
	}

	PG_RETURN_POINTER(result);
}

/**
 * convert a GSERIALIZED to BOX3D
 *
 * NOTE: the bounding box is always recomputed as the cache is a BOX2D, not a BOX3D.
 *
 */
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX3D);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = PG_GETARG_GSERIALIZED_P(0);
	LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
	GBOX gbox;
	BOX3D *result;
	int rv = lwgeom_calculate_gbox(lwgeom, &gbox);

	if (rv == LW_FAILURE)
		PG_RETURN_NULL();

	result = box3d_from_gbox(&gbox);
	result->srid = lwgeom->srid;

	lwgeom_free(lwgeom);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX3D_xmin);
Datum BOX3D_xmin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(Min(box->xmin, box->xmax));
}

PG_FUNCTION_INFO_V1(BOX3D_ymin);
Datum BOX3D_ymin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(Min(box->ymin, box->ymax));
}

PG_FUNCTION_INFO_V1(BOX3D_zmin);
Datum BOX3D_zmin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(Min(box->zmin, box->zmax));
}

PG_FUNCTION_INFO_V1(BOX3D_xmax);
Datum BOX3D_xmax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(Max(box->xmin, box->xmax));
}

PG_FUNCTION_INFO_V1(BOX3D_ymax);
Datum BOX3D_ymax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(Max(box->ymin, box->ymax));
}

PG_FUNCTION_INFO_V1(BOX3D_zmax);
Datum BOX3D_zmax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(Max(box->zmin, box->zmax));
}

/**
 * Used in the ST_Extent and ST_Extent3D aggregates, does not read the
 * serialized cached bounding box (since that is floating point)
 * but calculates the box in full from the underlying geometry.
 */
PG_FUNCTION_INFO_V1(BOX3D_combine);
Datum BOX3D_combine(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	GSERIALIZED *geom = PG_ARGISNULL(1) ? NULL : (GSERIALIZED *)PG_DETOAST_DATUM(PG_GETARG_POINTER(1));
	LWGEOM *lwgeom = NULL;
	BOX3D *result = NULL;
	GBOX gbox;
	int32_t srid;
	int rv;

	/* Can't do anything with null inputs */
	if (!box && !geom)
	{
		PG_RETURN_NULL();
	}
	/* Null geometry but non-null box, return the box */
	else if (!geom)
	{
		result = palloc(sizeof(BOX3D));
		memcpy(result, box, sizeof(BOX3D));
		PG_RETURN_POINTER(result);
	}

	/*
	 * Deserialize geometry and *calculate* the box
	 * We can't use the cached box because it's float, we *must* calculate
	 */
	lwgeom = lwgeom_from_gserialized(geom);
	srid = lwgeom->srid;
	rv = lwgeom_calculate_gbox(lwgeom, &gbox);
	lwgeom_free(lwgeom);

	/* If we couldn't calculate the box, return what we know */
	if (rv == LW_FAILURE)
	{
		PG_FREE_IF_COPY(geom, 1);
		/* No geom box, no input box, so null return */
		if (!box)
			PG_RETURN_NULL();
		result = palloc(sizeof(BOX3D));
		memcpy(result, box, sizeof(BOX3D));
		PG_RETURN_POINTER(result);
	}

	/* Null box and non-null geometry, just return the geometry box */
	if (!box)
	{
		PG_FREE_IF_COPY(geom, 1);
		result = box3d_from_gbox(&gbox);
		result->srid = srid;
		PG_RETURN_POINTER(result);
	}

	result = palloc(sizeof(BOX3D));
	result->xmax = Max(box->xmax, gbox.xmax);
	result->ymax = Max(box->ymax, gbox.ymax);
	result->zmax = Max(box->zmax, gbox.zmax);
	result->xmin = Min(box->xmin, gbox.xmin);
	result->ymin = Min(box->ymin, gbox.ymin);
	result->zmin = Min(box->zmin, gbox.zmin);
	result->srid = srid;

	PG_FREE_IF_COPY(geom, 1);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX3D_combine_BOX3D);
Datum BOX3D_combine_BOX3D(PG_FUNCTION_ARGS)
{
	BOX3D *box0 = (BOX3D *)(PG_ARGISNULL(0) ? NULL : PG_GETARG_POINTER(0));
	BOX3D *box1 = (BOX3D *)(PG_ARGISNULL(1) ? NULL : PG_GETARG_POINTER(1));
	BOX3D *result;

	if (box0 && !box1)
		PG_RETURN_POINTER(box0);

	if (box1 && !box0)
		PG_RETURN_POINTER(box1);

	if (!box1 && !box0)
		PG_RETURN_NULL();

	result = palloc(sizeof(BOX3D));
	result->xmax = Max(box0->xmax, box1->xmax);
	result->ymax = Max(box0->ymax, box1->ymax);
	result->zmax = Max(box0->zmax, box1->zmax);
	result->xmin = Min(box0->xmin, box1->xmin);
	result->ymin = Min(box0->ymin, box1->ymin);
	result->zmin = Min(box0->zmin, box1->zmin);
	result->srid = box0->srid;

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX3D_construct);
Datum BOX3D_construct(PG_FUNCTION_ARGS)
{
	GSERIALIZED *min = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *max = PG_GETARG_GSERIALIZED_P(1);
	BOX3D *result = palloc(sizeof(BOX3D));
	LWGEOM *minpoint, *maxpoint;
	POINT3DZ minp, maxp;

	minpoint = lwgeom_from_gserialized(min);
	maxpoint = lwgeom_from_gserialized(max);

	if (minpoint->type != POINTTYPE || maxpoint->type != POINTTYPE)
	{
		elog(ERROR, "BOX3D_construct: args must be points");
		PG_RETURN_NULL();
	}

	if (lwgeom_is_empty(minpoint) || lwgeom_is_empty(maxpoint) ){
		elog(ERROR, "BOX3D_construct: args can not be empty points");
		PG_RETURN_NULL();
	}

	gserialized_error_if_srid_mismatch(min, max, __func__);

	getPoint3dz_p(((LWPOINT *)minpoint)->point, 0, &minp);
	getPoint3dz_p(((LWPOINT *)maxpoint)->point, 0, &maxp);

	result->xmax = maxp.x;
	result->ymax = maxp.y;
	result->zmax = maxp.z;

	result->xmin = minp.x;
	result->ymin = minp.y;
	result->zmin = minp.z;

	result->srid = minpoint->srid;

	PG_RETURN_POINTER(result);
}

/** needed for sp-gist support PostgreSQL 11+ **/
#if POSTGIS_PGSQL_VERSION > 100
/*****************************************************************************
 * BOX3D functions
 *****************************************************************************/

/* contains? */
bool
BOX3D_contains_internal(BOX3D *box1, BOX3D *box2)
{
	return (box1->xmax >= box2->xmax && box1->xmin <= box2->xmin) &&
	       (box1->ymax >= box2->ymax && box1->ymin <= box2->ymin) &&
	       (box1->zmax >= box2->zmax && box1->zmin <= box2->zmin);
}

PG_FUNCTION_INFO_V1(BOX3D_contains);

PGDLLEXPORT Datum BOX3D_contains(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_contains_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* contained by? */
bool
BOX3D_contained_internal(BOX3D *box1, BOX3D *box2)
{
	return BOX3D_contains_internal(box2, box1);
}

PG_FUNCTION_INFO_V1(BOX3D_contained);

PGDLLEXPORT Datum BOX3D_contained(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_contained_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* overlaps? */
bool
BOX3D_overlaps_internal(BOX3D *box1, BOX3D *box2)
{
	return (box1->xmin <= box2->xmax && box2->xmin <= box1->xmax) &&
	       (box1->ymin <= box2->ymax && box2->ymin <= box1->ymax) &&
	       (box1->zmin <= box2->zmax && box2->zmin <= box1->zmax);
}

PG_FUNCTION_INFO_V1(BOX3D_overlaps);

PGDLLEXPORT Datum BOX3D_overlaps(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_overlaps_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* same? */
bool
BOX3D_same_internal(BOX3D *box1, BOX3D *box2)
{
	return (FPeq(box1->xmax, box2->xmax) && FPeq(box1->xmin, box2->xmin)) &&
	       (FPeq(box1->ymax, box2->ymax) && FPeq(box1->ymin, box2->ymin)) &&
	       (FPeq(box1->zmax, box2->zmax) && FPeq(box1->zmin, box2->zmin));
}

PG_FUNCTION_INFO_V1(BOX3D_same);

PGDLLEXPORT Datum BOX3D_same(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_same_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* strictly left of? */
bool
BOX3D_left_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->xmax < box2->xmin;
}

PG_FUNCTION_INFO_V1(BOX3D_left);

PGDLLEXPORT Datum BOX3D_left(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_left_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* does not extend to right of? */
bool
BOX3D_overleft_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->xmax <= box2->xmax;
}

PG_FUNCTION_INFO_V1(BOX3D_overleft);

PGDLLEXPORT Datum BOX3D_overleft(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_overleft_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* strictly right of? */
bool
BOX3D_right_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->xmin > box2->xmax;
}

PG_FUNCTION_INFO_V1(BOX3D_right);

PGDLLEXPORT Datum BOX3D_right(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_right_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* does not extend to left of? */
bool
BOX3D_overright_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->xmin >= box2->xmin;
}

PG_FUNCTION_INFO_V1(BOX3D_overright);

PGDLLEXPORT Datum BOX3D_overright(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_overright_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* strictly below of? */
bool
BOX3D_below_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->ymax < box2->ymin;
}

PG_FUNCTION_INFO_V1(BOX3D_below);

PGDLLEXPORT Datum BOX3D_below(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_below_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* does not extend above of? */
bool
BOX3D_overbelow_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->ymax <= box2->ymax;
}

PG_FUNCTION_INFO_V1(BOX3D_overbelow);

PGDLLEXPORT Datum BOX3D_overbelow(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_overbelow_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* strictly above of? */
bool
BOX3D_above_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->ymin > box2->ymax;
}

PG_FUNCTION_INFO_V1(BOX3D_above);

PGDLLEXPORT Datum BOX3D_above(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_above_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* does not extend below of? */
bool
BOX3D_overabove_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->ymin >= box2->ymin;
}

PG_FUNCTION_INFO_V1(BOX3D_overabove);

PGDLLEXPORT Datum BOX3D_overabove(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_overabove_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* strictly in before of? */
bool
BOX3D_front_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->zmax < box2->zmin;
}

PG_FUNCTION_INFO_V1(BOX3D_front);

PGDLLEXPORT Datum BOX3D_front(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_front_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* does not extend to the after of? */
bool
BOX3D_overfront_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->zmax <= box2->zmax;
}

PG_FUNCTION_INFO_V1(BOX3D_overfront);

PGDLLEXPORT Datum BOX3D_overfront(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_overfront_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* strictly after of? */
bool
BOX3D_back_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->zmin > box2->zmax;
}

PG_FUNCTION_INFO_V1(BOX3D_back);

PGDLLEXPORT Datum BOX3D_back(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_back_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* does not extend to the before of? */
bool
BOX3D_overback_internal(BOX3D *box1, BOX3D *box2)
{
	return box1->zmin >= box2->zmin;
}

PG_FUNCTION_INFO_V1(BOX3D_overback);

PGDLLEXPORT Datum BOX3D_overback(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	bool result = BOX3D_overback_internal(box1, box2);
	PG_RETURN_BOOL(result);
}

/* Minimum distance between 2 bounding boxes */
double
BOX3D_distance_internal(BOX3D *box1, BOX3D *box2)
{
	double sqrDist = 0;
	double d;

	if (BOX3D_overlaps_internal(box1, box2))
		return 0.0;

	/* X axis */
	if (box1->xmax < box2->xmin)
	{
		d = box1->xmax - box2->xmin;
		sqrDist += d * d;
	}
	else if (box1->xmin > box2->xmax)
	{
		d = box1->xmin - box2->xmax;
		sqrDist += d * d;
	}
	/* Y axis */
	if (box1->ymax < box2->ymin)
	{
		d = box1->ymax - box2->ymin;
		sqrDist += d * d;
	}
	else if (box1->ymin > box2->ymax)
	{
		d = box1->ymin - box2->ymax;
		sqrDist += d * d;
	}
	/* Z axis */
	if (box1->zmax < box2->zmin)
	{
		d = box1->zmax - box2->zmin;
		sqrDist += d * d;
	}
	else if (box1->zmin > box2->zmax)
	{
		d = box1->zmin - box2->zmax;
		sqrDist += d * d;
	}

	return sqrt(sqrDist);
}

PG_FUNCTION_INFO_V1(BOX3D_distance);

PGDLLEXPORT Datum BOX3D_distance(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = PG_GETARG_BOX3D_P(0);
	BOX3D *box2 = PG_GETARG_BOX3D_P(1);
	PG_RETURN_FLOAT8(BOX3D_distance_internal(box1, box2));
}
#endif
