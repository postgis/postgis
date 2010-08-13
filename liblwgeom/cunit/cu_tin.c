/**********************************************************************
 * $Id:$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_tin.h"


void triangle_parse(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;

	cu_error_msg_reset();	/* Because i don't trust that much prior tests...  ;) */

	/* 2 dims */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1,2 3,4 5,0 1))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), TRIANGLETYPE);
	CU_ASSERT_STRING_EQUAL("TRIANGLE((0 1,2 3,4 5,0 1))", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE));
	lwgeom_free(geom);
	/* 3DM */
	geom = lwgeom_from_ewkt("TRIANGLEM((0 1 2,3 4 5,6 7 8,0 1 2))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), TRIANGLETYPE);
	CU_ASSERT_STRING_EQUAL("TRIANGLEM((0 1 2,3 4 5,6 7 8,0 1 2))", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE));
	lwgeom_free(geom);

	/* ERROR: a missing Z values */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1 2,3 4 5,6 7,0 1 2))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("can not mix dimensionality in a geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed rings */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1 2,3 4 5,6 7 8,0 0 2))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 3))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim, with a 4D geom */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1 2 3,4 5 6 7,8 9 10 11,0 1 3 3))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: only 3 points in a face */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1 2,3 4 5,0 1 2))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("triangle must have exactly 4 points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: more than 4 points in a face */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1 2,3 4 5,6 7 8,9 10 11,0 1 2))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("triangle must have exactly 4 points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: no interior rings allowed */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 2),(9 10 11,12 13 14,15 16 17,9 10 11)", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("parse error - invalid geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* EMPTY face */	
	geom = lwgeom_from_ewkt("TRIANGLE EMPTY", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
        /*
          NOTA: Theses 3 ASSERT results will change a day cf #294 
        */
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), COLLECTIONTYPE);
	CU_ASSERT_STRING_EQUAL("GEOMETRYCOLLECTION EMPTY", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE));
	lwgeom_free(geom);

	/* explicit SRID */
	geom = lwgeom_from_ewkt("SRID=4326;TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 2))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), TRIANGLETYPE);
	CU_ASSERT_EQUAL(geom->SRID, 4326);
	CU_ASSERT_STRING_EQUAL("SRID=4326;TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 2))", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE)); 
	lwgeom_free(geom);


	/* geography support */
	geom = lwgeom_from_ewkt("TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 2))", PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(geom, 1, 0);
        CU_ASSERT_EQUAL(gserialized_get_type(g), TRIANGLETYPE);
	lwgeom_free(geom);
	lwfree(g);
}


void tin_parse(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;

	cu_error_msg_reset();	/* Because i don't trust that much prior tests...  ;) */

	/* 2 dims */
	geom = lwgeom_from_ewkt("TIN(((0 1,2 3,4 5,0 1)))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), TINTYPE);
	CU_ASSERT_STRING_EQUAL("TIN(((0 1,2 3,4 5,0 1)))", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE));
	lwgeom_free(geom);

	/* 3DM */
	geom = lwgeom_from_ewkt("TINM(((0 1 2,3 4 5,6 7 8,0 1 2)))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), TINTYPE);
	CU_ASSERT_STRING_EQUAL("TINM(((0 1 2,3 4 5,6 7 8,0 1 2)))", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE));
	lwgeom_free(geom);

	/* ERROR: a missing Z values */
	geom = lwgeom_from_ewkt("TIN(((0 1 2,3 4 5,6 7,0 1 2)))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("can not mix dimensionality in a geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed rings */
	geom = lwgeom_from_ewkt("TIN(((0 1 2,3 4 5,6 7 8,0 0 2)))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim */
	geom = lwgeom_from_ewkt("TIN(((0 1 2,3 4 5,6 7 8,0 1 3)))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim, with a 4D geom */
	geom = lwgeom_from_ewkt("TIN(((0 1 2 3,4 5 6 7,8 9 10 11,0 1 3 3)))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: only 3 points in a face */
	geom = lwgeom_from_ewkt("TIN(((0 1 2,3 4 5,0 1 2)))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("triangle must have exactly 4 points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: more than 3 points in a face */
	geom = lwgeom_from_ewkt("TIN(((0 1 2,3 4 5,6 7 8,9 10 11,0 1 2)))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("triangle must have exactly 4 points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: use ring for triangle */
	geom = lwgeom_from_ewkt("TIN(((0 1 2,3 4 5,6 7 8,0 1 2),(9 10 11,12 13 14,15 16 17,9 10 11)))", PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("parse error - invalid geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* EMPTY face */	
	geom = lwgeom_from_ewkt("TIN EMPTY", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
        /*
          NOTA: Theses 3 ASSERT results will change a day cf #294 
        */
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), COLLECTIONTYPE);
	CU_ASSERT_STRING_EQUAL("GEOMETRYCOLLECTION EMPTY", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE));
	lwgeom_free(geom);

	/* A simple tetrahedron */
	geom = lwgeom_from_ewkt("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), TINTYPE);
	CU_ASSERT_EQUAL(geom->SRID, -1);
	CU_ASSERT_STRING_EQUAL("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE)); 
	lwgeom_free(geom);

	/* A 4D tetrahedron */
	geom = lwgeom_from_ewkt("TIN(((0 0 0 0,0 0 1 0,0 1 0 2,0 0 0 0)),((0 0 0 0,0 1 0 0,1 0 0 4,0 0 0 0)),((0 0 0 0,1 0 0 0,0 0 1 6,0 0 0 0)),((1 0 0 0,0 1 0 0,0 0 1 0,1 0 0 0)))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), TINTYPE);
	CU_ASSERT_EQUAL(TYPE_HASM(geom->type), 1);
	CU_ASSERT_EQUAL(geom->SRID, -1);
	CU_ASSERT_STRING_EQUAL("TIN(((0 0 0 0,0 0 1 0,0 1 0 2,0 0 0 0)),((0 0 0 0,0 1 0 0,1 0 0 4,0 0 0 0)),((0 0 0 0,1 0 0 0,0 0 1 6,0 0 0 0)),((1 0 0 0,0 1 0 0,0 0 1 0,1 0 0 0)))", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE)); 
	lwgeom_free(geom);


	/* explicit SRID */
	geom = lwgeom_from_ewkt("SRID=4326;TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), TINTYPE);
	CU_ASSERT_EQUAL(geom->SRID, 4326);
	CU_ASSERT_STRING_EQUAL("SRID=4326;TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", lwgeom_to_ewkt(geom, PARSER_CHECK_NONE)); 
	lwgeom_free(geom);


	/* geography support */
	geom = lwgeom_from_ewkt("TIN(((0 1 2,3 4 5,6 7 8,0 1 2)))", PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(geom, 1, 0);
        CU_ASSERT_EQUAL(gserialized_get_type(g), TINTYPE);
	lwgeom_free(geom);
	lwfree(g);
}


/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo tin_tests[] = {
        PG_TEST(triangle_parse),
        PG_TEST(tin_parse),
        CU_TEST_INFO_NULL
};
CU_SuiteInfo tin_suite = {"TIN and Triangle Suite",  NULL,  NULL, tin_tests};
