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

#include <ctype.h>

#include "libgeom.h"

/* Structure for the type array */
struct geomtype_struct
{
	char *typename;
	int type;
	int z;
	int m;
};

/* Type array. Note that the order of this array is important in
   that any typename in the list must *NOT* occur within an entry
   before it. Otherwise if we search for "POINT" at the top of the
   list we would also match MULTIPOINT, for example. */

struct geomtype_struct geomtype_struct_array[32] = 
{
	{ "GEOMETRYCOLLECTIONZM", COLLECTIONTYPE, 1, 1 },
	{ "GEOMETRYCOLLECTIONZ", COLLECTIONTYPE, 1, 0 },
	{ "GEOMETRYCOLLECTIONM", COLLECTIONTYPE, 0, 1 },
	{ "GEOMETRYCOLLECTION", COLLECTIONTYPE, 0, 0 },
	{ "GEOMETRYZM", 0, 1, 1 },
	{ "GEOMETRYZ", 0, 1, 0 },
	{ "GEOMETRYM", 0, 0, 1 },
	{ "GEOMETRY", 0, 0, 0 },
	{ "MULTILINESTRINGZM", MULTILINETYPE, 1, 1 },
	{ "MULTILINESTRINGZ", MULTILINETYPE, 1, 0 },
	{ "MULTILINESTRINGM", MULTILINETYPE, 0, 1 },
	{ "MULTILINESTRING", MULTILINETYPE, 0, 0 },
	{ "MULTIPOLYGONZM", MULTIPOLYGONTYPE, 1, 1 },
	{ "MULTIPOLYGONZ", MULTIPOLYGONTYPE, 1, 0 },
	{ "MULTIPOLYGONM", MULTIPOLYGONTYPE, 0, 1 },
	{ "MULTIPOLYGON", MULTIPOLYGONTYPE, 0, 0 },
	{ "MULTIPOINTZM", MULTIPOINTTYPE, 1, 1 },
	{ "MULTIPOINTZ", MULTIPOINTTYPE, 1, 0 },
	{ "MULTIPOINTM", MULTIPOINTTYPE, 0, 1 },
	{ "MULTIPOINT", MULTIPOINTTYPE, 0, 0 },
	{ "LINESTRINGZM", LINETYPE, 1, 1 },
	{ "LINESTRINGZ", LINETYPE, 1, 0 },
	{ "LINESTRINGM", LINETYPE, 0, 1 },
	{ "LINESTRING", LINETYPE, 0, 0 },
	{ "POLYGONZM", POLYGONTYPE, 1, 1 },
	{ "POLYGONZ", POLYGONTYPE, 1, 0 },
	{ "POLYGONM", POLYGONTYPE, 0, 1 },
	{ "POLYGON", POLYGONTYPE, 0, 0 },
	{ "POINTZM", POINTTYPE, 1, 1 },
	{ "POINTZ", POINTTYPE, 1, 0 },
	{ "POINTM", POINTTYPE, 0, 1 },
	{ "POINT", POINTTYPE, 0, 0 }
};


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
	char *tmpstr;
	int tmpstartpos, tmpendpos;
	int i;

	assert(str);
	assert(type);
	assert(z);
	assert(m);

	/* Initialize. */
	*type = 0;
	*z = 0;
	*m = 0;

	/* Locate any leading/trailing spaces */
	tmpstartpos = 0;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] != ' ')
		{
			tmpstartpos = i;
			break;		
		}
	}	

	tmpendpos = strlen(str) - 1;
	for (i = strlen(str) - 1; i >= 0; i--)
	{
		if (str[i] != ' ')
		{
			tmpendpos = i;
			break;		
		}
	}	

	/* Copy and convert to upper case for comparison */
	tmpstr = lwalloc(tmpendpos - tmpstartpos + 2);
	for (i = tmpstartpos; i <= tmpendpos; i++)
		tmpstr[i - tmpstartpos] = toupper(str[i]);

	/* Add NULL to terminate */
	tmpstr[i - tmpstartpos] = '\0';

	/* Now check for the type */
	for (i = 0; i < 32; i++)
	{
		if (!strcmp(tmpstr, geomtype_struct_array[i].typename))
		{
			*type = geomtype_struct_array[i].type;
			*z = geomtype_struct_array[i].z;
			*m = geomtype_struct_array[i].m;

			lwfree(tmpstr);

			return G_SUCCESS;
		}
 
	} 

	lwfree(tmpstr);

	return G_FAILURE;
}




