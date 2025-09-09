/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2023, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2023, PostGIS contributors
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written
 * agreement is hereby granted, provided that the above copyright notice and
 * this paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL UNIVERSITE LIBRE DE BRUXELLES BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF UNIVERSITE LIBRE DE BRUXELLES HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * UNIVERSITE LIBRE DE BRUXELLES SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND UNIVERSITE LIBRE DE BRUXELLES HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *****************************************************************************/

/**
 * @file
 * @brief Spatial functions for PostGIS geography.
 *
 * These functions are supposed to be included in a forthcoming version of
 * PostGIS, to be proposed as a PR. This still remains to be done.
 * These functions are not needed in MobilityDB.
 */

// #include "point/geography_funcs.h"

/* C */
#include <float.h>
/* PostGIS */
#include <liblwgeom_internal.h>
#include <lwgeodetic_tree.h>

/*****************************************************************************/


/***********************************************************************
 * ST_LineSubstring for geographies
 ***********************************************************************/

/**
 * Find interpolation point p between geography points p1 and p2
 * so that the len(p1,p) == len(p1,p2)
 * f and p falls on p1,p2 segment
 */
static void
interpolate_point4d_spheroid(
	const POINT4D *p1, const POINT4D *p2, POINT4D *p,
	const SPHEROID *s, double f)

{
	GEOGRAPHIC_POINT g, g1, g2;
	geographic_point_init(p1->x, p1->y, &g1);
	geographic_point_init(p2->x, p2->y, &g2);
	int success;
	double dist, dir;

	/* Special sphere case */
	if ( s == NULL || s->a == s->b )
	{
		/* Calculate distance and direction between g1 and g2 */
		dist = sphere_distance(&g1, &g2);
		dir = sphere_direction(&g1, &g2, dist);
		/* Compute interpolation point */
		success = sphere_project(&g1, dist*f, dir, &g);
	}
	/* Spheroid case */
	else
	{
		/* Calculate distance and direction between g1 and g2 */
		dist = spheroid_distance(&g1, &g2, s);
		dir = spheroid_direction(&g1, &g2, s);
		/* Compute interpolation point */
		success = spheroid_project(&g1, s, dist*f, dir, &g);
	}

	/* If success, use newly computed lat and lon,
	* otherwise return precomputed cartesian result */
	if (success == LW_SUCCESS)
	{
		p->x = rad2deg(longitude_radians_normalize(g.lon));
		p->y = rad2deg(latitude_radians_normalize(g.lat));
	}
}


/**
 * @brief Return the part of a line between two fractional locations.
 */
