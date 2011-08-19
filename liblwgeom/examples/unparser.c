/**********************************************************************
 * $Id$
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

#include "liblwgeom_internal.h"


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
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	int result;

	LWGEOM *lwgeom;
	uint8_t *serialized_lwgeom;

	POINTARRAY *dpa;
	POINT4D point4d;
	POINTARRAY **rings;

	LWPOINT *testpoint;
	LWLINE *testline;
	LWPOLY *testpoly;


	/*
	 * Construct a geometry equivalent to POINT(0 51)
	 */

	dpa = ptarray_construct_empty(0, 0, 2);
	point4d.x = 0;
	point4d.y = 51;

ptarray_append_point(dpa, &point4d, LW_FALSE);
	testpoint = lwpoint_construct(-1, NULL, dpa);

	/* Generate the LWGEOM from LWPOINT, then serialize it ready for the parser */
	lwgeom = lwpoint_as_lwgeom(testpoint);
	serialized_lwgeom = lwgeom_serialize(lwgeom);

	/* Output the geometry in WKT and WKB */
	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_ALL);
	printf("WKT format    : %s\n", lwg_unparser_result.wkoutput);
	result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_ALL, NDR);
	printf("HEXWKB format : %s\n\n", lwg_unparser_result.wkoutput);

	/* Free all of the allocated items */
	lwfree(lwg_unparser_result.wkoutput);
	lwfree(serialized_lwgeom);
	lwpoint_free(testpoint);


	/*
	 * Construct a geometry equivalent to LINESTRING(0 0, 2 2, 4 1)
	 */

	dpa = ptarray_construct_empty(0, 0, 2);
	point4d.x = 0;
	point4d.y = 0;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 2;
	point4d.y = 2;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 4;
	point4d.y = 1;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	testline = lwline_construct(-1, NULL, dpa);

	/* Generate the LWGEOM from LWLINE, then serialize it ready for the parser */
	lwgeom = lwline_as_lwgeom(testline);
	serialized_lwgeom = lwgeom_serialize(lwgeom);

	/* Output the geometry in WKT and WKB */
	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_ALL);
	printf("WKT format    : %s\n", lwg_unparser_result.wkoutput);
	result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_ALL, NDR);
	printf("HEXWKB format : %s\n\n", lwg_unparser_result.wkoutput);

	/* Free all of the allocated items */
	lwfree(lwg_unparser_result.wkoutput);
	lwfree(serialized_lwgeom);
	lwline_free(testline);


	/*
	 * Construct a geometry equivalent to POLYGON((0 0, 0 10, 10 10, 10 0, 0 0)(3 3, 3 6, 6 6, 6 3, 3 3))
	 */

	/* Allocate memory for the rings */
	rings = lwalloc(sizeof(POINTARRAY) * 2);

	/* Construct the first ring */
	dpa = ptarray_construct_empty(0, 0, 2);
	point4d.x = 0;
	point4d.y = 0;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 0;
	point4d.y = 10;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 10;
	point4d.y = 10;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 10;
	point4d.y = 0;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 0;
	point4d.y = 0;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	rings[0] = dpa;

	/* Construct the second ring */
	dpa = ptarray_construct_empty(0, 0, 2);
	point4d.x = 3;
	point4d.y = 3;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 3;
	point4d.y = 6;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 6;
	point4d.y = 6;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 6;
	point4d.y = 3;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	point4d.x = 3;
	point4d.y = 3;
	ptarray_append_point(dpa, &point4d, LW_FALSE);

	rings[1] = dpa;

	testpoly = lwpoly_construct(-1, NULL, 2, rings);

	/* Generate the LWGEOM from LWPOLY, then serialize it ready for the parser */
	lwgeom = lwpoly_as_lwgeom(testpoly);
	serialized_lwgeom = lwgeom_serialize(lwgeom);

	/* Output the geometry in WKT and WKB */
	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE);
	printf("WKT format    : %s\n", lwg_unparser_result.wkoutput);
	result = serialized_lwgeom_to_hexwkb(&lwg_unparser_result, serialized_lwgeom, PARSER_CHECK_NONE, NDR);
	printf("HEXWKB format : %s\n\n", lwg_unparser_result.wkoutput);

	/* Free all of the allocated items */
	lwfree(lwg_unparser_result.wkoutput);
	lwfree(serialized_lwgeom);
	lwpoly_free(testpoly);

}
