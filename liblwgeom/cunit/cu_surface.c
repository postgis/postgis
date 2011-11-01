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

#include "cu_surface.h"

static void check_tgeom(char *ewkt, int type, uint32_t srid, int is_solid);

void triangle_parse(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;
	char *tmp;

	cu_error_msg_reset();	/* Because i don't trust that much prior tests...  ;) */

	/* 2 dims */
	geom = lwgeom_from_wkt("TRIANGLE((0 1,2 3,4 5,0 1))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, TRIANGLETYPE);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("TRIANGLE((0 1,2 3,4 5,0 1))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);
	/* 3DM */
	geom = lwgeom_from_wkt("TRIANGLEM((0 1 2,3 4 5,6 7 8,0 1 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, TRIANGLETYPE);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("TRIANGLEM((0 1 2,3 4 5,6 7 8,0 1 2))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* ERROR: a missing Z values */
	geom = lwgeom_from_wkt("TRIANGLE((0 1 2,3 4 5,6 7,0 1 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("can not mix dimensionality in a geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed rings */
	geom = lwgeom_from_wkt("TRIANGLE((0 1 2,3 4 5,6 7 8,0 0 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim */
	geom = lwgeom_from_wkt("TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 3))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim, with a 4D geom */
	geom = lwgeom_from_wkt("TRIANGLE((0 1 2 3,4 5 6 7,8 9 10 11,0 1 3 3))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: only 3 points in a face */
	geom = lwgeom_from_wkt("TRIANGLE((0 1 2,3 4 5,0 1 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("triangle must have exactly 4 points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: more than 4 points in a face */
	geom = lwgeom_from_wkt("TRIANGLE((0 1 2,3 4 5,6 7 8,9 10 11,0 1 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("triangle must have exactly 4 points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: no interior rings allowed */
	geom = lwgeom_from_wkt("TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 2),(9 10 11,12 13 14,15 16 17,9 10 11)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("parse error - invalid geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* EMPTY face */
	geom = lwgeom_from_wkt("TRIANGLE EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	/*
	  NOTA: Theses 3 ASSERT results will change a day cf #294
	*/
	CU_ASSERT_EQUAL(geom->type, TRIANGLETYPE);
//	CU_ASSERT_STRING_EQUAL("TRIANGLE EMPTY", lwgeom_to_wkt(geom, LW_PARSER_CHECK_NONE));
	lwgeom_free(geom);

	/* explicit SRID */
	geom = lwgeom_from_wkt("SRID=4326;TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 2))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, TRIANGLETYPE);
	CU_ASSERT_EQUAL(geom->srid, 4326);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("SRID=4326;TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 2))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* geography support */
	geom = lwgeom_from_wkt("TRIANGLE((0 1 2,3 4 5,6 7 8,0 1 2))", LW_PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(geom, 1, 0);
	CU_ASSERT_EQUAL(gserialized_get_type(g), TRIANGLETYPE);
	lwgeom_free(geom);
	lwfree(g);
}


void tin_parse(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;
	char *tmp;

	cu_error_msg_reset();	/* Because i don't trust that much prior tests...  ;) */

	/* empty */
	geom = lwgeom_from_wkt("TIN EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	/*
	  NOTA: Theses 3 ASSERT results will change a day cf #294
	*/
	CU_ASSERT_EQUAL(geom->type, TINTYPE);
//	tmp = lwgeom_to_ewkt(geom);
//	CU_ASSERT_STRING_EQUAL("TIN EMPTY", tmp);
//	lwfree(tmp);
	lwgeom_free(geom);

	/* 2 dims */
	geom = lwgeom_from_wkt("TIN(((0 1,2 3,4 5,0 1)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, TINTYPE);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("TIN(((0 1,2 3,4 5,0 1)))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* 3DM */
	geom = lwgeom_from_wkt("TINM(((0 1 2,3 4 5,6 7 8,0 1 2)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, TINTYPE);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("TINM(((0 1 2,3 4 5,6 7 8,0 1 2)))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* ERROR: a missing Z values */
	geom = lwgeom_from_wkt("TIN(((0 1 2,3 4 5,6 7,0 1 2)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("can not mix dimensionality in a geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed rings */
	geom = lwgeom_from_wkt("TIN(((0 1 2,3 4 5,6 7 8,0 0 2)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim */
	geom = lwgeom_from_wkt("TIN(((0 1 2,3 4 5,6 7 8,0 1 3)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim, with a 4D geom */
	geom = lwgeom_from_wkt("TIN(((0 1 2 3,4 5 6 7,8 9 10 11,0 1 3 3)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: only 3 points in a face */
	geom = lwgeom_from_wkt("TIN(((0 1 2,3 4 5,0 1 2)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("triangle must have exactly 4 points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: more than 3 points in a face */
	geom = lwgeom_from_wkt("TIN(((0 1 2,3 4 5,6 7 8,9 10 11,0 1 2)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("triangle must have exactly 4 points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: use ring for triangle */
	geom = lwgeom_from_wkt("TIN(((0 1 2,3 4 5,6 7 8,0 1 2),(9 10 11,12 13 14,15 16 17,9 10 11)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("parse error - invalid geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* EMPTY face */
	geom = lwgeom_from_wkt("TIN EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	/*
	  NOTA: Theses 3 ASSERT results will change a day cf #294
	*/
	CU_ASSERT_EQUAL(geom->type, TINTYPE);
//	CU_ASSERT_STRING_EQUAL("GEOMETRYCOLLECTION EMPTY", lwgeom_to_ewkt(geom));
	lwgeom_free(geom);

	/* A simple tetrahedron */
	geom = lwgeom_from_wkt("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, TINTYPE);
	CU_ASSERT_EQUAL(geom->srid, SRID_UNKNOWN);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* A 4D tetrahedron */
	geom = lwgeom_from_wkt("TIN(((0 0 0 0,0 0 1 0,0 1 0 2,0 0 0 0)),((0 0 0 0,0 1 0 0,1 0 0 4,0 0 0 0)),((0 0 0 0,1 0 0 0,0 0 1 6,0 0 0 0)),((1 0 0 0,0 1 0 0,0 0 1 0,1 0 0 0)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, TINTYPE);
	CU_ASSERT_EQUAL(FLAGS_GET_M(geom->flags), 1);
	CU_ASSERT_EQUAL(geom->srid, SRID_UNKNOWN);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("TIN(((0 0 0 0,0 0 1 0,0 1 0 2,0 0 0 0)),((0 0 0 0,0 1 0 0,1 0 0 4,0 0 0 0)),((0 0 0 0,1 0 0 0,0 0 1 6,0 0 0 0)),((1 0 0 0,0 1 0 0,0 0 1 0,1 0 0 0)))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* explicit SRID */
	geom = lwgeom_from_wkt("SRID=4326;TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, TINTYPE);
	CU_ASSERT_EQUAL(geom->srid, 4326);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("SRID=4326;TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* geography support */
	geom = lwgeom_from_wkt("TIN(((0 1 2,3 4 5,6 7 8,0 1 2)))", LW_PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(geom, 1, 0);
	CU_ASSERT_EQUAL(gserialized_get_type(g), TINTYPE);
	lwgeom_free(geom);
	lwfree(g);
}


static void
check_tgeom(char *ewkt, int type, uint32_t srid, int is_solid)
{
	LWGEOM *g1, *g2;
	TGEOM *tgeom;

	g1 = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	if (strlen(cu_error_msg)) printf("\n[%s], %s\n", ewkt, cu_error_msg);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);

	if (g1->type != type)
		printf("\n[%s], TYPE %s\n", ewkt, lwtype_name(g1->type));
	CU_ASSERT_EQUAL(g1->type, type);
	tgeom = tgeom_from_lwgeom(g1);

	if (srid != tgeom->srid)
		printf("\n[%s], srid %i / %i\n", ewkt, srid, tgeom->srid);
	CU_ASSERT_EQUAL(srid, tgeom->srid);

	if (FLAGS_GET_SOLID(tgeom->flags) != is_solid)
		printf("\n[%s], solid %i / %i\n", ewkt,
		       FLAGS_GET_SOLID(tgeom->flags), is_solid);
	CU_ASSERT_EQUAL(FLAGS_GET_SOLID(tgeom->flags), is_solid);

	g2 = lwgeom_from_tgeom(tgeom);
	if (!lwgeom_same(g1, g2))
	{
		printf("\n[%s]\n, lwgeom_same I", ewkt);
		printTGEOM(tgeom);
		if (type == TINTYPE)
		{
			printLWTIN((LWTIN *)g1);
			printLWTIN((LWTIN *)g2);
		}
		else
		{
			printLWPSURFACE((LWPSURFACE *)g1);
			printLWPSURFACE((LWPSURFACE *)g2);
		}
	}
	CU_ASSERT(lwgeom_same(g1, g2));
	lwfree(g2);

	g2 = lwgeom_from_tgeom(tgeom_deserialize(tgeom_serialize(tgeom)));
	if (!lwgeom_same(g1, g2))
	{
		printf("\n[%s]\n, lwgeom_same II", ewkt);
		printTGEOM(tgeom);
		if (type == TINTYPE)
		{
			printLWTIN((LWTIN *)g1);
			printLWTIN((LWTIN *)g2);
		}
		else
		{
			printLWPSURFACE((LWPSURFACE *)g1);
			printLWPSURFACE((LWPSURFACE *)g2);
		}
	}
	CU_ASSERT(lwgeom_same(g1, g2));

	tgeom_free(tgeom);
	lwgeom_free(g1);
	lwgeom_free(g2);
}


void polyhedralsurface_parse(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;
	char *tmp;

	cu_error_msg_reset();	/* Because i don't trust that much prior tests...  ;) */

	/* 2 dims */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 1,2 3,4 5,0 1)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, POLYHEDRALSURFACETYPE);
	tmp = lwgeom_to_hexwkb(geom, WKB_NDR | WKB_EXTENDED, 0);
//	printf("%s\n",tmp);
//	printf("010D00000001000000010300000001000000040000000000000000000000000000000000F03F00000000000000400000000000000840000000000000104000000000000014400000000000000000000000000000F03F\n");
	CU_ASSERT_STRING_EQUAL("010F00000001000000010300000001000000040000000000000000000000000000000000F03F00000000000000400000000000000840000000000000104000000000000014400000000000000000000000000000F03F", tmp);
	lwfree(tmp);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("POLYHEDRALSURFACE(((0 1,2 3,4 5,0 1)))", tmp);
	lwfree(tmp);
	lwgeom_free(geom);
	
	/* 3DM */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACEM(((0 1 2,3 4 5,6 7 8,0 1 2)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, POLYHEDRALSURFACETYPE);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("POLYHEDRALSURFACEM(((0 1 2,3 4 5,6 7 8,0 1 2)))", tmp);
	lwfree(tmp);
	tmp = lwgeom_to_hexwkb(geom, WKB_NDR | WKB_EXTENDED, 0);
	CU_ASSERT_STRING_EQUAL("010F00004001000000010300004001000000040000000000000000000000000000000000F03F000000000000004000000000000008400000000000001040000000000000144000000000000018400000000000001C4000000000000020400000000000000000000000000000F03F0000000000000040", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* ERROR: a missing Z values */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 1 2,3 4 5,6 7,0 1 2)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_STRING_EQUAL("can not mix dimensionality in a geometry", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* 1 face with 1 interior ring */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 1 2,3 4 5,6 7 8,0 1 2),(9 10 11,12 13 14,15 16 17,9 10 11)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, POLYHEDRALSURFACETYPE);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("POLYHEDRALSURFACE(((0 1 2,3 4 5,6 7 8,0 1 2),(9 10 11,12 13 14,15 16 17,9 10 11)))", tmp);
	lwfree(tmp);
	tmp = lwgeom_to_hexwkb(geom, WKB_NDR | WKB_EXTENDED, 0);
	CU_ASSERT_STRING_EQUAL("010F00008001000000010300008002000000040000000000000000000000000000000000F03F000000000000004000000000000008400000000000001040000000000000144000000000000018400000000000001C4000000000000020400000000000000000000000000000F03F00000000000000400400000000000000000022400000000000002440000000000000264000000000000028400000000000002A400000000000002C400000000000002E4000000000000030400000000000003140000000000000224000000000000024400000000000002640", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* ERROR: non closed rings */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 1 2,3 4 5,6 7 8,0 0 2)))", LW_PARSER_CHECK_ALL);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 1 2,3 4 5,6 7 8,0 1 3)))", LW_PARSER_CHECK_ALL);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: non closed face in Z dim, with a 4D geom */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 1 2 3,4 5 6 7,8 9 10 11,0 1 3 3)))", LW_PARSER_CHECK_ALL);
	CU_ASSERT_STRING_EQUAL("geometry contains non-closed rings", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* ERROR: only 3 points in a face */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 1 2,3 4 5,0 1 2)))", LW_PARSER_CHECK_ALL);
	CU_ASSERT_STRING_EQUAL("geometry requires more points", cu_error_msg);
	cu_error_msg_reset();
	lwgeom_free(geom);

	/* EMPTY face */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE EMPTY", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	/*
	  NOTA: Theses 3 ASSERT results will change a day cf #294
	*/
	CU_ASSERT_EQUAL(geom->type, POLYHEDRALSURFACETYPE);
//	CU_ASSERT_STRING_EQUAL("010700000000000000", lwgeom_to_wkb(geom, WKB_HEX | WKB_ISO | WKB_NDR, 0));
//	CU_ASSERT_STRING_EQUAL("POLYHEDRALSURFACE EMPTY", lwgeom_to_ewkt(geom));
	lwgeom_free(geom);

	/* A simple tetrahedron */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, POLYHEDRALSURFACETYPE);
	CU_ASSERT_EQUAL(geom->srid, SRID_UNKNOWN);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", tmp);
	lwfree(tmp);
	tmp = lwgeom_to_hexwkb(geom, WKB_NDR | WKB_EXTENDED, 0);
	CU_ASSERT_STRING_EQUAL("010F000080040000000103000080010000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000F03F0000000000000000000000000000F03F0000000000000000000000000000000000000000000000000000000000000000010300008001000000040000000000000000000000000000000000000000000000000000000000000000000000000000000000F03F0000000000000000000000000000F03F0000000000000000000000000000000000000000000000000000000000000000000000000000000001030000800100000004000000000000000000000000000000000000000000000000000000000000000000F03F0000000000000000000000000000000000000000000000000000000000000000000000000000F03F00000000000000000000000000000000000000000000000001030000800100000004000000000000000000F03F000000000000000000000000000000000000000000000000000000000000F03F000000000000000000000000000000000000000000000000000000000000F03F000000000000F03F00000000000000000000000000000000", tmp);
	lwfree(tmp);
	lwgeom_free(geom);

	/* A 4D tetrahedron */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 0 0 0,0 0 1 0,0 1 0 2,0 0 0 0)),((0 0 0 0,0 1 0 0,1 0 0 4,0 0 0 0)),((0 0 0 0,1 0 0 0,0 0 1 6,0 0 0 0)),((1 0 0 0,0 1 0 0,0 0 1 0,1 0 0 0)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, POLYHEDRALSURFACETYPE);
	CU_ASSERT_EQUAL(FLAGS_GET_M(geom->flags), 1);
	CU_ASSERT_EQUAL(geom->srid, SRID_UNKNOWN);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("POLYHEDRALSURFACE(((0 0 0 0,0 0 1 0,0 1 0 2,0 0 0 0)),((0 0 0 0,0 1 0 0,1 0 0 4,0 0 0 0)),((0 0 0 0,1 0 0 0,0 0 1 6,0 0 0 0)),((1 0 0 0,0 1 0 0,0 0 1 0,1 0 0 0)))", tmp);
	lwfree(tmp);
	tmp = lwgeom_to_hexwkb(geom, WKB_NDR | WKB_EXTENDED, 0);
	CU_ASSERT_STRING_EQUAL("010F0000C00400000001030000C00100000004000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000F03F00000000000000000000000000000000000000000000F03F00000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000000001030000C0010000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000F03F00000000000000000000000000000000000000000000F03F000000000000000000000000000000000000000000001040000000000000000000000000000000000000000000000000000000000000000001030000C001000000040000000000000000000000000000000000000000000000000000000000000000000000000000000000F03F00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000F03F0000000000001840000000000000000000000000000000000000000000000000000000000000000001030000C00100000004000000000000000000F03F0000000000000000000000000000000000000000000000000000000000000000000000000000F03F0000000000000000000000000000000000000000000000000000000000000000000000000000F03F0000000000000000000000000000F03F000000000000000000000000000000000000000000000000", tmp);
	lwfree(tmp);
	lwgeom_free(geom);


	/* explicit SRID */
	geom = lwgeom_from_wkt("SRID=4326;POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(geom->type, POLYHEDRALSURFACETYPE);
	CU_ASSERT_EQUAL(geom->srid, 4326);
	tmp = lwgeom_to_ewkt(geom);
	CU_ASSERT_STRING_EQUAL("SRID=4326;POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", tmp);
	lwfree(tmp);
	tmp = lwgeom_to_hexwkb(geom, WKB_NDR | WKB_EXTENDED, 0);
	CU_ASSERT_STRING_EQUAL("010F0000A0E6100000040000000103000080010000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000F03F0000000000000000000000000000F03F0000000000000000000000000000000000000000000000000000000000000000010300008001000000040000000000000000000000000000000000000000000000000000000000000000000000000000000000F03F0000000000000000000000000000F03F0000000000000000000000000000000000000000000000000000000000000000000000000000000001030000800100000004000000000000000000000000000000000000000000000000000000000000000000F03F0000000000000000000000000000000000000000000000000000000000000000000000000000F03F00000000000000000000000000000000000000000000000001030000800100000004000000000000000000F03F000000000000000000000000000000000000000000000000000000000000F03F000000000000000000000000000000000000000000000000000000000000F03F000000000000F03F00000000000000000000000000000000", tmp);
	lwfree(tmp);
	lwgeom_free(geom);


	/* geography support */
	geom = lwgeom_from_wkt("POLYHEDRALSURFACE(((0 1 2,3 4 5,6 7 8,0 1 2)))", LW_PARSER_CHECK_NONE);
	g = gserialized_from_lwgeom(geom, 1, 0);
	CU_ASSERT_EQUAL(gserialized_get_type(g), POLYHEDRALSURFACETYPE);
	lwgeom_free(geom);
	lwfree(g);
}

void
tin_tgeom(void)
{
#if 0
	/* 2D a single face */
	check_tgeom("TIN(((0 0,0 1,1 1,0 0)))", TINTYPE, 0, 0);

	/* 3DM a single face */
	check_tgeom("TINM(((0 1 2,3 4 5,6 7 8,0 1 2)))", TINTYPE, 0, 0);

	/* 4D a single face */
	check_tgeom("TIN(((0 1 2 3,4 5 6 7,8 9 10 11,0 1 2 3)))", TINTYPE, 0, 0);

	/* 3D a simple polyhedron */
	check_tgeom("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", TINTYPE, 0, 1);

	/* 3D a simple polyhedron with SRID */
	check_tgeom("SRID=4326;TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", TINTYPE, 4326, 1);

	/* EMPTY TIN */
	/* NOTA: This ASSERT type will change a day cf #294 */
	check_tgeom("TIN EMPTY", COLLECTIONTYPE, 0, 0);
	cu_error_msg_reset();
#endif
}


void
psurface_tgeom(void)
{
#if 0
	/* 2D a single face */
	check_tgeom("POLYHEDRALSURFACE(((0 0,0 1,1 1,0 0)))", POLYHEDRALSURFACETYPE, 0, 0);

	/* 3DM a single face */
	check_tgeom("POLYHEDRALSURFACEM(((0 1 2,3 4 5,6 7 8,0 1 2)))", POLYHEDRALSURFACETYPE, 0, 0);

	/* 4D a single face */
	check_tgeom("POLYHEDRALSURFACE(((0 1 2 3,4 5 6 7,8 9 10 11,0 1 2 3)))", POLYHEDRALSURFACETYPE, 0, 0);

	/* 3D a simple polyhedron */
	check_tgeom("POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", POLYHEDRALSURFACETYPE, 0, 1);

	/* 3D a simple polyhedron with SRID */
	check_tgeom("SRID=4326;POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", POLYHEDRALSURFACETYPE, 4326, 1);

	/* 4D a simple polyhedron */
	check_tgeom("POLYHEDRALSURFACE(((0 0 0 1,0 0 1 2,0 1 0 3,0 0 0 1)),((0 0 0 4,0 1 0 5,1 0 0 6,0 0 0 4)),((0 0 0 7,1 0 0 8,0 0 1 9,0 0 0 7)),((1 0 0 10,0 1 0 11,0 0 1 12,1 0 0 10)))", POLYHEDRALSURFACETYPE, 0, 0);

	/* EMPTY POLYHEDRALSURFACE */
	/* NOTA: This ASSERT type will change a day cf #294 */
	check_tgeom("POLYHEDRALSURFACE EMPTY", COLLECTIONTYPE, 0, 0);
	cu_error_msg_reset();

	/* 2D single face with one internal ring */
	check_tgeom("POLYHEDRALSURFACE(((0 1,2 3,4 5,0 1),(6 7,8 9,10 11,6 7)))", POLYHEDRALSURFACETYPE, 0, 0);

	/* 2D single face with two internal rings */
	check_tgeom("POLYHEDRALSURFACE(((0 1,2 3,4 5,0 1),(6 7,8 9,10 11,6 7),(12 13,14 15,16 17,12 13)))", POLYHEDRALSURFACETYPE, 0, 0);

	/* 4D single face with two internal rings */
	check_tgeom("POLYHEDRALSURFACE(((0 1 2 3,4 5 6 7,8 9 10 11,0 1 2 3),(12 13 14 15,16 17 18 19,20 21 22 23,12 13 14 15),(16 17 18 19,20 21 22 23,24 25 26 27,16 17 18 19)))", POLYHEDRALSURFACETYPE, 0, 0);

#endif
}


static void
check_dimension(char *ewkt, int dim)
{
	LWGEOM *geom;

	geom = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	CU_ASSERT_EQUAL(lwgeom_dimensionality(geom), dim);
	lwgeom_free(geom);
}

void
surface_dimension(void)
{
	/* 2D */
	check_dimension("POLYHEDRALSURFACE(((0 0,0 1,1 1,0 0)))", 2);
	check_dimension("TIN(((0 0,0 1,1 1,0 0)))", 2);

	/* 3D single face */
	check_dimension("POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)))", 2);
	check_dimension("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)))", 2);

	/* Tetrahedron */
	check_dimension("POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", 3);
	check_dimension("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", 3);
}


static void
check_perimeter(char *ewkt, int dim, double p)
{
	LWGEOM *geom;
	TGEOM *tgeom;

	geom = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(strlen(cu_error_msg), 0);
	tgeom = tgeom_from_lwgeom(geom);

	if (dim == 2) CU_ASSERT_DOUBLE_EQUAL(tgeom_perimeter2d(tgeom), p, 0.01);
	if (dim == 3) CU_ASSERT_DOUBLE_EQUAL(tgeom_perimeter(tgeom), p, 0.01);

	tgeom_free(tgeom);
	lwgeom_free(geom);
}

void
surface_perimeter(void)
{
	/* 2D single face */
	check_perimeter("POLYHEDRALSURFACE(((0 0,0 1,1 1,0 0)))", 2, 3.4142);
	check_perimeter("TIN(((0 0,0 1,1 1,0 0)))", 2, 3.4142);

	/* 3D single face */
	check_perimeter("POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)))", 3, 3.4142);
	check_perimeter("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)))", 3, 3.4142);

	/* 3D Tetrahedron */
	check_perimeter("POLYHEDRALSURFACE(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", 3, 0.0);
	check_perimeter("TIN(((0 0 0,0 0 1,0 1 0,0 0 0)),((0 0 0,0 1 0,1 0 0,0 0 0)),((0 0 0,1 0 0,0 0 1,0 0 0)),((1 0 0,0 1 0,0 0 1,1 0 0)))", 3, 0.0);
}


/*
** Used by test harness to register the tests in this file.
*/
/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo surface_tests[] =
{
	PG_TEST(triangle_parse),
	PG_TEST(tin_parse),
	PG_TEST(polyhedralsurface_parse),
	PG_TEST(tin_tgeom),
	PG_TEST(psurface_tgeom),
	PG_TEST(surface_dimension),
	PG_TEST(surface_perimeter),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo surface_suite = {"Triangle, Tin and PolyhedralSurface Suite",  NULL,  NULL, surface_tests};