LWGEOM *
geography_substring(
	const LWLINE *lwline,
	const SPHEROID *s,
	double from, double to,
	double tolerance)
{
	POINTARRAY *dpa;
	POINTARRAY *ipa = lwline->points;
	LWGEOM *lwresult;
	POINT4D pt;
	POINT4D p1, p2;
	GEOGRAPHIC_POINT g1, g2;
	int nsegs, i;
	double length, slength, tlength;
	int state = 0; /* 0 = before, 1 = inside */
	uint32_t srid = lwline->srid;

	/*
	* Create a dynamic pointarray with an initial capacity
	* equal to full copy of input points
	*/
	dpa = ptarray_construct_empty(
		(char) FLAGS_GET_Z(ipa->flags),
		(char) FLAGS_GET_M(ipa->flags),
		ipa->npoints);

	/* Compute total line length */
	length = ptarray_length_spheroid(ipa, s);

	/* Get 'from' and 'to' lengths */
	from = length * from;
	to = length * to;
	tlength = 0;
	getPoint4d_p(ipa, 0, &p1);
	geographic_point_init(p1.x, p1.y, &g1);
	nsegs = ipa->npoints - 1;
	for (i = 0; i < nsegs; i++)
	{
		double dseg;
		getPoint4d_p(ipa, (uint32_t) i+1, &p2);
		geographic_point_init(p2.x, p2.y, &g2);

		/* Find the length of this segment */
		/* Special sphere case */
		if ( s->a == s->b )
			slength = s->radius * sphere_distance(&g1, &g2);
		/* Spheroid case */
		else
			slength = spheroid_distance(&g1, &g2, s);

		/*  We are before requested start. */
		if (state == 0) /* before */
		{
			if (fabs ( from - ( tlength + slength ) ) <= tolerance)
			{
				/* Second point is our start */
				ptarray_append_point(dpa, &p2, LW_FALSE);
				state = 1; /* we're inside now */
				goto END;
			}
			else if (fabs(from - tlength) <= tolerance)
			{
				/*  First point is our start */
				ptarray_append_point(dpa, &p1, LW_FALSE);
				/*
				 * We're inside now, but will check
				 * 'to' point as well
				 */
				state = 1;
			}
			/*
			* Didn't reach the 'from' point,
			* nothing to do
			*/
			else if (from > tlength + slength)
			{
				goto END;
			}
			else  /* tlength < from < tlength+slength */
			{
				/* Our start is between first and second point */
				dseg = (from - tlength) / slength;
				interpolate_point4d_spheroid(&p1, &p2, &pt, s, dseg);
				ptarray_append_point(dpa, &pt, LW_FALSE);
				/* We're inside now, but will check 'to' point as well */
				state = 1;
			}
	    }

	    if (state == 1) /* inside */
	    {
			/*  'to' point is our second point. */
			if (fabs(to - ( tlength + slength ) ) <= tolerance )
			{
				ptarray_append_point(dpa, &p2, LW_FALSE);
				break; /* substring complete */
			}
			/*
			* 'to' point is our first point.
			* (should only happen if 'to' is 0)
			*/
			else if (fabs(to - tlength) <= tolerance)
			{
				ptarray_append_point(dpa, &p1, LW_FALSE);
				break; /* substring complete */
			}
			/*
			* Didn't reach the 'end' point,
			* just copy second point
			*/
			else if (to > tlength + slength)
			{
				ptarray_append_point(dpa, &p2, LW_FALSE);
				goto END;
			}
			/*
			* 'to' point falls on this segment
			* Interpolate and break.
			*/
			else if (to < tlength + slength )
			{
				dseg = (to - tlength) / slength;
				interpolate_point4d_spheroid(&p1, &p2, &pt, s, dseg);
				ptarray_append_point(dpa, &pt, LW_FALSE);
				break;
			}
	    }

	END:
		tlength += slength;
		memcpy(&p1, &p2, sizeof(POINT4D));
		geographic_point_init(p1.x, p1.y, &g1);
	}

	if (dpa->npoints <= 1) {
		lwresult = lwpoint_as_lwgeom(lwpoint_construct(srid, NULL, dpa));
	}
	else {
		lwresult = lwline_as_lwgeom(lwline_construct(srid, NULL, dpa));
	}

	return lwresult;
}


/***********************************************************************
 * Interpolate a point along a geographic line.
 ***********************************************************************/

/**
 * @brief Interpolate a point along a geographic line.
 */
