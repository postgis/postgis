#include <stdio.h>
#include "lwgeom.h"

int main()
{
	POINT2D pts2d[10];
	unsigned int npoints;
	POINTARRAY pa;
	LWGEOM point, line, poly;

	// Construct a point2d
	pts2d[0].x = 10;
	pts2d[0].y = 10;
	pts2d[1].x = 10;
	pts2d[1].y = 20;
	pts2d[2].x = 20;
	pts2d[2].y = 20;
	pts2d[3].x = 20;
	pts2d[3].y = 10;
	pts2d[4].x = 10;
	pts2d[4].y = 10;

	// Construct a single-point pointarray2d
	pa = ptarray_construct2d(1, pts2d);

	// Construct a point LWGEOM
	point = lwpoint_construct(-1, 0, pa);

	// Print WKT
	printf("WKT: %s\n", lwgeom_to_wkt(point));

	// Construct a 5-points pointarray2d
	pa = ptarray_construct2d(5, pts2d);

	// Construct a line LWGEOM
	line = lwline_construct(-1, 0, pa);

	// Print WKT
	printf("WKT: %s\n", lwgeom_to_wkt(line));

	return 1;
}
