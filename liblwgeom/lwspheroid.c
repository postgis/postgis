#include "lwspheroid.h"

#define POW2(x) ((x)*(x))

static double spheroid_mu2(double alpha, double a, double b)
{
	double b2 = POW2(b);
	return POW2(cos(alpha)) * (POW2(a) - b2) / b2;
}

static double spheroid_big_a(double u2)
{
	return 1.0 + (u2 / 16384.0) * (4096.0 + u2 * (-768.0 + u2 * (320.0 - 175.0 * u2)));
}

static double spheroid_big_b(double u2)
{
	return (u2 / 1024.0) * (256.0 + u2 * (-128.0 + u2 * (74.0 - 47.0 * u2)));
}

double spheroid_distance(GEOGRAPHIC_POINT r, GEOGRAPHIC_POINT s, double a, double b)
{
	double lambda = (s.lon - r.lon);
	double f = (a - b)/a;
	double omf = 1 - f;
	double u1, u2;
	double cos_u1, cos_u2;
	double sin_u1, sin_u2;
	double big_a, big_b, delta_sigma;
	double alpha, sin_alpha, cos_alphasq, c;
	double sigma, sin_sigma, cos_sigma, cos2_sigma_m, sqrsin_sigma, last_lambda, omega;
	double cos_lambda, sin_lambda;
	double distance;
	int i = 0;

	/* Same point => zero distance */
	if( geographic_point_equals(r, s) )
	{
		return 0.0;
	}
		
	u1 = atan(omf * tan(r.lat));
	cos_u1 = cos(u1);
	sin_u1 = sin(u1);
	u2 = atan(omf * tan(s.lat));
	cos_u2 = cos(u2);
	sin_u2 = sin(u2);

	omega = lambda;
	do
	{
		cos_lambda = cos(lambda);
		sin_lambda = sin(lambda);
		sqrsin_sigma = POW2(cos_u2 * sin_lambda) +
		               POW2((cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos_lambda));
		sin_sigma = sqrt(sqrsin_sigma);
		cos_sigma = sin_u1 * sin_u2 + cos_u1 * cos_u2 * cos_lambda;
		sigma = atan2(sin_sigma, cos_sigma);
		sin_alpha = cos_u1 * cos_u2 * sin_lambda / sin(sigma);

		/* Numerical stability issue, ensure asin is not NaN */
		if( sin_alpha > 1.0 )
			alpha = M_PI_2;
		else if( sin_alpha < -1.0 )
			alpha = -1.0 * M_PI_2;
		else
			alpha = asin(sin_alpha);

		cos_alphasq = POW2(cos(alpha));
		cos2_sigma_m = cos(sigma) - (2.0 * sin_u1 * sin_u2 / cos_alphasq);

		/* Numerical stability issue, cos2 is in range */
		if( cos2_sigma_m > 1.0 ) 
			cos2_sigma_m = 1.0;
		if( cos2_sigma_m < -1.0 ) 
			cos2_sigma_m = -1.0;

		c = (f / 16.0) * cos_alphasq * (4.0 + f * (4.0 - 3.0 * cos_alphasq));
		last_lambda = lambda;
		lambda = omega + (1.0 - c) * f * sin(alpha) * (sigma + c * sin(sigma) *
		         (cos2_sigma_m + c * cos(sigma) * (-1.0 + 2.0 * POW2(cos2_sigma_m))));
		i++;
	} 
	while ( i < 999 && lambda != 0.0 && fabs((last_lambda - lambda)/lambda) > 1.0e-9 );

	u2 = spheroid_mu2(alpha, a, b);
	big_a = spheroid_big_a(u2);
	big_b = spheroid_big_b(u2);
	delta_sigma = big_b * sin_sigma * (cos2_sigma_m + (big_b / 4.0) * (cos_sigma * (-1.0 + 2.0 * POW2(cos2_sigma_m)) -
		          (big_b / 6.0) * cos2_sigma_m * (-3.0 + 4.0 * sqrsin_sigma) * (-3.0 + 4.0 * POW2(cos2_sigma_m))));

	distance = b * big_a * (sigma - delta_sigma);
	
	/* Algorithm failure, distance == NaN, fallback to sphere */
	if( distance != distance )
	{
		lwerror("spheroid_distance returned NaN: (%.20g %.20g) (%.20g %.20g) a = %.20g b = %.20g",r.lat, r.lon, s.lat, s.lon, a, b);
		return (2.0 * a + b) * sphere_distance(r, s) / 3.0;
	}

	return distance;
}