LWGEOM *
geography_interpolate_points(
	const LWLINE *line, double length_fraction,
 	const SPHEROID *s, char repeat)
{
	POINT4D pt;
	uint32_t i;
	uint32_t points_to_interpolate;
	uint32_t points_found = 0;
	double length;
	double length_fraction_increment = length_fraction;
	double length_fraction_consumed = 0;
	char has_z = (char) lwgeom_has_z(lwline_as_lwgeom(line));
	char has_m = (char) lwgeom_has_m(lwline_as_lwgeom(line));
	const POINTARRAY* ipa = line->points;
	POINTARRAY *opa;
	POINT4D p1, p2;
	POINT3D q1, q2;
	LWGEOM *lwresult;
	GEOGRAPHIC_POINT g1, g2;
	uint32_t srid = line->srid;

	/* Empty.InterpolatePoint == Point Empty */
	if ( lwline_is_empty(line) )
	{
		return lwgeom_clone_deep(lwline_as_lwgeom(line));
	}

	/*
	* If distance is one of the two extremes, return the point on that
	* end rather than doing any computations
	*/
	if ( length_fraction == 0.0 || length_fraction == 1.0 )
	{
		if ( length_fraction == 0.0 )
			getPoint4d_p(ipa, 0, &pt);
		else
			getPoint4d_p(ipa, ipa->npoints-1, &pt);

		opa = ptarray_construct(has_z, has_m, 1);
		ptarray_set_point4d(opa, 0, &pt);
		return lwpoint_as_lwgeom(lwpoint_construct(srid, NULL, opa));
	}

	/* Interpolate points along the line */
	length = ptarray_length_spheroid(ipa, s);
	points_to_interpolate = repeat ? (uint32_t) floor(1 / length_fraction) : 1;
	opa = ptarray_construct(has_z, has_m, points_to_interpolate);

	getPoint4d_p(ipa, 0, &p1);
	geographic_point_init(p1.x, p1.y, &g1);
	for ( i = 0; i < ipa->npoints - 1 && points_found < points_to_interpolate; i++ )
	{
		double segment_length_frac;
		getPoint4d_p(ipa, i+1, &p2);
		geographic_point_init(p2.x, p2.y, &g2);

		/* Special sphere case */
		if ( s->a == s->b )
			segment_length_frac = s->radius * sphere_distance(&g1, &g2) / length;
		/* Spheroid case */
		else
			segment_length_frac = spheroid_distance(&g1, &g2, s) / length;

		/* If our target distance is before the total length we've seen
		* so far. create a new point some distance down the current
		* segment.
		*/
		while ( length_fraction < length_fraction_consumed + segment_length_frac && points_found < points_to_interpolate )
		{
			geog2cart(&g1, &q1);
			geog2cart(&g2, &q2);
			double segment_fraction = (length_fraction - length_fraction_consumed) / segment_length_frac;
			interpolate_point4d_spheroid(&p1, &p2, &pt, s, segment_fraction);
			ptarray_set_point4d(opa, points_found++, &pt);
			length_fraction += length_fraction_increment;
		}

		length_fraction_consumed += segment_length_frac;

		p1 = p2;
		g1 = g2;
	}

	/* Return the last point on the line. This shouldn't happen, but
	* could if there's some floating point rounding errors. */
	if (points_found < points_to_interpolate)
	{
		getPoint4d_p(ipa, ipa->npoints - 1, &pt);
		ptarray_set_point4d(opa, points_found, &pt);
	}

	if (opa->npoints <= 1)
	{
		lwresult = lwpoint_as_lwgeom(lwpoint_construct(srid, NULL, opa));
	} else {
		lwresult = lwmpoint_as_lwgeom(lwmpoint_construct(srid, opa));
	}

	return lwresult;
}

/**
 * @brief Locate a point along the point array defining a geographic line.
 */
