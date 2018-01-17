/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2010-2015 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2011      Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/

#include "liblwgeom_internal.h"

/*
* Coordinate object to hold information about last coordinate temporarily.
* We need to know how many dimensions there are at any given time.
*/
typedef struct
{
	uint8_t flags;
	double x;
	double y;
	double z;
	double m;
}
POINT;

/*
* Global that holds the final output geometry for the WKT parser.
*/
extern LWGEOM_PARSER_RESULT global_parser_result;
extern const char *parser_error_messages[];

/*
* Prototypes for the lexer
*/
extern void wkt_lexer_init(char *str);
extern void wkt_lexer_close(void);
extern int wkt_yylex_destroy(void);


/*
* Functions called from within the bison parser to construct geometries.
*/
int wkt_lexer_read_srid(char *str);
POINT wkt_parser_coord_2(double c1, double c2);
POINT wkt_parser_coord_3(double c1, double c2, double c3);
POINT wkt_parser_coord_4(double c1, double c2, double c3, double c4);
POINTARRAY* wkt_parser_ptarray_add_coord(POINTARRAY *pa, POINT p);
POINTARRAY* wkt_parser_ptarray_new(POINT p);
LWGEOM* wkt_parser_point_new(POINTARRAY *pa, char *dimensionality);
LWGEOM* wkt_parser_linestring_new(POINTARRAY *pa, char *dimensionality);
LWGEOM* wkt_parser_circularstring_new(POINTARRAY *pa, char *dimensionality);
LWGEOM* wkt_parser_triangle_new(POINTARRAY *pa, char *dimensionality);
LWGEOM* wkt_parser_polygon_new(POINTARRAY *pa, char dimcheck);
LWGEOM* wkt_parser_polygon_add_ring(LWGEOM *poly, POINTARRAY *pa, char dimcheck);
LWGEOM* wkt_parser_polygon_finalize(LWGEOM *poly, char *dimensionality);
LWGEOM* wkt_parser_curvepolygon_new(LWGEOM *ring);
LWGEOM* wkt_parser_curvepolygon_add_ring(LWGEOM *poly, LWGEOM *ring);
LWGEOM* wkt_parser_curvepolygon_finalize(LWGEOM *poly, char *dimensionality);
LWGEOM* wkt_parser_compound_new(LWGEOM *element);
LWGEOM* wkt_parser_compound_add_geom(LWGEOM *col, LWGEOM *geom);
LWGEOM* wkt_parser_collection_new(LWGEOM *geom);
LWGEOM* wkt_parser_collection_add_geom(LWGEOM *col, LWGEOM *geom);
LWGEOM* wkt_parser_collection_finalize(int lwtype, LWGEOM *col, char *dimensionality);
void    wkt_parser_geometry_new(LWGEOM *geom, int srid);

