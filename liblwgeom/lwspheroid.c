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
		sqrsin_sigma = POW2(cos_u2 * sin(lambda)) +
		               POW2((cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos(lambda)));
//		printf("\ni %d\n", i);
//		printf("sqrsin_sigma %.12g\n", sqrsin_sigma);
		sin_sigma = sqrt(sqrsin_sigma);
//		printf("sin_sigma %.12g\n", sin_sigma);
		cos_sigma = sin_u1 * sin_u2 + cos_u1 * cos_u2 * cos(lambda);
//		printf("cos_sigma %.12g\n", cos_sigma);
		sigma = atan2(sin_sigma, cos_sigma);
//		printf("sigma %.12g\n", sigma);
		sin_alpha = cos_u1 * cos_u2 * sin(lambda) / sin(sigma);
//		printf("sin_alpha %.12g\n", sin_alpha);
		if( FP_EQUALS(sin_alpha, 1.0) )
			alpha = M_PI_2;
		else
			alpha = asin(sin_alpha);
//		printf("alpha %.12g\n", alpha);
		cos_alphasq = POW2(cos(alpha));
//		printf("cos_alphasq %.12g\n", cos_alphasq);
		cos2_sigma_m = cos(sigma) - (2.0 * sin_u1 * sin_u2 / cos_alphasq);
//		printf("cos2_sigma_m %.12g\n", cos2_sigma_m);
		if( cos2_sigma_m > 1.0 ) cos2_sigma_m = 1.0;
//		printf("cos2_sigma_m %.12g\n", cos2_sigma_m);
		if( cos2_sigma_m < -1.0 ) cos2_sigma_m = -1.0;
//		printf("cos2_sigma_m %.12g\n", cos2_sigma_m);
		c = (f / 16.0) * cos_alphasq * (4.0 + f * (4.0 - 3.0 * cos_alphasq));
//		printf("c %.12g\n", c);
		last_lambda = lambda;
//		printf("last_lambda %.12g\n", last_lambda);
		lambda = omega + (1.0 - c) * f * sin(alpha) * (sigma + c * sin(sigma) *
		         (cos2_sigma_m + c * cos(sigma) * (-1.0 + 2.0 * POW2(cos2_sigma_m))));
//		printf("lambda %.12g\n", lambda);
//		printf("i %d\n\n", i);
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
//		return (2.0 * a + b) * sphere_distance(r, s) / 3.0;
		lwerror("spheroid_distance returned NaN: (%.20g %.20g) (%.20g %.20g) a = %.20g b = %.20g",r.lat, r.lon, s.lat, s.lon, a, b);
		return -1.0;
	}

	return distance;
}