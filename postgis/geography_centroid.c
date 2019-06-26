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
 * Copyright (C) 2017 Danny Götte <danny.goette@fem.tu-ilmenau.de>
 *
 **********************************************************************/

#include "postgres.h"

#include "../postgis_config.h"

#include <math.h>

#include "liblwgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For pg macros. */
#include "lwgeom_transform.h" /* For SRID functions */

Datum geography_centroid(PG_FUNCTION_ARGS);

/* internal functions */
LWPOINT *geography_centroid_from_wpoints(const int32_t srid, const POINT3DM *points, const uint32_t size);
LWPOINT* geography_centroid_from_mline(const LWMLINE* mline, SPHEROID* s);
LWPOINT* geography_centroid_from_mpoly(const LWMPOLY* mpoly, bool use_spheroid, SPHEROID* s);
LWPOINT *cart_to_lwpoint(const double_t x_sum,
			 const double_t y_sum,
			 const double_t z_sum,
			 const double_t weight_sum,
			 const int32_t srid);
POINT3D* lonlat_to_cart(const double_t raw_lon, const double_t raw_lat);

/**
 * geography_centroid(GSERIALIZED *g)
 * returns centroid as point
 */
PG_FUNCTION_INFO_V1(geography_centroid);
Datum geography_centroid(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	LWGEOM *lwgeom_out = NULL;
    LWPOINT *lwpoint_out = NULL;
	GSERIALIZED *g = NULL;
	GSERIALIZED *g_out = NULL;
	int32_t srid;
	bool use_spheroid = true;
	SPHEROID s;

	/* Get our geometry object loaded into memory. */
	g = PG_GETARG_GSERIALIZED_P(0);
	lwgeom = lwgeom_from_gserialized(g);

	if (g == NULL)
	{
		PG_RETURN_NULL();
	}

	srid = lwgeom_get_srid(lwgeom);

	/* on empty input, return empty output */
	if (gserialized_is_empty(g))
	{
		LWCOLLECTION* empty = lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	 	lwgeom_out = lwcollection_as_lwgeom(empty);
		g_out = geography_serialize(lwgeom_out);
		PG_RETURN_POINTER(g_out);
	}

    /* Initialize spheroid */
    spheroid_init_from_srid(fcinfo, srid, &s);

    /* Set to sphere if requested */
    use_spheroid = PG_GETARG_BOOL(1);
	if ( ! use_spheroid )
		s.a = s.b = s.radius;

	switch (lwgeom_get_type(lwgeom))
	{

	case POINTTYPE:
    {
        /* centroid of a point is itself */
        PG_RETURN_POINTER(g);
        break;
    }

	case MULTIPOINTTYPE:
    {
        LWMPOINT* mpoints = lwgeom_as_lwmpoint(lwgeom);

        /* average between all points */
        uint32_t size = mpoints->ngeoms;
        POINT3DM* points = palloc(size*sizeof(POINT3DM));

		uint32_t i;
		for (i = 0; i < size; i++) {
            points[i].x = lwpoint_get_x(mpoints->geoms[i]);
            points[i].y = lwpoint_get_y(mpoints->geoms[i]);
            points[i].m = 1;
        }

		lwpoint_out = geography_centroid_from_wpoints(srid, points, size);
        pfree(points);
        break;
    }

	case LINETYPE:
    {
        LWLINE* line = lwgeom_as_lwline(lwgeom);

        /* reuse mline function */
        LWMLINE* mline = lwmline_construct_empty(srid, 0, 0);
        lwmline_add_lwline(mline, line);

        lwpoint_out = geography_centroid_from_mline(mline, &s);
        lwmline_free(mline);
		break;
    }

	case MULTILINETYPE:
    {
        LWMLINE* mline = lwgeom_as_lwmline(lwgeom);
        lwpoint_out = geography_centroid_from_mline(mline, &s);
		break;
    }

	case POLYGONTYPE:
    {
        LWPOLY* poly = lwgeom_as_lwpoly(lwgeom);

        /* reuse mpoly function */
        LWMPOLY* mpoly = lwmpoly_construct_empty(srid, 0, 0);
        lwmpoly_add_lwpoly(mpoly, poly);

        lwpoint_out = geography_centroid_from_mpoly(mpoly, use_spheroid, &s);
        lwmpoly_free(mpoly);
        break;
    }

	case MULTIPOLYGONTYPE:
    {
        LWMPOLY* mpoly = lwgeom_as_lwmpoly(lwgeom);
        lwpoint_out = geography_centroid_from_mpoly(mpoly, use_spheroid, &s);
        break;
    }
	default:
		elog(ERROR, "ST_Centroid(geography) unhandled geography type");
		PG_RETURN_NULL();
	}

	PG_FREE_IF_COPY(g, 0);

    lwgeom_out = lwpoint_as_lwgeom(lwpoint_out);
    g_out = geography_serialize(lwgeom_out);

	PG_RETURN_POINTER(g_out);
}


