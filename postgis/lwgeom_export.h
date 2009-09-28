/**********************************************************************
 * $Id:$
 *
 * PostGIS - Export functions for PostgreSQL/PostGIS
 * Copyright 2009 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/**
 * Commons define and prototype function for all export functions 
 */

#define SHOW_DIGS_DOUBLE 15
#define MAX_DOUBLE_PRECISION 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 2) /* +2 mean add dot and sign */

char * getSRSbySRID(int SRID, bool short_crs);

char *geometry_to_geojson(uchar *srl, char *srs, bool has_bbox, int precision);
char *geometry_to_gml2(uchar *srl, char *srs, int precision);
char *geometry_to_gml3(uchar *srl, char *srs, int precision, bool is_deegree);
char *geometry_to_kml2(uchar *srl, int precision);
char *geometry_to_svg(uchar *srl, bool relative, int precision);
