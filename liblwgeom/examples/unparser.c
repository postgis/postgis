/**********************************************************************
 * $Id: unparser.c 2797 2008-05-31 09:56:44Z mcayland $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include <stdio.h>
#include <math.h>

#include "liblwgeom.h"


void lwgeom_init_allocators()
{
	/*
	 * Any program linked into liblwgeom *MUST* have a function called lwgeom_init_allocators()
	 * defined. The first time that a memory allocation is required, liblwgeom calls this function
	 * to enable the user to setup their own functions for lwalloc/lwfree. Hence when being called
	 * from PostGIS we can ensure we use palloc/pfree for all memory requests rather than the
	 * system memory management calls.
	 *
	 * Since using the standard malloc/free is likely to be a common option, liblwgeom contains a
	 * function called lwgeom_install_default_allocators() which sets this up for you. Hence most
	 * people will only ever need this line within their lwgeom_init_allocations() function. 
	 */

	lwgeom_install_default_allocators();
}


int main()
{
	/*
	 * An example to show how to call the WKT/WKB unparsers in liblwgeom
	 */

	LWGEOM *lwgeom;
	uchar *serialized_lwgeom, *wkt, *wkb;
	size_t wkb_size;	

	POINTARRAY *pa;
	POINT2D point2d;
	POINTARRAY **rings;
	
	LWPOINT *testpoint;
	LWLINE *testline;
	LWPOLY *testpoly;


	/*
	 * Construct a geometry equivalent to POINT(0 51)
	 */	

	pa = ptarray_construct(0, 0, 0);
        point2d.x = 0;
        point2d.y = 51;
        pa = ptarray_addPoint(pa, (uchar *)&point2d, 2, 0);

        testpoint = lwpoint_construct(-1, NULL, pa);

	/* Generate the LWGEOM from LWPOINT, then serialize it ready for the parser */
	lwgeom = lwpoint_as_lwgeom(testpoint);
	serialized_lwgeom = lwgeom_serialize(lwgeom); 

	/* Output the geometry in WKT and WKB */
	wkt = serialized_lwgeom_to_ewkt(serialized_lwgeom);
	printf("WKT format    : %s\n", wkt);
	wkb = serialized_lwgeom_to_hexwkb(serialized_lwgeom, NDR, &wkb_size);
	printf("HEXWKB format : %s\n\n", wkb); 

	/* Free all of the allocated items */
	lwfree(wkb);
	lwfree(wkt);
	lwfree(serialized_lwgeom);
	pfree_point(testpoint);


	/*
	 * Construct a geometry equivalent to LINESTRING(0 0, 2 2, 4 1)
	 */

	pa = ptarray_construct(0, 0, 0);
	point2d.x = 0;
	point2d.y = 0;
	pa = ptarray_addPoint(pa, (uchar *)&point2d, 2, 0);		

	point2d.x = 2;
	point2d.y = 2;
	pa = ptarray_addPoint(pa, (uchar *)&point2d, 2, 1);

	point2d.x = 4;
	point2d.y = 1;
	pa = ptarray_addPoint(pa, (uchar *)&point2d, 2, 2);

	testline = lwline_construct(-1, NULL, pa);

	/* Generate the LWGEOM from LWLINE, then serialize it ready for the parser */
	lwgeom = lwline_as_lwgeom(testline);
	serialized_lwgeom = lwgeom_serialize(lwgeom); 

	/* Output the geometry in WKT and WKB */
	wkt = serialized_lwgeom_to_ewkt(serialized_lwgeom);
	printf("WKT format    : %s\n", wkt);
	wkb = serialized_lwgeom_to_hexwkb(serialized_lwgeom, NDR, &wkb_size);
	printf("HEXWKB format : %s\n\n", wkb); 

	/* Free all of the allocated items */
	lwfree(wkb);
	lwfree(wkt);
	lwfree(serialized_lwgeom);
	pfree_line(testline);



	/*
	 * Construct a geometry equivalent to POLYGON((0 0, 0 10, 10 10, 10 0, 0 0)(3 3, 3 6, 6 6, 6 3, 3 3))
	 */

	/* Allocate memory for the rings */
	rings = lwalloc(sizeof(POINTARRAY) * 2);

	/* Construct the first ring */
	rings[0] = ptarray_construct(0, 0, 0);
	point2d.x = 0;
	point2d.y = 0;
	rings[0] = ptarray_addPoint(rings[0], (uchar *)&point2d, 2, 0);

	point2d.x = 0;
	point2d.y = 10;
	rings[0] = ptarray_addPoint(rings[0], (uchar *)&point2d, 2, 1);

	point2d.x = 10;
	point2d.y = 10;
	rings[0] = ptarray_addPoint(rings[0], (uchar *)&point2d, 2, 2);

	point2d.x = 10;
	point2d.y = 0;
	rings[0] = ptarray_addPoint(rings[0], (uchar *)&point2d, 2, 3);

	point2d.x = 0;
	point2d.y = 0;
	rings[0] = ptarray_addPoint(rings[0], (uchar *)&point2d, 2, 4);
	

	/* Construct the second ring */
	rings[1] = ptarray_construct(0, 0, 0);
	point2d.x = 3;
	point2d.y = 3;
	rings[1] = ptarray_addPoint(rings[1], (uchar *)&point2d, 2, 0);

	point2d.x = 3;
	point2d.y = 6;
	rings[1] = ptarray_addPoint(rings[1], (uchar *)&point2d, 2, 1);

	point2d.x = 6;
	point2d.y = 6;
	rings[1] = ptarray_addPoint(rings[1], (uchar *)&point2d, 2, 2);

	point2d.x = 6;
	point2d.y = 3;
	rings[1] = ptarray_addPoint(rings[1], (uchar *)&point2d, 2, 3);

	point2d.x = 3;
	point2d.y = 3;
	rings[1] = ptarray_addPoint(rings[1], (uchar *)&point2d, 2, 4);	

	testpoly = lwpoly_construct(-1, NULL, 2, rings);

	/* Generate the LWGEOM from LWPOLY, then serialize it ready for the parser */
	lwgeom = lwpoly_as_lwgeom(testpoly);
	serialized_lwgeom = lwgeom_serialize(lwgeom); 

	/* Output the geometry in WKT and WKB */
	wkt = serialized_lwgeom_to_ewkt(serialized_lwgeom);
	printf("WKT format    : %s\n", wkt);
	wkb = serialized_lwgeom_to_hexwkb(serialized_lwgeom, NDR, &wkb_size);
	printf("HEXWKB format : %s\n\n", wkb); 

	/* Free all of the allocated items */
	lwfree(wkb);
	lwfree(wkt);
	lwfree(serialized_lwgeom);
	pfree_polygon(testpoly);
}