/**
 * Convert lat-lon-points to x-y-z-coordinates, calculate a weighted average
 * point and return lat-lon-coordinated
 */
LWPOINT *
geography_centroid_from_wpoints(const int32_t srid, const POINT3DM *points, const uint32_t size)
{
    double_t x_sum = 0;
    double_t y_sum = 0;
    double_t z_sum = 0;
    double_t weight_sum = 0;

    double_t weight = 1;
    POINT3D* point;

	uint32_t i;
	for (i = 0; i < size; i++ )
    {
		point = lonlat_to_cart(points[i].x, points[i].y);
        weight = points[i].m;

        x_sum += point->x * weight;
        y_sum += point->y * weight;
        z_sum += point->z * weight;

        weight_sum += weight;

		lwfree(point);
    }

    return cart_to_lwpoint(x_sum, y_sum, z_sum, weight_sum, srid);
}

POINT3D* lonlat_to_cart(const double_t raw_lon, const double_t raw_lat)
{
    double_t lat, lon;
    double_t sin_lat;

    POINT3D* point = lwalloc(sizeof(POINT3D));;

    // prepare coordinate for trigonometric functions from [-90, 90] -> [0, pi]
    lat = (raw_lat + 90) / 180 * M_PI;

    // prepare coordinate for trigonometric functions from [-180, 180] -> [-pi, pi]
    lon = raw_lon / 180 * M_PI;

    /* calculate value only once */
    sin_lat = sinl(lat);

    /* convert to 3D cartesian coordinates */
    point->x = sin_lat * cosl(lon);
    point->y = sin_lat * sinl(lon);
    point->z = cosl(lat);

    return point;
}

LWPOINT *
cart_to_lwpoint(const double_t x_sum,
		const double_t y_sum,
		const double_t z_sum,
		const double_t weight_sum,
		const int32_t srid)
{
    double_t x = x_sum / weight_sum;
    double_t y = y_sum / weight_sum;
    double_t z = z_sum / weight_sum;

    /* x-y-z vector length */
    double_t r = sqrtl(powl(x, 2) + powl(y, 2) + powl(z, 2));

    double_t lon = atan2l(y, x) * 180 / M_PI;
    double_t lat = acosl(z / r) * 180 / M_PI - 90;

	return lwpoint_make2d(srid, lon, lat);
}

/**
 * Split lines into segments and calculate with middle of segment as weighted
 * point.
 */
LWPOINT* geography_centroid_from_mline(const LWMLINE* mline, SPHEROID* s)
{
    double_t tolerance = 0.0;
    uint32_t size = 0;
    uint32_t i, k, j = 0;
    POINT3DM* points;
    LWPOINT* result;

    /* get total number of points */
    for (i = 0; i < mline->ngeoms; i++) {
        size += (mline->geoms[i]->points->npoints - 1) * 2;
    }

    points = palloc(size*sizeof(POINT3DM));

    for (i = 0; i < mline->ngeoms; i++) {
        LWLINE* line = mline->geoms[i];

        /* add both points of line segment as weighted point */
        for (k = 0; k < line->points->npoints - 1; k++) {
            const POINT2D* p1 = getPoint2d_cp(line->points, k);
            const POINT2D* p2 = getPoint2d_cp(line->points, k+1);
            double_t weight;

            /* use line-segment length as weight */
            LWPOINT* lwp1 = lwpoint_make2d(mline->srid, p1->x, p1->y);
            LWPOINT* lwp2 = lwpoint_make2d(mline->srid, p2->x, p2->y);
            LWGEOM* lwgeom1 = lwpoint_as_lwgeom(lwp1);
            LWGEOM* lwgeom2 = lwpoint_as_lwgeom(lwp2);
            lwgeom_set_geodetic(lwgeom1, LW_TRUE);
            lwgeom_set_geodetic(lwgeom2, LW_TRUE);

            /* use point distance as weight */
            weight = lwgeom_distance_spheroid(lwgeom1, lwgeom2, s, tolerance);

            points[j].x = p1->x;
            points[j].y = p1->y;
            points[j].m = weight;
            j++;

            points[j].x = p2->x;
            points[j].y = p2->y;
            points[j].m = weight;
            j++;

            lwgeom_free(lwgeom1);
            lwgeom_free(lwgeom2);
        }
    }

    result = geography_centroid_from_wpoints(mline->srid, points, size);
    pfree(points);
    return result;
}


