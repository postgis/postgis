#include <stdio.h>
#include "lwgeom.h"

int main()
{
	POINT2D *pts2d[10];
	POINT3DM *pts3dm[10];
	POINT3DZ *pts3dz[10];
	POINT4D *pts4d[10];
	unsigned int npoints;
	POINTARRAY pa;
	LWGEOM point, line, poly;

	// Construct a point2d
	pts2d[0] = lwalloc(sizeof(POINT2D));
	pts2d[0]->x = 10;
	pts2d[0]->y = 20;

	// Construct a pointarray2d
	pa = ptarray_construct2d(1, pts2d);

	// Construct a point LWGEOM
	point = lwpoint_construct(-1, 0, pa);

	// Print WKT
	printf("WKT: %s\n", lwgeom_to_wkt(point));

	return 1;
}
