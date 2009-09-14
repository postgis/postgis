/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "libgeom.h"

uchar gflags(int hasz, int hasm, int geodetic)
{
	unsigned char flags = 0;
	if ( hasz ) 
		flags = FLAGS_SET_Z(flags, 1);
	if ( hasm ) 
		flags = FLAGS_SET_M(flags, 1);
	if ( geodetic ) 
		flags = FLAGS_SET_GEODETIC(flags, 1);
	return flags;
}

/**
* Calculate type integer and dimensional flags from string input.
* Case insensitive, and insensitive to spaces at front and back.
* Type == 0 in the case of the string "GEOMETRY" or "GEOGRAPHY".
* Return G_SUCCESS for success.
*/
int geometry_type_from_string(char *str, int *type, int *z, int *m)
{
	regex_t rx_point;
	regex_t rx_linestring;
	regex_t rx_polygon;
	regex_t rx_geometrycollection;
	regex_t rx_geometry;
	size_t rx_nmatch = 5;
	regmatch_t rx_matchptr[5];

	assert(str);
	assert(type);
	assert(z);
	assert(m);

	/* Initialize. */
	*type = 0;
	*z = 0;
	*m = 0;

	regcomp(&rx_point, "^ *(st_)?(multi)?point(z)?(m)? *$", REG_ICASE | REG_EXTENDED);
	regcomp(&rx_linestring, "^ *(st_)?(multi)?linestring(z)?(m)? *$", REG_ICASE | REG_EXTENDED);
	regcomp(&rx_polygon, "^ *(st_)?(multi)?polygon(z)?(m)? *$", REG_ICASE | REG_EXTENDED);
	regcomp(&rx_geometrycollection, "^ *(st_)?geometrycollection(z)?(m)? *$", REG_ICASE | REG_EXTENDED);
	regcomp(&rx_geometry, "^ *(st_)?geometry(z)?(m)? *$", REG_ICASE | REG_EXTENDED);

	if( ! regexec(&rx_point, str, rx_nmatch, rx_matchptr, 0) )
	{
		*type = POINTTYPE;
		if(rx_matchptr[2].rm_so != -1) /* MULTI */
			*type = MULTIPOINTTYPE;
			
		if(rx_matchptr[3].rm_so != -1) /* Z */
			*z = 1;
			
		if(rx_matchptr[4].rm_so != -1) /* M */
			*m = 1;

		return G_SUCCESS;
	}
	if( ! regexec(&rx_linestring, str, rx_nmatch, rx_matchptr, 0) )
	{
		*type = LINETYPE;
		if(rx_matchptr[2].rm_so != -1) /* MULTI */
			*type = MULTILINETYPE;
			
		if(rx_matchptr[3].rm_so != -1) /* Z */
			*z = 1;
			
		if(rx_matchptr[4].rm_so != -1) /* M */
			*m = 1;

		return G_SUCCESS;
	}
	if( ! regexec(&rx_polygon, str, rx_nmatch, rx_matchptr, 0) )
	{
		*type = POLYGONTYPE;
		if(rx_matchptr[2].rm_so != -1) /* MULTI */
			*type = MULTIPOLYGONTYPE;
			
		if(rx_matchptr[3].rm_so != -1) /* Z */
			*z = 1;
			
		if(rx_matchptr[4].rm_so != -1) /* M */
			*m = 1;

		return G_SUCCESS;
	}
	if( ! regexec(&rx_geometrycollection, str, rx_nmatch, rx_matchptr, 0) )
	{
		*type = COLLECTIONTYPE;

		if(rx_matchptr[2].rm_so != -1) /* Z */
			*z = 1;
			
		if(rx_matchptr[3].rm_so != -1) /* M */
			*m = 1;

		return G_SUCCESS;
	}
	if( ! regexec(&rx_geometry, str, rx_nmatch, rx_matchptr, 0) )
	{
		*type = 0; /* Generic geometry type. */

		if(rx_matchptr[2].rm_so != -1) /* Z */
			*z = 1;
			
		if(rx_matchptr[3].rm_so != -1) /* M */
			*m = 1;

		return G_SUCCESS;
	}
	
	return G_FAILURE;

}