double ptarray_distance_spheroid(POINTARRAY *pa1, POINTARRAY *pa2, double a, double b, double tolerance, int check_intersection)
{
	GEOGRAPHIC_EDGE e1, e2;
	GEOGRAPHIC_POINT g1, g2;
	GEOGRAPHIC_POINT nearest1, nearest2;
	POINT2D p;
	double radius = (2.0*a + b)/3.0;
	double distance;
	int i, j;
	
	/* Make result really big, so that everything will be smaller than it */
	distance = MAXFLOAT;
	
	/* Empty point arrays? Return negative */
	if ( pa1->npoints == 0 || pa1->npoints == 0 )
		return -1.0;
	
	/* Handle point/point case here */
	if ( pa1->npoints == 1 && pa2->npoints == 1 )
	{
		getPoint2d_p(pa1, 0, &p);
		geographic_point_init(p.x, p.y, &g1);
		getPoint2d_p(pa2, 0, &p);
		geographic_point_init(p.x, p.y, &g2);
		return spheroid_distance(g1, g2, a, b);
	}

	/* Handle point/line case here */
	if ( pa1->npoints == 1 || pa2->npoints == 1 )
	{
		/* Handle one/many case here */
		int i;
		POINTARRAY *pa_one, *pa_many;
		
		if( pa1->npoints == 1 )
		{
			pa_one = pa1;
			pa_many = pa2;
		}
		else
		{
			pa_one = pa2;
			pa_many = pa1;
		}
		
		/* Initialize our point */
		getPoint2d_p(pa_one, 0, &p);
		geographic_point_init(p.x, p.y, &g1);

		/* Initialize start of line */
		getPoint2d_p(pa_many, 0, &p);
		geographic_point_init(p.x, p.y, &(e1.start));

		/* Iterate through the edges in our line */
		for( i = 1; i < pa_many->npoints; i++ )
		{
			double d;
			getPoint2d_p(pa_many, i, &p);
			geographic_point_init(p.x, p.y, &(e1.end));
			d = radius * edge_distance_to_point(e1, g1, &g2);
			if( d < distance ) 
			{
				distance = d;
				nearest2 = g2;
			}
			if( d < tolerance ) 
			{
				double sd = spheroid_distance(g1, nearest2, a, b);
				if( sd < tolerance )
					return sd;
			}
			e1.start = e1.end;
		}
		return spheroid_distance(g1, nearest2, a, b);
	}

	/* Initialize start of line 1 */
	getPoint2d_p(pa1, 0, &p);
	geographic_point_init(p.x, p.y, &(e1.start));

	
	/* Handle line/line case */
	for( i = 1; i < pa1->npoints; i++ )
	{
		getPoint2d_p(pa1, i, &p);
		geographic_point_init(p.x, p.y, &(e1.end));

		/* Initialize start of line 2 */
		getPoint2d_p(pa2, 0, &p);
		geographic_point_init(p.x, p.y, &(e2.start));

		for( j = 1; j < pa2->npoints; j++ )
		{
			double d;
			GEOGRAPHIC_POINT g;

			getPoint2d_p(pa2, j, &p);
			geographic_point_init(p.x, p.y, &(e2.end));

			LWDEBUGF(4, "e1.start == GPOINT(%.6g %.6g) ", e1.start.lat, e1.start.lon);
			LWDEBUGF(4, "e1.end == GPOINT(%.6g %.6g) ", e1.end.lat, e1.end.lon);
			LWDEBUGF(4, "e2.start == GPOINT(%.6g %.6g) ", e2.start.lat, e2.start.lon);
			LWDEBUGF(4, "e2.end == GPOINT(%.6g %.6g) ", e2.end.lat, e2.end.lon);

			if ( check_intersection && edge_intersection(e1, e2, &g) )
			{
				LWDEBUG(4,"edge intersection! returning 0.0");
				return 0.0;
			}
			d = radius * edge_distance_to_edge(e1, e2, &g1, &g2);
			LWDEBUGF(4,"got edge_distance_to_edge %.8g", d);
			
			if( d < distance )
			{
				distance = d;
				nearest1 = g1;
				nearest2 = g2;
			}
			if( d < tolerance )
			{
				double sd = spheroid_distance(nearest1, nearest2, a, b);
				if( sd < tolerance )
					return sd;
			}
				
			/* Copy end to start to allow a new end value in next iteration */
			e2.start = e2.end;
		}
		
		/* Copy end to start to allow a new end value in next iteration */
		e1.start = e1.end;
		
	}
	
	return spheroid_distance(nearest1, nearest2, a, b);
}