/**
 * Split polygons into triangles and use centroid of the triangle with the
 * triangle area as weight to calculate the centroid of a (multi)polygon.
 */
LWPOINT* geography_centroid_from_mpoly(const LWMPOLY* mpoly, bool use_spheroid, SPHEROID* s)
{
	uint32_t size = 0;
	uint32_t i, ir, ip, j = 0;
	POINT3DM* points;
	POINT4D* reference_point = NULL;
	LWPOINT* result = NULL;

    for (ip = 0; ip < mpoly->ngeoms; ip++) {
		for (ir = 0; ir < mpoly->geoms[ip]->nrings; ir++) {
        	size += mpoly->geoms[ip]->rings[ir]->npoints - 1;
		}
    }

    points = palloc(size*sizeof(POINT3DM));


    /* use first point as reference to create triangles */
    reference_point = (POINT4D*) getPoint2d_cp(mpoly->geoms[0]->rings[0], 0);

    for (ip = 0; ip < mpoly->ngeoms; ip++) {
        LWPOLY* poly = mpoly->geoms[ip];

        for (ir = 0; ir < poly->nrings; ir++) {
            POINTARRAY* ring = poly->rings[ir];

            /* split into triangles (two points + reference point) */
            for (i = 0; i < ring->npoints - 1; i++) {
                const POINT4D* p1 = (const POINT4D*) getPoint2d_cp(ring, i);
                const POINT4D* p2 = (const POINT4D*) getPoint2d_cp(ring, i+1);
		LWPOLY* poly_tri;
		LWGEOM* geom_tri;
		double_t weight;
		POINT3DM triangle[3];
		LWPOINT* tri_centroid;

                POINTARRAY* pa = ptarray_construct_empty(0, 0, 4);
                ptarray_insert_point(pa, p1, 0);
                ptarray_insert_point(pa, p2, 1);
                ptarray_insert_point(pa, reference_point, 2);
                ptarray_insert_point(pa, p1, 3);

                poly_tri = lwpoly_construct_empty(mpoly->srid, 0, 0);
                lwpoly_add_ring(poly_tri, pa);

                geom_tri = lwpoly_as_lwgeom(poly_tri);
                lwgeom_set_geodetic(geom_tri, LW_TRUE);

            	/* Calculate the weight of the triangle. If counter clockwise,
                 * the weight is negative (e.g. for holes in polygons)
                 */

            	if ( use_spheroid )
            		weight = lwgeom_area_spheroid(geom_tri, s);
            	else
            		weight = lwgeom_area_sphere(geom_tri, s);


                triangle[0].x = p1->x;
                triangle[0].y = p1->y;
                triangle[0].m = 1;

                triangle[1].x = p2->x;
                triangle[1].y = p2->y;
                triangle[1].m = 1;

                triangle[2].x = reference_point->x;
                triangle[2].y = reference_point->y;
                triangle[2].m = 1;

                /* get center of triangle */
                tri_centroid = geography_centroid_from_wpoints(mpoly->srid, triangle, 3);

                points[j].x = lwpoint_get_x(tri_centroid);
                points[j].y = lwpoint_get_y(tri_centroid);
                points[j].m = weight;
                j++;

                lwpoint_free(tri_centroid);
                lwgeom_free(geom_tri);
            }
        }
    }
    result = geography_centroid_from_wpoints(mpoly->srid, points, size);
    pfree(points);
    return result;
}
