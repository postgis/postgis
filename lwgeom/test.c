#include <stdio.h>
#include "lwgeom.h"

int main()
{
	LWGEOM point;

	// Construct a point2d
	point = make_lwpoint2d(-1, 10, 20);

	// Print WKT end HEXWKB
	printf("WKT: %s\n", lwgeom_to_wkt(point));
	printf("HEXWKB: %s\n", lwgeom_to_hexwkb(point,-1));

	return 1;
}