double lwgeom_distance_spheroid(LWGEOM *lwgeom1, LWGEOM *lwgeom2, GBOX gbox1, GBOX gbox2, double a, double b, double tolerance)
{
	int type1, type2;
	int check_intersection = LW_FALSE;
	
	assert(lwgeom1);
	assert(lwgeom2);
	
	LWDEBUGF(4, "entered function, tolerance %.8g", tolerance);

	/* What's the distance to an empty geometry? We don't know. */
	if( lwgeom_is_empty(lwgeom1) || lwgeom_is_empty(lwgeom2) )
	{
		return 0.0;
	}
		
	type1 = TYPE_GETTYPE(lwgeom1->type);
	type2 = TYPE_GETTYPE(lwgeom2->type);
	
	
	/* If the boxes aren't disjoint, we have to check for edge intersections */
	if( gbox_overlaps(gbox1, gbox2) )
		check_intersection = LW_TRUE;
	
	/* Point/line combinations can all be handled with simple point array iterations */
	if( ( type1 == POINTTYPE || type1 == LINETYPE ) && 
	    ( type2 == POINTTYPE || type2 == LINETYPE ) )
	{
		POINTARRAY *pa1, *pa2;
		
		if( type1 == POINTTYPE ) 
			pa1 = ((LWPOINT*)lwgeom1)->point;
		else 
			pa1 = ((LWLINE*)lwgeom1)->points;
		
		if( type2 == POINTTYPE )
			pa2 = ((LWPOINT*)lwgeom2)->point;
		else
			pa2 = ((LWLINE*)lwgeom2)->points;
		
		return ptarray_distance_spheroid(pa1, pa2, a, b, tolerance, check_intersection);
	}
	
	/* Point/Polygon cases, if point-in-poly, return zero, else return distance. */
	if( ( type1 == POLYGONTYPE && type2 == POINTTYPE ) || 
	    ( type2 == POLYGONTYPE && type1 == POINTTYPE ) )
	{
		POINT2D p;
		LWPOLY *lwpoly;
		LWPOINT *lwpt;
		GBOX gbox;
		double distance = MAXFLOAT;
		int i;
		
		if( type1 == POINTTYPE )
		{
			lwpt = (LWPOINT*)lwgeom1;
			lwpoly = (LWPOLY*)lwgeom2;
			gbox = gbox2;
		}
		else
		{
			lwpt = (LWPOINT*)lwgeom2;
			lwpoly = (LWPOLY*)lwgeom1;
			gbox = gbox1;
		}
		getPoint2d_p(lwpt->point, 0, &p);

		/* Point in polygon implies zero distance */
		if( lwpoly_covers_point2d(lwpoly, gbox, p) )
			return 0.0;
		
		/* Not inside, so what's the actual distance? */
		for( i = 0; i < lwpoly->nrings; i++ )
		{
			double ring_distance = ptarray_distance_spheroid(lwpoly->rings[i], lwpt->point, a, b, tolerance, check_intersection);
			if( ring_distance < distance )
				distance = ring_distance;
			if( distance < tolerance )
				return distance;
		}
		return distance;
	}

	/* Line/polygon case, if start point-in-poly, return zero, else return distance. */
	if( ( type1 == POLYGONTYPE && type2 == LINETYPE ) || 
	    ( type2 == POLYGONTYPE && type1 == LINETYPE ) )
	{
		POINT2D p;
		LWPOLY *lwpoly;
		LWLINE *lwline;
		GBOX gbox;
		double distance = MAXFLOAT;
		int i;
		
		if( type1 == LINETYPE )
		{
			lwline = (LWLINE*)lwgeom1;
			lwpoly = (LWPOLY*)lwgeom2;
			gbox = gbox2;
		}
		else
		{
			lwline = (LWLINE*)lwgeom2;
			lwpoly = (LWPOLY*)lwgeom1;
			gbox = gbox1;
		}
		getPoint2d_p(lwline->points, 0, &p);

		LWDEBUG(4, "checking if a point of line is in polygon");

		/* Point in polygon implies zero distance */
		if( lwpoly_covers_point2d(lwpoly, gbox, p) )
			return 0.0;

		LWDEBUG(4, "checking ring distances");

		/* Not contained, so what's the actual distance? */
		for( i = 0; i < lwpoly->nrings; i++ )
		{
			double ring_distance = ptarray_distance_spheroid(lwpoly->rings[i], lwline->points, a, b, tolerance, check_intersection);
			LWDEBUGF(4, "ring[%d] ring_distance = %.8g", i, ring_distance);
			if( ring_distance < distance )
				distance = ring_distance;
			if( distance < tolerance )
				return distance;
		}
		LWDEBUGF(4, "all rings checked, returning distance = %.8g", distance);
		return distance;

	}

	/* Polygon/polygon case, if start point-in-poly, return zero, else return distance. */
	if( ( type1 == POLYGONTYPE && type2 == POLYGONTYPE ) || 
	    ( type2 == POLYGONTYPE && type1 == POLYGONTYPE ) )
	{
		POINT2D p;
		LWPOLY *lwpoly1 = (LWPOLY*)lwgeom1;
		LWPOLY *lwpoly2 = (LWPOLY*)lwgeom2;
		double distance = MAXFLOAT;
		int i, j;
		
		/* Point of 2 in polygon 1 implies zero distance */
		getPoint2d_p(lwpoly1->rings[0], 0, &p);
		if( lwpoly_covers_point2d(lwpoly2, gbox2, p) )
			return 0.0;

		/* Point of 1 in polygon 2 implies zero distance */
		getPoint2d_p(lwpoly2->rings[0], 0, &p);
		if( lwpoly_covers_point2d(lwpoly1, gbox1, p) )
			return 0.0;
		
		/* Not contained, so what's the actual distance? */
		for( i = 0; i < lwpoly1->nrings; i++ )
		{
			for( j = 0; j < lwpoly2->nrings; j++ )
			{
				double ring_distance = ptarray_distance_spheroid(lwpoly1->rings[i], lwpoly2->rings[j], a, b, tolerance, check_intersection);
				if( ring_distance < distance )
					distance = ring_distance;
				if( distance < tolerance )
					return distance;
			}
		}
		return distance;		
	}

	/* Recurse into collections */
	if( lwgeom_is_collection(type1) )
	{
		int i;
		double distance = MAXFLOAT;
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom1;

		for( i = 0; i < col->ngeoms; i++ )
		{
			double geom_distance = lwgeom_distance_spheroid(col->geoms[i], lwgeom2, gbox1, gbox2, a, b, tolerance);
			if( geom_distance < distance )
				distance = geom_distance;
			if( distance < tolerance )
				return distance;
		}
		return distance;
	}

	/* Recurse into collections */
	if( lwgeom_is_collection(type2) )
	{
		int i;
		double distance = MAXFLOAT;
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom2;

		for( i = 0; i < col->ngeoms; i++ )
		{
			double geom_distance = lwgeom_distance_spheroid(lwgeom1, col->geoms[i], gbox1, gbox2, a, b, tolerance);
			if( geom_distance < distance )
				distance = geom_distance;
			if( distance < tolerance )
				return distance;
		}
		return distance;
	}


	lwerror("arguments include unsupported geometry type (%s, %s)", lwgeom_typename(type1), lwgeom_typename(type1));
	return -1.0;
	
}

