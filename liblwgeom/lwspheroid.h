#include "lwgeodetic.h"

/*
** Useful information about spheroids:
** 
** 	a = semi-major axis 
**  b = semi-minor axis = a - af
**  f = flattening = (a - b)/a
**  e = eccentricity (first)
**  e_sq = eccentricity (first) squared = (a*a - b*b)/(a*a)
**  omf = 1 - f
*/

double spheroid_distance(GEOGRAPHIC_POINT r, GEOGRAPHIC_POINT s, double a, double b);