double
ptarray_locate_point_spheroid(
	const POINTARRAY *pa,
	const POINT4D *p4d,
	const SPHEROID *s,
	double tolerance,
	double *mindistout,
	POINT4D *proj4d)
{
	GEOGRAPHIC_EDGE e;
	GEOGRAPHIC_POINT a, b, nearest = {0}; /* make compiler quiet */
	POINT4D p1, p2;
	const POINT2D *p;
	POINT2D proj;
	uint32_t i, seg = 0;
	int use_sphere = (s->a == s->b ? 1 : 0);
	int hasz;
	double za = 0.0, zb = 0.0;
	double distance,
		length,   /* Used for computing lengths */
		seglength = 0.0,  /* length of the segment where the closest point is located */
		partlength = 0.0, /* length from the beginning of the point array to the closest point */
		totlength = 0.0;  /* length of the point array */

	/* Initialize our point */
	geographic_point_init(p4d->x, p4d->y, &a);

	/* Handle point/point case here */
	if ( pa->npoints <= 1)
	{
		double mindist = 0.0;
		if ( pa->npoints == 1 )
		{
			p = getPoint2d_cp(pa, 0);
			geographic_point_init(p->x, p->y, &b);
			/* Sphere special case, axes equal */
			mindist = s->radius * sphere_distance(&a, &b);
			/* If close or greater than tolerance, get the real answer to be sure */
			if ( ! use_sphere || mindist > 0.95 * tolerance )
			{
				mindist = spheroid_distance(&a, &b, s);
			}
		}
		if ( mindistout ) *mindistout = mindist;
		return 0.0;
	}

	/* Make result really big, so that everything will be smaller than it */
	distance = FLT_MAX;

	/* Initialize start of line */
	p = getPoint2d_cp(pa, 0);
	geographic_point_init(p->x, p->y, &(e.start));

	/* Iterate through the edges in our line */
	for ( i = 1; i < pa->npoints; i++ )
	{
		double d;
		p = getPoint2d_cp(pa, i);
		geographic_point_init(p->x, p->y, &(e.end));
		/* Get the spherical distance between point and edge */
		d = s->radius * edge_distance_to_point(&e, &a, &b);
		/* New shortest distance! Record this distance / location / segment */
		if ( d < distance )
		{
			distance = d;
			nearest = b;
			seg = i - 1;
		}
	    /* We've gotten closer than the tolerance... */
		if ( d < tolerance )
		{
			/* Working on a sphere? The answer is correct, return */
			if ( use_sphere )
			{
				break;
			}
			/* Far enough past the tolerance that the spheroid calculation won't change things */
			else if ( d < tolerance * 0.95 )
			{
				break;
			}
			/* On a spheroid and near the tolerance? Confirm that we are *actually* closer than tolerance */
			else
			{
				d = spheroid_distance(&a, &nearest, s);
				/* Yes, closer than tolerance, return! */
				if ( d < tolerance )
					break;
			}
		}
		e.start = e.end;
	}

	if ( mindistout ) *mindistout = distance;

	/* See if we have a third dimension */
	hasz = (bool) FLAGS_GET_Z(pa->flags);

	/* Initialize first point of array */
	getPoint4d_p(pa, 0, &p1);
	geographic_point_init(p1.x, p1.y, &a);
	if ( hasz )
		za = p1.z;

	/* Loop and sum the length for each segment */
	for ( i = 1; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &p1);
		geographic_point_init(p1.x, p1.y, &b);
		if ( hasz )
			zb = p1.z;

		/* Special sphere case */
		if ( s->a == s->b )
			length = s->radius * sphere_distance(&a, &b);
		/* Spheroid case */
		else
			length = spheroid_distance(&a, &b, s);

		/* Add in the vertical displacement if we're in 3D */
		if ( hasz )
			length = sqrt( (zb-za)*(zb-za) + length*length );

		/* Add this segment length to the total length */
		totlength += length;

		/* Add this segment length to the partial length */
		if (i - 1 < seg)
			partlength += length;
		else if (i - 1 == seg)
			/* Save segment length for computing the final value of partlength */
			seglength = length;

		/* B gets incremented in the next loop, so we save the value here */
		a = b;
		za = zb;
	}

	/* Copy nearest into 2D/4D holder */
	proj4d->x = proj.x = rad2deg(nearest.lon);
	proj4d->y = proj.y = rad2deg(nearest.lat);

	/* Compute distance from beginning of the segment to closest point */

	/* Start of the segment */
	getPoint4d_p(pa, seg, &p1);
	geographic_point_init(p1.x, p1.y, &a);

	/* Closest point */
	geographic_point_init(proj4d->x, proj4d->y, &b);

	/* Special sphere case */
	if ( s->a == s->b )
		length = s->radius * sphere_distance(&a, &b);
	/* Spheroid case */
	else
		length = spheroid_distance(&a, &b, s);

	if ( hasz )
	{
		/* Compute Z and M values for closest point */
		double f = length / seglength;
		getPoint4d_p(pa, seg + 1, &p2);
		proj4d->z = p1.z + ((p2.z - p1.z) * f);
		proj4d->m = p1.m + ((p2.m - p1.m) * f);
		/* Add in the vertical displacement if we're in 3D */
		za = p1.z;
		zb = proj4d->z;
		length = sqrt( (zb-za)*(zb-za) + length*length );
	}

	/* Add this segment length to the total */
	partlength += length;

	/* Location of any point on a zero-length line is 0 */
	/* See http://trac.osgeo.org/postgis/ticket/1772#comment:2 */
	if ( partlength == 0 || totlength == 0 )
		return 0.0;

	/* For robustness, force 0 when closest point == startpoint */
	p = getPoint2d_cp(pa, 0);
	if ( seg == 0 && p2d_same(&proj, p) )
		return 0.0;

	/* For robustness, force 1 when closest point == endpoint */
	p = getPoint2d_cp(pa, pa->npoints - 1);
	if ( (seg >= (pa->npoints-2)) && p2d_same(&proj, p) )
		return 1.0;

	/* Floating point arithmetic is not reliable, make sure we return values [0,1] */
	double result = partlength / totlength;
	if ( result < 0.0 ) {
		result = 0.0;
	} else if ( result > 1.0 ) {
		result = 1.0;
	}
	return result;
}

/*****************************************************************************/
