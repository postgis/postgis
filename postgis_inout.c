
/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 * $Log$
 * Revision 1.34  2003/12/12 14:39:04  strk
 * Fixed a bug in make_line allocating less memory then required
 *
 * Revision 1.33  2003/12/12 12:03:30  strk
 * More debugging output, some code cleanup.
 *
 * Revision 1.32  2003/12/09 11:58:42  strk
 * Final touch to wkb binary input function
 *
 * Revision 1.31  2003/12/09 11:13:41  strk
 * WKB_recv: set StringInfo cursor to the end of StringInfo buf as required by postgres backend
 *
 * Revision 1.30  2003/12/08 17:57:36  strk
 * Binary WKB input function built only when USE_VERSION > 73. Making some modifications based on reported failures
 *
 * Revision 1.29  2003/11/28 11:06:49  strk
 * Added WKB_recv function for binary WKB input
 *
 * Revision 1.28  2003/10/06 18:09:08  dblasby
 * Fixed typo in add_to_geometry().  With very poorly aligned sub-objects, it
 * wouldnt allocate enough memory.  Fixed it so its pesimistic and will allocate
 * enough memory.
 *
 * Revision 1.27  2003/08/22 17:40:11  dblasby
 * fixed geometry_in('SRID=<int>{no ;}').
 *
 * Revision 1.26  2003/08/21 16:22:09  dblasby
 * added patch from strk@freek.keybit.net  for PG_NARGS() not being in 7.2
 *
 * Revision 1.25  2003/08/08 18:19:20  dblasby
 * Conformance changes.
 * Removed junk from postgis_debug.c and added the first run of the long
 * transaction locking support.  (this will change - dont use it)
 * conformance tests were corrected
 * some dos cr/lf removed
 * empty geometries i.e. GEOMETRYCOLLECT(EMPTY) added (with indexing support)
 * pointN(<linestring>,1) now returns the first point (used to return 2nd)
 *
 * Revision 1.24  2003/08/06 19:31:18  dblasby
 * Added the WKB parser.  Added all the functions like
 * PolyFromWKB(<WKB>,[<SRID>]).
 *
 * Added all the functions like PolyFromText(<WKT>,[<SRID>])
 *
 * Minor problem in GEOS library fixed.
 *
 * Revision 1.23  2003/07/25 17:08:37  pramsey
 * Moved Cygwin endian define out of source files into postgis.h common
 * header file.
 *
 * Revision 1.22  2003/07/08 18:35:54  dblasby
 * changed asbinary_specify() so that it is more aware of TEXT being
 * un-terminated.
 *
 * this is a modified patch from David Garnier <david.garnier@etudier-online.com>.
 *
 * Revision 1.21  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 *
 **********************************************************************/

#include "postgres.h"


#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"


#include "fmgr.h"


#include "postgis.h"
#if USE_VERSION > 73
# include "lib/stringinfo.h" // for WKB binary input
#endif
#include "utils/elog.h"


#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


// #define DEBUG_GIST
//#define DEBUG_GIST2


#define WKB3DOFFSET 0x80000000


void swap_char(char	*a,char *b)
{
	char c;

	c = *a;
	*a=*b;
	*b=c;
}


void	flip_endian_double(char 	*d)
{
	swap_char(d+7, d);
	swap_char(d+6, d+1);
	swap_char(d+5, d+2);
	swap_char(d+4, d+3);
}

void		flip_endian_int32(char		*i)
{
	swap_char (i+3,i);
	swap_char (i+2,i+1);
}



//takes a point and sticks it at the end of a string
void print_point(char *result, POINT3D *pt,bool is3d)
{
	char	temp[MAX_DIGS_DOUBLE*3 +6];


	if ( (pt == NULL) || (result == NULL) )
		return;

	if (is3d)
		sprintf(temp, "%.15g %.15g %.15g", pt->x,pt->y,pt->z);
	else
		sprintf(temp, "%.15g %.15g", pt->x,pt->y);

	strcat(result,temp);

}

//result MUST BE null terminated (or result[0] = 0)
void print_many_points(char *result, POINT3D *pt ,int npoints, bool is3d)
{
	int	u;

	result += strlen(result);
	if (is3d)
	{
		for (u=0;u<npoints;u++)
		{
			if (u != 0)
			{
				result[0] = ',';
				result++;
			}
			result+=	sprintf(result,"%.15g %.15g %.15g", pt[u].x,pt[u].y,pt[u].z);
		}
	}
	else
	{
		for (u=0;u<npoints;u++)
		{
			if (u != 0)
			{
				result[0] = ',';
				result++;
			}

			result += sprintf(result, "%.15g %.15g",
					pt[u].x, pt[u].y);
		}
	}
}


//swap two doubles
void swap(double *d1, double *d2)
{
	double t;

	t = *d1;
	*d1 = *d2;
	*d2 = t;
}


//returns how many points are in the first list in str
//
//  1. scan ahead looking for "("
//  2. find "," until hit a ")"
//  3. return number of points found
//
// NOTE: doesnt actually parse the points, so if the
//       str contains an invalid geometry, this could give
// 	   back the wrong answer.
//
// "(1 2 3, 4 5 6),(7 8, 9 10, 11 12 13)" => 2 (2nd list is not included)
int	numb_points_in_list(char	*str)
{
	bool	keep_going;
	int	points_found = 1; //no "," if only one point (and last point)
					// # points = 1 + numb of ","

	if ( (str == NULL) || (str[0] == 0) )
	{
		return 0;  //either null string or empty string
	}

	//look ahead for the "("

	str = strchr(str,'(') ;

	if ( (str == NULL) || (str[1] == 0) )  // str[0] = '(';
	{
		return 0;  //either didnt find "(" or its at the end of the string
	}

	keep_going = TRUE;
	while (keep_going)
	{
		str=strpbrk(str,",)");  // look for a "," or ")"
		keep_going = (str != NULL);
		if (keep_going)  // found a , or )
		{
			if (str[0] == ')')
			{
				//finished
				return points_found;
			}
			else	//str[0] = ","
			{
				points_found++;
				str++; //move 1 char forward
			}
		}
	}
	return points_found; // technically it should return an error.
}

//Actually parse the points in a list
//  "(1 2 3, 4 5 6),(7 8, 9 10, 11 12 13)" =>
//			points[0] = {1,2,3} and points[1] = {4,5,6}
//		(2nd list is not parsed)
//
// parse at most max_points (points should already have enough space)
// if any of the points are 3d, is3d is set to true, otherwise its untouched.
//	2d points have z=0.0
//
// returns true if everything parses okay, otherwise false

bool	parse_points_in_list(char	*str, POINT3D	*points, int32	max_points, bool *is3d)
{
	bool	keep_going;
	int	numb_found= 0;
	int	num_entities;

	if ( (str == NULL) || (str[0] == 0)  || (max_points <0) || (points == NULL) )
	{
		return FALSE;  //either null string or empty string or other problem
	}

	if (max_points == 0)
		return TRUE; //ask for nothing, get nothing

	//look ahead for the "("

	str = strchr(str,'(') ;

	if ( (str == NULL) || (str[1] == 0) )  // str[0] = '(';
	{
		return FALSE;  //either didnt find "(" or its at the end of the string
	}
	str++;  //move forward one char

	keep_going = TRUE;
	while (keep_going)
	{
		//attempt to get the point
	      	num_entities = sscanf(str,"%le %le %le",  &(points[numb_found].x),
									      &(points[numb_found].y),
								     	      &(points[numb_found].z));

			if (num_entities !=3)
			{
				if (num_entities !=2 )
				{
					elog(ERROR, "geom3d: parse_points_in_list() on invalid point");
					return FALSE; //error
				}
				else
				{
					points[numb_found].z = 0.0; //2d (only found x,y - set z =0.0)
				}
			}
			else
			{
				*is3d = TRUE; //found 3 entites (x,y,z)
			}
			numb_found++;



		str=strpbrk(str,",)");  // look for a "," or ")"
		if (str != NULL)
			str++;
		keep_going =  ( (str != NULL) && (str[0] != ')' ) && (numb_found < max_points)  );
	}
	return TRUE;
}


//Actually parse the points in a list
//  "(1 2 3, 4 5 6),(7 8, 9 10, 11 12 13)" =>
//			points[0] = {1,2,3} and points[1] = {4,5,6}
//		(2nd list is not parsed)
//
// parse at most max_points (points should already have enough space)
// if any of the points are 3d, is3d is set to true, otherwise its untouched.
//	2d points have z=0.0
//
// returns true if everything parses okay, otherwise false
//
// THIS IS EXACTLY the same as parse_points_in_list(), but returns FALSE if
// the number of points parsed is != max_points

bool	parse_points_in_list_exact(char	*str, POINT3D	*points, int32	max_points, bool *is3d)
{
	bool	keep_going;
	int	numb_found= 0;

	char	*end_of_double;

	if ( (str == NULL) || (str[0] == 0)  || (max_points <0) || (points == NULL) )
	{
		return FALSE;  //either null string or empty string or other problem
	}

	if (max_points == 0)
		return TRUE; //ask for nothing, get nothing

	//look ahead for the "("

	str = strchr(str,'(') ;

	if ( (str == NULL) || (str[1] == 0) )  // str[0] = '(';
	{
		return FALSE;  //either didnt find "(" or its at the end of the string
	}
	str++;  //move forward one char

	keep_going = TRUE;
	while (keep_going)
	{
		//attempt to get the point

			//scanf is slow, so we use strtod()


			points[numb_found].x = strtod(str,&end_of_double);
			if (end_of_double == str)
			{
				return FALSE; //error occured (nothing parsed)
			}
			str = end_of_double;
			points[numb_found].y = strtod(str,&end_of_double);
			if (end_of_double == str)
			{
				return FALSE; //error occured (nothing parsed)
			}
			str = end_of_double;
			points[numb_found].z = strtod(str,&end_of_double); //will be zero if error occured
			if (!(end_of_double == str))
			{
				*is3d = TRUE; //found 3 entites (x,y,z)
			}
			str = end_of_double;
			numb_found++;



		str=strpbrk(str,",)");  // look for a "," or ")"
		if (str != NULL)
			str++;
		keep_going =  ( (str != NULL) && (str[0] != ')' ) && (numb_found < max_points)  );
	}
	return (numb_found == max_points);
}




// Find the number of sub lists in a list
// ( (..),(..),(..) )  -> 3
// ( ( (..),(..) ), ( (..) )) -> 2
// ( ) -> 0
// scan through the list, for every "(", depth (nesting) increases by 1
//				  for every ")", depth (nesting) decreases by 1
// if find a "(" at depth 1, then there is a sub list
//
// example:
//      "(((..),(..)),((..)))"
//depth  12333223332112333210
//        +           +         increase here

int	find_outer_list_length(char *str)
{
	int	current_depth = 0;
	int	numb_lists = 0;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"()"); //look for "(" or ")"
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				current_depth++;
				if (current_depth == 2)
					numb_lists ++;
			}
			if (str[0] == ')')
			{
				current_depth--;
				if (current_depth == 0)
					return numb_lists ;
			}
			str++;
		}
	}
	return numb_lists ; // probably should give an error
}


// Find out how many points are in each sublist, put the result in the array npoints[]
//  (for at most max_list sublists)
//
//  ( (L1),(L2),(L3) )  --> npoints[0] = points in L1,
//				    npoints[1] = points in L2,
//				    npoints[2] = points in L3
//
// We find these by, again, scanning through str looking for "(" and ")"
// to determine the current depth.  We dont actually parse the points.

bool	points_per_sublist( char *str, int32 *npoints, int32 max_lists)
{
	//scan through, noting depth and ","s

	int	current_depth = 0;
	int	current_list =-1 ;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"(),");  //find "(" or ")" or ","
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				current_depth++;
				if (current_depth == 2)
				{
					current_list ++;
					if (current_list >=max_lists)
						return TRUE;			// too many sub lists found
					npoints[current_list] = 1;
				}
				// might want to return an error if depth>2
			}
			if (str[0] == ')')
			{
				current_depth--;
				if (current_depth == 0)
					return TRUE ;
			}
			if (str[0] == ',')
			{
				if (current_depth==2)
				{
					npoints[current_list] ++;
				}
			}

			str++;
		}
	}
	return TRUE ; // probably should give an error
}




//simple scan-forward to find the next "(" at the same level
//  ( (), (),(), ),(...
//                 + return this location
char	*scan_to_same_level(char	*str)
{

	//scan forward in string looking for at "(" at the same level
	// as the one its already pointing at

		int	current_depth = 0;
		bool  first_one=TRUE;


	while ( (str != NULL) && (str[0] != 0) )
	{
		str=strpbrk(str,"()");
		if (str != NULL)
		{
			if (str[0] == '(')
			{
				if (!(first_one))
				{
					if (current_depth == 0)
						return str;
				}
				else
					first_one = FALSE;  //ignore the first opening "("
				current_depth++;
			}
			if (str[0] == ')')
			{
				current_depth--;
			}

			str++;
		}
	}
	return str ; // probably should give an error
}


/*
 *  BOX3D_in - takes a string rep of BOX3D and returns internal rep
 *
 *  example:
 *     "BOX3D(x1 y1 z1,x2 y2 z2)"
 * or  "BOX3D(x1 y1,x2 y2)"   z1 and z2 = 0.0
 *
 *
 */

BOX3D	*parse_box3d(char *str)
{

	BOX3D	   *bbox = (BOX3D *) palloc(sizeof(BOX3D));
	bool		junk_bool;
	bool	   okay;
	int 		npoints;


	//verify that there are exactly two points

	//strip leading spaces
	while (isspace((unsigned char) *str))
		str++;
//printf( "box3d_in gets '%s'\n",str);

	if (strstr(str,"BOX3D") !=  str )
	{
		 elog(ERROR,"BOX3D parser - doesnt start with BOX3D");
		 pfree(bbox);
		 return NULL;
	}

	if ((npoints = numb_points_in_list(str)) != 2)
	{
//printf("npoints in this bbox is %i, should be 2\n",npoints);
		 elog(ERROR,"BOX3D parser - number of points should be exactly 2");
		 pfree(bbox);
		 return NULL;
	}

	//want to parse two points, and dont care if they are 2d or 3d
	okay = 	parse_points_in_list(str, &(bbox->LLB), 2, &junk_bool);
	if (okay == 0)
	{
		elog(ERROR,"box3d: couldnt parse: '%s'\n",str);
		pfree(bbox);
		return NULL;
	}


	//validate corners so LLB is mins of x,y,z and URT is maxs x,y,z
	if (	bbox->LLB.x > bbox->URT.x)
	{
		swap ( &bbox->LLB.x , &bbox->URT.x ) ;
	}
	if (	bbox->LLB.y > bbox->URT.y)
	{
		swap ( &bbox->LLB.y , &bbox->URT.y );
	}
	if (	bbox->LLB.z > bbox->URT.z)
	{
		swap ( &bbox->LLB.z , &bbox->URT.z ) ;
	}

	return bbox;
}

PG_FUNCTION_INFO_V1(box3d_in);
Datum box3d_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	BOX3D	   *bbox ;

	bbox = parse_box3d(str);
	if (bbox != NULL)
		PG_RETURN_POINTER(bbox);
	else
		PG_RETURN_NULL();
}

/*
 *  Takes an internal rep of a BOX3D and returns a string rep.
 *
 *  example:
 *     "BOX3D(LLB.x LLB.y LLB.z, URT.x URT.y URT.z)"
 */
PG_FUNCTION_INFO_V1(box3d_out);
Datum box3d_out(PG_FUNCTION_ARGS)
{
	BOX3D  *bbox = (BOX3D *) PG_GETARG_POINTER(0);
	int size;
	char	*result;

	if (bbox == NULL)
	{
		result = palloc(5);
		strcat(result,"NULL");
		PG_RETURN_CSTRING(result);
	}

	size = MAX_DIGS_DOUBLE*6+5+2+4+5+1;
	result = (char *) palloc(size); //double digits+ "BOX3D"+ "()" + commas +null
	sprintf(result, "BOX3D(%.15g %.15g %.15g,%.15g %.15g %.15g)",
			bbox->LLB.x,bbox->LLB.y,bbox->LLB.z,
			bbox->URT.x,bbox->URT.y,bbox->URT.z);

	PG_RETURN_CSTRING(result);
}

/***************************************************************
 * these functions return the number of sub-objects inside a
 * sub-portion of a string.
 *
 * LINESTRING(), POLYGON(), POINT() allways have 1 sub object
 *
 * MULTIPOLYGON() MULTILINESTRING() MULTIPOINT() will have a variable number of sub-parts
 *           ex MULTIPOINT(1 1 1, 2 2 2,3 3 3,4 4 4) -->4 because there are 4 points in it
 *
 * GEOMETRYCOLLECTION() - need to look at each sub-object in the collection
 ***************************************************************/

//number of object inside a point
int objects_inside_point(char *str)
{
	return 1; //trivial points always have 1 point in them
}


//number of object inside a line
int objects_inside_line(char *str)
{
	return 1; //trivial lines always have 1 line in them
}

//number of object inside a polygon
int objects_inside_polygon(char *str)
{
	return 1; //trivial polygon always have 1 polygon in them
}


//bit more complicated - need to find out how many points are in this entity
int objects_inside_multipoint(char *str)
{
	return numb_points_in_list(str);
}

//bit more complicated - need to find out how many lines are in this entity
int objects_inside_multiline(char *str)
{
	return find_outer_list_length(str);
}


//bit more complicated - need to find out how many polygons are in this entity
int objects_inside_multipolygon(char *str)
{
	return find_outer_list_length(str);
}



//This one is more complicated, we have to look at each of the sub-object inside
// the collection.
//  result = sum of ( number of objects in each of the collection's sub-objects )
//
// EX:   'GEOMETRYCOLLECTION( POINT(1 2 3),
//	POINT (4 5 6) ),
//	LINESTRING(3 4 5,1 2 3,5 6 0),
//	MULTILINESTRING((3 4 5,1 2 3,5 6 0)),
//	MULTIPOINT(1 2 3),
//	MULTILINESTRING((3 4 5,1 2 3,5 6 0),(1 2 0,4 5 0,6 7 0,99 99 0)),
//	MULTIPOLYGON ((  (3 4 5,1 2 3,5 6),(1 2, 4 5, 6 7)  ), ( (11 11, 22 22, 33 33),(44 44, 55 55, 66 66) ) )'
//
//	# object = # of objs in POINT (1) + # of objs in POINT (1) +   # of objs in LINESTRING (1)
//			+ # of objs in MULTILINESTRING (1) + # of objs in MULTIPOINT  (1)
//			+ # of objs in MULTILINESTRING (1) + # of objs in  MULTIPOLYGON  (2)

int objects_inside_collection(char *str)
{
	int		tally = 0;
	int		sub_size = 0;

	//skip past the "geometrycollection" at begining of string
	str += 18;
	if (strstr(str, "GEOMETRYCOLLECTION") != NULL)
		return -1; // collection inside collection; bad boy!

	//we scan to the first letter, then pass it to objects inside
 	// to find out how many sub-objects there are.  Then move to
 	// the next object


	while ( str != NULL)
	{
		//scan for a letter  (all objs start with M, P, or L
		str = strpbrk(str,"MPL"); //search for MULTI*, POLY*, POIN*
		if (str != NULL)
		{
			//object found
			sub_size = objects_inside(str); //how many in this object
			if (sub_size == -1)
				return -1;//relay error

			tally += sub_size;	// running total

			//need to move to next item
			// by scanning past any letters
			str = strchr(str,'(');  //need to move to the next sub-object
		}
	}
	return tally;
}


// Find the # of sub-objects inside the geometry
// 	Just pass it off to the other functions
int objects_inside(char *str)
{

	char		*parenth;
	char		*loc;


	parenth = strchr(str,'(');
	if (parenth == NULL)
		return -1; //invalid string

	// look only at the begining of the string
	// the order of these "if" are important!

	if (  ((loc = strstr(str, "GEOMETRYCOLLECTION")) != NULL) && (loc < parenth) )
		return objects_inside_collection(str);

	// do multi before base types (MULTIPOINT has the string "POINT" in it)

	if (((loc = strstr(str,"MULTIPOINT")) != NULL) && (loc < parenth) )
		return objects_inside_multipoint(str);
	if (((loc = strstr(str,"MULTILINESTRING") )!= NULL) && (loc < parenth) )
		return objects_inside_multiline(str);
	if (((loc = strstr(str,"MULTIPOLYGON")) != NULL) && (loc < parenth) )
		return objects_inside_multipolygon(str);

	//base types
	if (((loc = strstr(str,"POINT")) != NULL) && (loc < parenth) )
		return objects_inside_point(str);
	if (((loc = strstr(str,"LINESTRING")) != NULL) && (loc < parenth) )
		return objects_inside_line(str);
	if (((loc = strstr(str,"POLYGON")) != NULL) && (loc < parenth) )
		return objects_inside_polygon(str);

	return -1; //invalid

}


/****************************************************************************
 *  These functions actually parse the objects in the strings
 *  They all have arguments:
 * 	obj_size[]  -- size (in bytes) of each the sub-objects
 *    objs[][]    -- list of pointers to the actual objects
 *    obj_types   -- type of the sub-obj (see postgis.h)
 *    nobjs       -- max number of sub-obj
 *    str		-- actual string to parse from (start at begining)
 *    offset      -- which sub-object # are we dealing with
 *    is3d  	-- are any of the points in the object 3d?
 *
 *    basically, each of the functions will:
 *		+ parse str to find the information required
 *		+ allocate a new obj(s) and place it/them in objs to hold this info
 *		+ set the obj_size for this/these sub-object
 *		+ set the obj_type for this/these sub-object
 *		+ increase offset so the next objects will go in the correct place
 *		+ set is3d if there are any 3d points in the sub-object
 *		+ return FALSE if an error occured
 ****************************************************************************/


//parse a point - make a new POINT3D object
bool parse_objects_inside_point(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d)
{
	bool	result;

	if (*offset >= nobjs)
		return FALSE;     //too many objs found

	objs[*offset] = palloc (sizeof(POINT3D) );
	memset(objs[*offset], 0, sizeof(POINT3D) ); // zero memory
	obj_types[*offset] = POINTTYPE;
	obj_size[*offset] = sizeof(POINT3D);

	str = strchr(str,'(');
	if (str == NULL)
		return FALSE;  // parse error

//printf("about to parse the point\n");

	result = parse_points_in_list_exact(str, (POINT3D*) objs[*offset] , 1, is3d);
//printf("	+point parsed\n");

	*offset = *offset + 1;
	return (result); // pass along any parse error
}


//parse a multipoint object - offset may increase by >1
// multiple sub object may be created
bool parse_objects_inside_multipoint(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d)
{
	bool	result;
	int	npoints;
	int	t;
	POINT3D	*pts;

	// first find out how many points are in the multi-point
	npoints = objects_inside_multipoint(str);

	if (npoints <0)
		return FALSE; // error in parsing (should have been caught before)

	if (npoints ==0)
		return TRUE;  //no points, no nothing

	if (*offset >= nobjs)
		return FALSE;     //too many objs found dont do anything, relay error


	str = strchr(str,'(');
	if (str == NULL)
		return FALSE;

		//allocate all the new objects
	pts = palloc( sizeof(POINT3D) * npoints);
	for (t=0;t<npoints; t++)
	{
		objs[*offset + t] = palloc( sizeof(POINT3D) );
		memset(objs[*offset+t], 0, sizeof(POINT3D) ); // zero memory

		obj_types[*offset+t] = POINTTYPE;
		obj_size[*offset+t] = sizeof(POINT3D);
	}

	//actually do the parsing into a temporary list
	result = parse_points_in_list_exact(str, pts , npoints, is3d);


	if (!(result))
	{
		pfree(pts);
		return FALSE; //relay error
	}

	for(t=0;t<npoints;t++)
	{
		memcpy(objs[*offset+t], &pts[t], sizeof(POINT3D) );
	}

	pfree(pts);
	*offset = *offset + npoints;
	return (result);
}


//parse a linestring
bool parse_objects_inside_line(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d)
{
	bool	result;
	int	num_points;
	bool	add_point = FALSE; // only 1 point --> make it two on top of each other


	if (*offset >= nobjs)
		return FALSE;     //too many objs found

	//how many points are in this linestring?
	num_points = numb_points_in_list(str);

	if (num_points == 0)
		return FALSE;


	if (num_points == 1)	//not really a line
	{
		num_points  = 2;
		add_point = TRUE;
	}



							// -1 because object already has 1 point in it
		objs[*offset] = palloc (sizeof(LINE3D) + sizeof(POINT3D)*(num_points-1) );
		memset(objs[*offset], 0, sizeof(LINE3D) + sizeof(POINT3D)*(num_points-1)  ); // zero memory

	obj_types[*offset] = LINETYPE;
	obj_size[*offset] = sizeof(LINE3D) + sizeof(POINT3D)*(num_points-1) ;

	str = strchr(str,'(');
	if (str == NULL)
		return FALSE;

	( (LINE3D *) objs[*offset] )->npoints = num_points;

	if (add_point)
	{
		result = parse_points_in_list_exact(str, &(( (LINE3D*) objs[*offset]) ->points[0]) , num_points-1, is3d);
	}
	else
	{
		result = parse_points_in_list_exact(str, &(( (LINE3D*) objs[*offset]) ->points[0]) , num_points, is3d);
	}

	if (add_point)
	{
		memcpy( &(( (LINE3D*) objs[*offset]) ->points[1])   , &(( (LINE3D*) objs[*offset]) ->points[0])  , sizeof(POINT3D) );
	}

	*offset = *offset + 1;
	return (result);
}



//parse a multi-line.  For each sub-linestring, call the parse_objects_inside_line() function
// 	'MULTILINESTRING((3 4 5,1 2 3,5 6 0),(1 2 0,4 5 0,6 7 0,99 99 0))'
// first scan to the second "(" - this is the begining of the first linestring.
// scan it, then move to the second linestring
bool parse_objects_inside_multiline(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d)
{
	bool	result;
	int	num_lines;
	int 	t;


		//how many lines are there?
	num_lines = objects_inside_multiline(str);


	if (num_lines <0)
		return FALSE;

	if (num_lines ==0)
		return TRUE;	//can be nothing


	if (*offset >= nobjs)
		return FALSE;     //too many objs found

	str= strchr(str,'('); //first one
	if (str != NULL)
		str++;
	str= strchr(str,'('); //2nd one

	if (str == NULL)
		return FALSE;  // couldnt find it

	for (t=0;t<num_lines; t++)
	{
			//pass along the work - it will update offset for us
		result = parse_objects_inside_line(obj_size,objs, obj_types,nobjs, str,offset,is3d);
		if (!(result))
			return FALSE;//relay error
		if (str != NULL)
			str++;  // go past the first '(' in the linestring
		else
			return FALSE; //parse error
		str = strchr(str,'('); // go to the beginning of the next linestring
	}

	return (TRUE);  //everything okay
}

//parse a polygon
//	POLYGON (  (3 4 5,1 2 3,5 6),(1 2, 4 5, 6 7)  )'


bool parse_objects_inside_polygon(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d)
{
	bool	result;
	int	num_rings;
	int32	*npoints;
	int32	sum_points,points_offset;
	int	t;
	int	size;
	POINT3D	*pts;
	POLYGON3D	*polygon;



	if (*offset >= nobjs)
		return FALSE;     //too many objs found

		//find out how many rings there are
	num_rings = find_outer_list_length(str);

	if (num_rings <1)
		return FALSE; // must have at least one ring

		//allocate space for how many points in each ring
	npoints = palloc( sizeof(int32) * num_rings);

		//find out how many points are in each sub-list (ring)
	result = points_per_sublist(str,npoints, num_rings);

		//how many points are inside this polygon in total?
	sum_points =0;
	for(t=0;  t< num_rings ; t++)
	{
		sum_points += npoints[t];
		if (npoints[t] <3)
		{
			elog(ERROR,"polygon has ring with <3 points.");
			pfree(npoints);
			return FALSE;
		}
	}

		//now we can allocate the structure


			// -1 because the POLYGON3D already has space for 1 ring with 1 point
			// 4 is for possible double-word alignment
	size = sizeof(POLYGON3D) + sizeof(int32)*(num_rings-1) +4+ sizeof(POINT3D)*(sum_points-1)  ;
	objs[*offset] = palloc (size);
		memset(objs[*offset], 0, size  ); // zero memory

	polygon = (POLYGON3D *) objs[*offset];
	obj_types[*offset] = POLYGONTYPE;
	obj_size[*offset] = size;

	str = strchr(str,'(');
	if (str == NULL)
	{
		pfree(npoints);
		return FALSE;
	}
	str++;

	polygon->nrings = num_rings;

		pts = (POINT3D *)  &(polygon->npoints[num_rings]); //pts[] is just past the end of npoints[]

			// make sure its double-word aligned

		pts = (POINT3D *) MAXALIGN(pts);


//printf("POLYGON is at %p, size is %i, pts is at %p (%i)\n",objs[*offset],size, pts, ((void *) pts) - ((void *)objs[*offset]) );
//printf("npoints[0] is at %p, num_rings=%i\n", &(((POLYGON3D *) objs[*offset])->npoints[0] ), num_rings );

	points_offset = 0;
	for(t=0;t<num_rings;t++)	//for each ring
	{
		//set num points in this obj
		polygon->npoints[t] = npoints[t]; // set # pts in this ring

			//use exact because it will screw up otherwise
//printf("about to parse the actual points in ring #%i with %i points:\n",t,npoints[t]);
		result = parse_points_in_list_exact(str,
					&pts[points_offset],
					npoints[t], is3d);
//printf("	+done parsing points\n");

//printf("1st points in ring is: [%g,%g,%g]\n", pts[points_offset].x ,pts[points_offset].y ,pts[points_offset].z );

		if (!(result))
		{
			pfree(npoints);
			return FALSE;	//relay error
		}

		//first and last point are the same
		if (!  (FPeq(pts[points_offset].x , pts[points_offset + npoints[t]-1].x  )
			 &&FPeq(pts[points_offset].y , pts[points_offset + npoints[t]-1].y   )
			 &&FPeq(pts[points_offset].z , pts[points_offset + npoints[t]-1].z   )    )   )
		{
			elog(ERROR,"polygon has ring where first point != last point");
			pfree(npoints);
			return FALSE; //invalid poly
		}


		points_offset += npoints[t]; //where to stick points in the next ring

		str = strchr(str,'('); // where is the next ring
		if (str == NULL)
		{
			pfree(npoints);
			return FALSE;  //relay parse error
		}
		str++;
	}

	pfree(npoints);
	*offset = *offset + 1;
	return (result);
}


//	MULTIPOLYGON ((  (3 4 5,1 2 3,5 6),(1 2, 4 5, 6 7)  ), ( (11 11, 22 22, 33 33),(44 44, 55 55, 66 66) ) )'
//	pass off work to parse_objects_inside_polygon()

bool parse_objects_inside_multipolygon(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d)
{
	bool	result;
	int	num_polys;
	int 	t;

	num_polys = objects_inside_multipolygon(str);  //how many polygons are inside me?

//printf("parse_objects_inside_multipolygon('%s')\n",str);

	if (num_polys <0)
		return FALSE;

	if (num_polys ==0)
		return TRUE;	//ask for nothing, get nothing

	if (*offset >= nobjs)
		return FALSE;     //too many objs found

	str= strchr(str,'('); //first one (begining of list of polys)
	if (str != NULL)
	{
		str++;
		str= strchr(str,'('); //2nd one (beginning of 1st poly)
	}
	if (str == NULL)
		return FALSE;	//relay error

	for (t=0;t<num_polys; t++)
	{
		if (str == NULL)
			return FALSE; 	//relay error
//printf("parse_inside_multipolygon: parsing a polygon #%i\n",t);
		result = parse_objects_inside_polygon(obj_size,objs, obj_types,nobjs, str,offset,is3d);
		if (!(result))
			return FALSE;	//relay error

		//scan ahead to next "(" start of next polygon
		str = scan_to_same_level(str);
	}
	return (TRUE);
}


// relay work to the above functions
bool parse_objects(int32 *obj_size,char **objs,int32	*obj_types,int32 nobjs,char *str, int *offset, bool *is3d)
{
		char		*parenth;
		char		*loc;


//printf("parse_objects: parsing object offset=%i\n",*offset);

	if (str == NULL)
		return FALSE;

	parenth = strchr(str,'(');
	if  (parenth == NULL)
		return FALSE; //invalid string


	if (  ((loc = strstr(str, "GEOMETRYCOLLECTION")) != NULL) && (loc < parenth) )
		return parse_objects_inside_collection(obj_size,objs, obj_types, nobjs, str, offset, is3d);


	if (((loc = strstr(str,"MULTIPOINT")) != NULL) && (loc < parenth) )
		return parse_objects_inside_multipoint(obj_size,objs, obj_types, nobjs, str, offset, is3d);


	if (((loc = strstr(str,"MULTILINESTRING") )!= NULL) && (loc < parenth) )
		return parse_objects_inside_multiline(obj_size,objs, obj_types, nobjs, str, offset, is3d);


	if (((loc = strstr(str,"MULTIPOLYGON")) != NULL) && (loc < parenth) )
		return parse_objects_inside_multipolygon(obj_size,objs, obj_types, nobjs, str, offset, is3d);

	if (((loc = strstr(str,"POINT")) != NULL) && (loc < parenth) )
		return parse_objects_inside_point(obj_size,objs, obj_types, nobjs, str, offset, is3d);


	if (((loc = strstr(str,"LINESTRING")) != NULL) && (loc < parenth) )
		return parse_objects_inside_line(obj_size,objs, obj_types, nobjs, str, offset, is3d);

	if (((loc = strstr(str,"POLYGON")) != NULL) && (loc < parenth) )
		return parse_objects_inside_polygon(obj_size,objs, obj_types, nobjs, str, offset, is3d);


	return FALSE; //invalid

}



//look for each of the object inside a collection, and send the work to parse it along
bool parse_objects_inside_collection(int32 *obj_size,char **objs, int32 *obj_types, int32 nobjs, char *str, int *offset, bool* is3d)
{
	bool	result = FALSE;

	str += 18; //scan past "geometrycollection"
	if (strstr(str, "GEOMETRYCOLLECTION") != NULL)
		return FALSE; // collection inside collection; bad boy!

	while ( str != NULL)
	{
		//scan for a letter
		str = strpbrk(str,"MPL"); //search for MULTI*, POLY*, POIN*
		if (str != NULL)
		{
			//object found
			result = parse_objects(obj_size,objs,obj_types,nobjs,str, offset, is3d);

			if (result == FALSE)
				return FALSE;//relay error

			//prepare to move to next item
			// by scanning past any letters
			str = strchr(str,'(');
		}
	}
	return result;
}

/**************************************************************
 * Find the bounding box3d of a sub-object
 **************************************************************/

//simple box encloses a point
BOX3D	*bbox_of_point(POINT3D *pt)
{
	BOX3D		*the_box = (BOX3D *) palloc (sizeof(BOX3D));

	the_box->LLB.x = pt->x;
	the_box->LLB.y = pt->y;
	the_box->LLB.z = pt->z;

	the_box->URT.x = pt->x;
	the_box->URT.y = pt->y;
	the_box->URT.z = pt->z;

	return the_box;
}

// box encloses all the points in all the rings
BOX3D	*bbox_of_polygon(POLYGON3D *polygon)
{
		BOX3D		*the_box ;
		int		numb_points =0,i;
		POINT3D	*pts,*pt;

	for (i=0; i<polygon->nrings; i++)
	{
		numb_points +=  polygon->npoints[i];
	}

	if (numb_points <1)
		return NULL;

	pts = (POINT3D *) ( (char *)&(polygon->npoints[polygon->nrings] )  );
	pts = (POINT3D *) MAXALIGN(pts);

	the_box = bbox_of_point(&pts[0]);

	for (i=1; i<numb_points;i++)
	{
		pt = &pts[i];

		if (pt->x < the_box->LLB.x)
			the_box->LLB.x = pt->x;

		if (pt->y < the_box->LLB.y)
			the_box->LLB.y = pt->y;

		if (pt->z < the_box->LLB.z)
			the_box->LLB.z = pt->z;

		if (pt->x > the_box->URT.x)
			the_box->URT.x = pt->x;

		if (pt->y > the_box->URT.y)
			the_box->URT.y = pt->y;

		if (pt->z > the_box->URT.z)
			the_box->URT.z = pt->z;

	}
	return the_box;
}

//box encloses points in line
BOX3D	*bbox_of_line(LINE3D *line)
{
	BOX3D		*the_box;
	POINT3D	*pt;
	int i;

	if (line->npoints <1)
	{
		return NULL;
	}
	the_box = bbox_of_point(&line->points[0]);

	for (i=1;i< line->npoints; i++)
	{
		pt = &line->points[i];
		if (pt->x < the_box->LLB.x)
			the_box->LLB.x = pt->x;

		if (pt->y < the_box->LLB.y)
			the_box->LLB.y = pt->y;

		if (pt->z < the_box->LLB.z)
			the_box->LLB.z = pt->z;

		if (pt->x > the_box->URT.x)
			the_box->URT.x = pt->x;

		if (pt->y > the_box->URT.y)
			the_box->URT.y = pt->y;

		if (pt->z > the_box->URT.z)
			the_box->URT.z = pt->z;
	}
	return the_box;
}

//merge box a and b (expand b)
//if b is null, new one is returned
// otherwise b is returned
BOX3D *union_box3d(BOX3D *a, BOX3D *b)
{
	if (a==NULL)
		return NULL;
	if (b==NULL)
	{
		b=(BOX3D*) palloc(sizeof(BOX3D));
		memcpy(b,a,sizeof(BOX3D) );
		return b;
	}


	if (a->LLB.x < b->LLB.x)
		b->LLB.x = a->LLB.x;
	if (a->LLB.y < b->LLB.y)
		b->LLB.y = a->LLB.y;
	if (a->LLB.z < b->LLB.z)
		b->LLB.z = a->LLB.z;

	if (a->URT.x > b->URT.x)
		b->URT.x = a->URT.x;
	if (a->URT.y > b->URT.y)
		b->URT.y = a->URT.y;
	if (a->URT.z > b->URT.z)
		b->URT.z = a->URT.z;
	return b;
}

BOX3D	*bbox_of_geometry(GEOMETRY *geom)
{
	int	i;
	int32	*offsets;
	char	*obj;
	BOX3D	*result=NULL;
	BOX3D	*a_box;

//printf("bbox_of_geometry(%p)\n", geom);

	if (geom->nobjs <1)
		return NULL;	//bbox of 0 objs is 0

	//where are the objects living?
	offsets = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;


	//for each sub-object
	for(i=0; i<geom->nobjs;i++)
	{
		obj = (char *) geom +offsets[i] ;


		if (geom->objType[i] == POINTTYPE)
		{
//printf("box of a point\n");
			a_box = bbox_of_point(  (POINT3D *) obj);
			result= union_box3d(a_box  ,result);

			if (a_box != NULL)
				pfree (a_box);
		}
		if (geom->objType[i] == LINETYPE)
		{
//printf("box of a line, # points = %i\n",((LINE3D *) obj)->npoints );
			a_box = bbox_of_line(  (LINE3D *) obj);
			result = union_box3d(a_box  ,result);
			if (a_box != NULL)
				pfree (a_box);
		}
		if (geom->objType[i] == POLYGONTYPE)
		{
//printf("box of a polygon\n");
			a_box = bbox_of_polygon(  (POLYGON3D *) obj);
			result =union_box3d(a_box  ,result);
			if (a_box != NULL)
				pfree (a_box);
		}
	}
	return result;
}


//given wkt and SRID, return a geometry
//actually we cheat, postgres will convert the string to a geometry for us...
PG_FUNCTION_INFO_V1(geometry_from_text);
Datum geometry_from_text(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32	   			SRID;

	GEOMETRY	*result;

	if ( ! PG_ARGISNULL(1) )
		SRID = PG_GETARG_INT32(1);
	else
		SRID = -1;


	result = (GEOMETRY *) palloc(geom1->size);
	memcpy(result,geom1, geom1->size);

	if (result != NULL)
	{
		result->SRID = SRID;
	}
	else
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_POINTER(result);
}



// Parse a geometry object
//
// 1. Find information on how many sub-object there are
// 2. create a group of sub-object and populate them
// 3. make a large structure & populate with all the above info.
//
//
PG_FUNCTION_INFO_V1(geometry_in);
Datum geometry_in(PG_FUNCTION_ARGS)
{
		char	   *str = PG_GETARG_CSTRING(0);
		GEOMETRY	*geometry;
		BOX3D		*the_bbox;
		int32 		nobjs;
		char		**objs;
		int32		*obj_types;
		int32		*obj_size;
		int		entire_size;
		bool		is3d = FALSE;
		bool		okay;
		int		next_loc;
		int32		*offsets;
		int		t;
		int		obj_parse_offset;
		int		nitems;
		int32		SRID;
		double	scale,offx,offy;
		BOX3D	*mybox;

//printf("str=%s\n",str);



		//trim white
	while (isspace((unsigned char) *str))
		str++;

	//test to see if it starts with a SRID=

	SRID = -1;
	scale = offx = offy = 0;

	if (strstr(str,"SRID="))
	{
		//its spatially located
		nitems = sscanf(str,"SRID=%i;",&SRID);
		if (nitems !=1 )
		{
			//error
			elog(ERROR,"couldnt parse objects in GEOMETRY (SRID related)");
			PG_RETURN_NULL() ;
		}
		//delete first part of string
		str = strchr(str,';');
		if (str != NULL)
			str++;
		else
		{
			elog(ERROR,"couldnt parse objects in GEOMETRY (SRID related - no ';')");
			PG_RETURN_NULL() ;
		}
	}


	if (strstr(str,"EMPTY"))
	{
		GEOMETRY *result=makeNullGeometry( SRID);
		if (strstr(str,"MULTIPOLYGON"))
			result->type = MULTIPOLYGONTYPE;
		if (strstr(str,"MULTILINESTRING"))
			result->type = MULTILINETYPE;
		if (strstr(str,"MULTIPOINT"))
			result->type = MULTIPOINTTYPE;
		PG_RETURN_POINTER( result );
	}

	if (strstr(str,"BOX3D") != NULL ) // bbox only
	{


		mybox = parse_box3d(str);

		if (mybox == NULL)
			PG_RETURN_NULL() ;

		geometry = (GEOMETRY *) palloc(sizeof(GEOMETRY));
		geometry->size = sizeof(GEOMETRY);
		geometry->type = BBOXONLYTYPE;
		geometry->nobjs = -1;
		geometry->SRID = SRID;
		geometry->scale = 1.0;
		geometry->offsetX = 0.0;
		geometry->offsetY = 0.0;
		memcpy(&(geometry->bvol),mybox, sizeof(BOX3D) );

		pfree(mybox);
		PG_RETURN_POINTER(geometry);
	}


	if ((str==NULL) || (strlen(str) == 0) )
	{
		elog(ERROR,"couldnt parse objects in GEOMETRY (null string)\n");
		PG_RETURN_NULL() ;
	}

	// handle the 2 variants of MULTIPOINT - 'MULTIPOINT(0 0, 1 1)'::geometry and 'MULTIPOINT( (0 0), (1 1))'::geometry;

		// we cheat - if its the 2nd variant, we replace the internal parethesis with spaces!
			if (strstr(str,"MULTIPOINT") != NULL )
			{
				//its a multipoint - replace any internal parenthesis with spaces
				char *first_paren;
				char *last_paren;
				char *current_paren;

				first_paren= index (str,'(');
				last_paren = rindex(str,')');

				if  ( (first_paren == NULL) || (last_paren == NULL) || (first_paren >last_paren) )
				{
						elog(ERROR,"couldnt parse objects in GEOMETRY (parenthesis related)\n");
						PG_RETURN_NULL() ;
				}
				//now we can just got through the string
				for (current_paren = (first_paren+1); current_paren <(last_paren);current_paren++)
				{
					if ( (current_paren[0] ==')') || (current_paren[0]=='(') )
					{
						current_paren[0] = ' ';
					}
				}

			}


//elog (NOTICE,"in:%s\n", str);

//printf("geometry_in got string ''\n");

	nobjs=	objects_inside(str);  //how many sub-objects
	if (nobjs <=0)	//dont allow zero-object items
	{
		elog(ERROR,"couldnt parse objects in GEOMETRY\n");
		PG_RETURN_NULL() ;
	}

//printf("geometry_in determins that there are %i sub-objects\n",nobjs);

	//allocate enough space for info structures


	objs = palloc(sizeof( char *) * nobjs);
	memset(objs, 0, sizeof( char *) * nobjs);

	obj_types = palloc(sizeof(int32) * nobjs);
	memset(obj_types, 0, sizeof(int32) * nobjs);

	obj_size  = palloc(sizeof(int32) * nobjs);
	memset(obj_size, 0, sizeof(int32) * nobjs);

	obj_parse_offset = 0; //start populating obj list at beginning


//printf("	+about to parse the objects\n");


	okay = parse_objects(obj_size,objs,obj_types,nobjs,str,&obj_parse_offset , &is3d);

//printf("	+finished parsing object\n");

	if (!(okay) || (obj_parse_offset != nobjs ) )
	{
		//free
		for (t =0; t<nobjs; t++)
		{
			if (objs[t] != NULL)
				pfree (objs[t]);
		}
		pfree(objs); pfree(obj_types); pfree(obj_size);
		elog(ERROR,"couldnt parse object in GEOMETRY\n");
		PG_RETURN_NULL();
	}

	entire_size = sizeof(GEOMETRY);

 	for (t =0; t<nobjs; t++)
	{
		//do they all have an obj_type?
		if ( (obj_types[t] == 0) || (objs[t] == NULL) )
		{
			okay = FALSE;
		}
		entire_size += obj_size[t];  //total space of the objects

		//size seem reasonable?
		if (obj_size[t] <1)
		{
			okay = FALSE;
		}

	}
	if (!(okay) )
	{
		for (t =0; t<nobjs; t++)
		{
			if (objs[t] != NULL)
				pfree (objs[t]);
		}
		pfree(objs); pfree(obj_types); pfree(obj_size);
		elog(ERROR,"couldnt parse object in GEOMETRY\n");
		PG_RETURN_NULL();
	}

		//size = object size + space for object type & offset array and possible double-word boundary stuff
	entire_size = entire_size + sizeof(int32) *nobjs*2 + nobjs*4;
	geometry = (GEOMETRY *) palloc (entire_size);

	geometry->size = entire_size;

	//set collection type.  Need to differentiate between
 	//  geometrycollection and (say) multipoint

	if (   strstr(str, "GEOMETRYCOLLECTION") != NULL  )
		geometry->type = COLLECTIONTYPE;
	else
	{
		if (   strstr(str, "MULTI") != NULL  )
		{
			if (obj_types[0] == POINTTYPE)
				geometry->type = MULTIPOINTTYPE;
			if (obj_types[0] == LINETYPE)
				geometry->type = MULTILINETYPE;
			if (obj_types[0] == POLYGONTYPE)
				geometry->type = MULTIPOLYGONTYPE;
		}
		else
		{
			geometry->type = obj_types[0]; //single object
		}
	}

	geometry->is3d = is3d;    //any 3d points found anywhere?
	geometry->nobjs = nobjs; // sub-objects


	//copy in type and length info

			//where to put objects.  next_loc will point to (bytes forward) &objData[0]
		next_loc = ( (char *) &(geometry->objType[0] ) - (char *) geometry);
		next_loc += sizeof(int32) * 2* nobjs;

		next_loc = MAXALIGN(next_loc);



		//where is the offsets array
		offsets = (int32 *) ( ((char *) &(geometry->objType[0] ))+ sizeof(int32) * nobjs ) ;


//printf("structure is at %p, offsets is at %p, next_loc = %i\n",geometry, offsets, next_loc);

	for (t=0; t<nobjs; t++)	//for each object
	{
		geometry->objType[t] = obj_types[t];  //set its type

		offsets[t] = next_loc;	//where is it
//printf("copy %i bytes from %p to %p\n",	obj_size[t] ,  objs[t],(char *) geometry + next_loc);
		memcpy( (char *) geometry + next_loc, objs[t], obj_size[t] );	//put sub-object into object
		pfree(objs[t]); // free the original object (its redundant)

		next_loc += obj_size[t];	//where does the next object go?

		next_loc =  MAXALIGN(next_loc);


	}

	//free temporary structures
	pfree(objs); pfree(obj_types); pfree(obj_size);


	//calculate its bounding volume
	the_bbox = bbox_of_geometry(geometry);

	if (the_bbox != NULL)
	{
		memcpy( &geometry->bvol, the_bbox, sizeof(BOX3D) );
		pfree(the_bbox);
	}


//printf("returning from geometry_in, nobjs = %i\n", nobjs);


		geometry->SRID = SRID;
		geometry->scale = scale;
		geometry->offsetX = offx;
		geometry->offsetY = offy;
	PG_RETURN_POINTER(geometry);
}

PG_FUNCTION_INFO_V1(srid_geom);
Datum srid_geom(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(geom1->SRID);
}


//Take internal rep of geometry, output string in the form of
//  'SRID=%i;<wkt>'  ie.  'SRID=5;POINT(1 1)'
PG_FUNCTION_INFO_V1(geometry_out);
Datum geometry_out(PG_FUNCTION_ARGS)
{
		GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		char				*wkt;
		char				*result;
		int				len;

		wkt = geometry_to_text(geom1);

		len = strlen(wkt) + 6+ 25 + 1;
		result = palloc(len);//extra space for SRID
		memset(result, 0, len);	//zero everything out

		sprintf(result,"SRID=%i;%s",geom1->SRID,wkt);

		pfree(wkt);

		PG_RETURN_CSTRING(result);

}

//Take internal rep of geometry, output string
PG_FUNCTION_INFO_V1(astext_geometry);
Datum astext_geometry(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char			   *wkt;
	char			   *result;
	int			len;

	wkt = geometry_to_text(geom1);

			//changed so it isnt null terminated (not needed with text)
	len = strlen(wkt) + 4;

	result= palloc(len);
	*((int *) result) = len;

	memcpy(result +4, wkt, len-4);

	pfree(wkt);

	PG_RETURN_CSTRING(result);
}


//Take internal rep of geometry, output string


//takes a GEOMETRY and returns a WKT representation
char *geometry_to_text(GEOMETRY  *geometry)
{
	char		*result;
	int		t,u;
	int32		*offsets;
	char		*obj;
	POINT3D	*pts;
	POLYGON3D	*polygon;
	LINE3D	*line;
	POINT3D	*point;

	int		pt_off,size;
	bool		briefmode= TRUE; // ie. dont print out "POINT3D(..)", just print out the ".." part
	bool 		is3d;
	bool		first_sub_obj = TRUE;
	bool		multi_obj = FALSE;
	int		mem_size,npts;


	if (geometry->nobjs == 0)
	{
			//empty geometry
		result = (char*) palloc(30);

		sprintf(result,"GEOMETRYCOLLECTION(EMPTY)");

		if (geometry->type == MULTILINETYPE)
			sprintf(result,"MULTILINESTRING(EMPTY)");
		if (geometry->type == MULTIPOINTTYPE)
			sprintf(result,"MULTIPOINT(EMPTY)");
		if (geometry->type == MULTIPOLYGONTYPE)
			sprintf(result,"MULTIPOLYGON(EMPTY)");
		return result;
	}


//printf("in geom_out(%p)\n",geometry);

	size = 30;	//just enough to put in object type
	result = (char *) palloc(30); mem_size= 30;       //try to limit number of repalloc()s

	if (geometry->type == BBOXONLYTYPE)
	{
			mem_size = MAX_DIGS_DOUBLE*6+5+2+4+5+1;
			pfree(result);
			result = (char *) palloc(mem_size); //double digits+ "BOX3D"+ "()" + commas +null
			sprintf(result, "BOX3D(%.15g %.15g %.15g,%.15g %.15g %.15g)",
					geometry->bvol.LLB.x,geometry->bvol.LLB.y,geometry->bvol.LLB.z,
					geometry->bvol.URT.x,geometry->bvol.URT.y,geometry->bvol.URT.z);
		return result;
	}


	if (geometry->type == POINTTYPE)
	{
		multi_obj = FALSE;
		sprintf(result, "POINT(" );
	}
	else if (geometry->type == LINETYPE)
	{
		multi_obj = FALSE;
		sprintf(result, "LINESTRING" );
	}
	else if (geometry->type == POLYGONTYPE)
	{
		multi_obj = FALSE;
		sprintf(result, "POLYGON" );
	}
	else if (geometry->type == MULTIPOINTTYPE)
	{
		//multiple sub-object need to be put into one object
		multi_obj = TRUE;
		sprintf(result, "MULTIPOINT(" );
	}
	else if (geometry->type == MULTILINETYPE)
	{
		//multiple sub-object need to be put into one object
		multi_obj = TRUE;
		sprintf(result, "MULTILINESTRING(" );
	}
	else if (geometry->type == MULTIPOLYGONTYPE)
	{
		//multiple sub-object need to be put into one object
		multi_obj = TRUE;
		sprintf(result, "MULTIPOLYGON(" );
	}
	else if (geometry->type == COLLECTIONTYPE)
	{
		sprintf(result, "GEOMETRYCOLLECTION(" );
		briefmode = FALSE;
		multi_obj = FALSE;
	}

		//where are the objects?
	offsets = (int32 *) ( ((char *) &(geometry->objType[0] ))+ sizeof(int32) * geometry->nobjs ) ;

	is3d = geometry->is3d;


	for(t=0;t<geometry->nobjs; t++)  //for each object
	{
		obj = (char *) geometry +offsets[t] ;

		if (geometry->objType[t] == 1)   //POINT
		{
			point = (POINT3D *) obj;
			if (briefmode)	//dont need to put in "POINT("
			{
				size +=MAX_DIGS_DOUBLE*3 + 2 +3  ;
				result = repalloc(result, size );  //make memory bigger
				if (!(first_sub_obj))
				{
					strcat(result,",");
				}
				else
				{
					first_sub_obj = FALSE;
				}
				print_point(result, point,is3d);  //render point
				if (t == (geometry->nobjs-1))
					strcat(result,")");	// need to close object?
			}
			else
			{
				size +=MAX_DIGS_DOUBLE*3 + 2 +3 +7  ;
				result = repalloc(result, size  );
				strcat(result, "POINT(");
				print_point(result, point,is3d);
				strcat(result, ")");
				if (t != (geometry->nobjs -1) )
					strcat(result,",");
			}

		}
		if (geometry->objType[t] == 2)	//LINESTRING
		{
			line = (LINE3D *) obj;
			if (briefmode)
			{
				size +=(MAX_DIGS_DOUBLE*3+5)*line->npoints +3;
				result = repalloc(result, size );
#ifdef DEBUG_TOTEXT
elog(NOTICE, "Added space for %d points, result size: %d", line->npoints, size);
elog(NOTICE, "Strlen result(%x): %d", result, strlen(result));
#endif

				if (!(first_sub_obj))
				{
					strcat(result,",");
				}
				else
				{
					first_sub_obj = FALSE;
				}

				strcat(result,"(");
				print_many_points(result, &line->points[0],line->npoints , is3d);
				strcat(result,")");
				if ((t == (geometry->nobjs-1)) && multi_obj )  //close object?
					strcat(result,")");
#ifdef DEBUG_TOTEXT
elog(NOTICE, "RESULT: %s", result);
#endif
			}
			else
			{
				size +=(MAX_DIGS_DOUBLE*3+5)*line->npoints +12+3;
				result = repalloc(result, size );
				strcat(result, "LINESTRING(");

				//optimized for speed

				print_many_points(result, &line->points[0],line->npoints , is3d);
				strcat(result,")");
				if (t != (geometry->nobjs -1) )
					strcat(result,",");

			}
		}
		if (geometry->objType[t] == 3)		//POLYGON
		{
				polygon = (POLYGON3D *) obj;
				if (!(briefmode))
				{
					size += 7 + polygon->nrings *3+9;
					result = repalloc(result, size );
					strcat(result,"POLYGON");
				}
				else
				{
					size += 7 + polygon->nrings *3;
					result = repalloc(result, size );
				}

				//need to separate objects?
				if (!(first_sub_obj))
				{
					strcat(result,",");
				}
				else
				{
					first_sub_obj = FALSE;
				}


				strcat(result,"(");	//begin poly

				pt_off = 0;	//where is the first point in this ring?

					//where are the points
				pts = (POINT3D *) ( (char *)&(polygon->npoints[polygon->nrings] )  );
				pts = (POINT3D *) MAXALIGN(pts);


				npts = 0;
				for (u=0; u< polygon->nrings ; u++)
					npts += polygon->npoints[u];

				size += (MAX_DIGS_DOUBLE*3+3) * npts + 5* polygon->nrings;
				result = repalloc(result, size );

				for (u=0; u< polygon->nrings ; u++)  //for each ring
				{
//printf("making ring #%i in obj, %i\n",u,t);
					if (u!=0)
						strcat(result,","); //separate rings

					strcat(result,"(");	//begin ring
					print_many_points(result, &pts[pt_off] ,polygon->npoints[u], is3d);

					pt_off = pt_off + polygon->npoints[u]; //where is first point of next ring?
					strcat(result,")");	//end ring
				}
				strcat(result,")"); //end poly
				if ((t == (geometry->nobjs-1)) && multi_obj )
					strcat(result,")");
				if ((!(briefmode)) && (t != (geometry->nobjs -1) ))
					strcat(result,",");
		}
		if (!(briefmode))
		{
			first_sub_obj = TRUE;
		}

	}

	if (!(briefmode))
		strcat(result,")");
	return(result);
}


/*****************************************************
 * conversion routines
 *****************************************************/





//get bvol inside a geometry
PG_FUNCTION_INFO_V1(get_bbox_of_geometry);
Datum get_bbox_of_geometry(PG_FUNCTION_ARGS)
{
	GEOMETRY  *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	BOX3D	 *result;

	if (geom->nobjs == 0)
		PG_RETURN_NULL();

	//make a copy of it

	result = palloc ( sizeof(BOX3D) );
	memcpy(result,&geom->bvol, sizeof(BOX3D) );


	PG_RETURN_POINTER(result);
}


//make a fake geomery with just it a bvol
PG_FUNCTION_INFO_V1(get_geometry_of_bbox);
Datum get_geometry_of_bbox(PG_FUNCTION_ARGS)
{
	BOX3D  *bbox = (BOX3D *) PG_GETARG_POINTER(0);
	GEOMETRY  *geom   ;

	//make a copy of it

	geom = palloc ( sizeof(GEOMETRY) );
	geom->size = sizeof(GEOMETRY);
	geom->type = BBOXONLYTYPE;
	geom->nobjs = -1;
	geom->SRID = -1;
	geom->scale = 1.0;
	geom->offsetX = 0.0;
	geom->offsetY = 0.0;
	memcpy(&(geom->bvol),bbox, sizeof(BOX3D) );


	PG_RETURN_POINTER(geom);
}




//*****************************************************************
//WKB does not do any padding or alignment!

// in general these function take
//	1. geometry item or list
//	2. if the endian-ness should be changed (flipbytes)
//	3. byte_order - 0=big endian (XDR) 1= little endian (NDR)
//    4. use the 3d extention wkb (use3d)
//
// NOTE:
//		A. mem - if null, function pallocs memory for result
//			 - otherwise, use mem to dump the wkb into
//		b. for lists, the # of sub-objects is also specified
//
//  and return
//	1. size of the wkb structure ("int *size")
//	2. the actual wkb structure
//*****************************************************************

//convert a POINT3D into wkb
char	*wkb_point(POINT3D *pt,int32 *size, bool flipbytes, char byte_order, bool use3d)
{
	char		*result ;
	uint32	type ;
	char		*c_type = (char *) &type;

	if (use3d)
	{
		*size = 29;
		type = ((unsigned int) WKB3DOFFSET + ((unsigned int)1));
	}
	else
	{
		*size = 21;
		type =  1;
	}

	result = palloc (*size);

	if (flipbytes)
		flip_endian_int32( (char *) &type);

	result[0] = byte_order;
	result[1] = c_type[0];
	result[2] = c_type[1];
	result[3] = c_type[2];
	result[4] = c_type[3];

	if (use3d)
		memcpy(&result[5], pt, 24 ); //copy in point data
	else
		memcpy(&result[5], pt, 16 ); //copy in point data

	if (flipbytes)
	{
		flip_endian_double((char *) &result[5]);		// x
		flip_endian_double((char *) &result[5+8]);	// y
		if (use3d)
			flip_endian_double((char *) &result[5+16]);	// z
	}

	return result;
}

//pt is a pointer to list of POINT3Ds
char	*wkb_multipoint(POINT3D *pt,int32 numb_points,int32 *size, bool flipbytes, char byte_order,bool use3d)
{
	char *result;
	uint32	type ; 		// 3D/2D multipoint, following frank's spec
	uint32	npoints = numb_points;
	char		*c_type = (char *) &type;
	char		*c_npoints = (char *) &npoints;
	int		t;
	int32		junk;
	const int		sub_size_3d = 29;
	const int		sub_size_2d = 21;




	if (use3d)
	{
		*size = 9 + sub_size_3d*numb_points;
		type = WKB3DOFFSET + 4;
	}
	else
	{
		*size = 9 + sub_size_2d*numb_points;
		type =  4;
	}


	result = palloc (*size);
	npoints = numb_points;

	if (flipbytes)
	{
		flip_endian_int32((char *) &type);
		flip_endian_int32((char *) &npoints);
	}


	result[0] = byte_order;
	result[1] = c_type[0];
	result[2] = c_type[1];
	result[3] = c_type[2];
	result[4] = c_type[3];

	result[5] = c_npoints[0];
	result[6] = c_npoints[1];
	result[7] = c_npoints[2];
	result[8] = c_npoints[3];

	for (t=0; t<numb_points;t++)  //for each point
	{
		if (use3d)
			memcpy(&result[9+ t*sub_size_3d],
					wkb_point(&pt[t],&junk,  flipbytes,  byte_order,use3d)
						, sub_size_3d);
		else
			memcpy(&result[9+ t*sub_size_2d],
					wkb_point(&pt[t],&junk,  flipbytes,  byte_order,use3d)
							, sub_size_2d);
	}

	return result;

}


//if mem != null, it must have enough space for the wkb
char	*wkb_line(LINE3D *line,int32 *size, bool flipbytes, char byte_order,bool use3d, char *mem)
{
	char *result;
	uint32	type ;

	char		*c_type = (char *) &type;
	int		t;

	uint32	npoints = line->npoints;
	int		numb_points = line->npoints;
	char		*c_npoints = (char *) &npoints;


	if (use3d)
	{
		*size = 9 + 24*numb_points;
		type = WKB3DOFFSET + 2;
	}
	else
	{
		*size = 9 + 16*numb_points;
		type =  2;
	}



	if (flipbytes)
	{
		flip_endian_int32((char *) &type);
		flip_endian_int32((char *) &npoints);
	}

	if (mem == NULL)
		result = palloc (*size);
	else
		result = mem;

	result[0] = byte_order;
	result[1] = c_type[0];		//type
	result[2] = c_type[1];
	result[3] = c_type[2];
	result[4] = c_type[3];

	result[5] = c_npoints[0];	//npoints
	result[6] = c_npoints[1];
	result[7] = c_npoints[2];
	result[8] = c_npoints[3];

	if (use3d)
	{
		memcpy(&result[9],&line->points, line->npoints*24); // copy all the pt data in one chuck
		if (flipbytes)
		{
			for (t=0;t<line->npoints; t++)
			{
				flip_endian_double((char *) &result[9   + t*24]);	// x
				flip_endian_double((char *) &result[9+8 + t*24]);	// y
				flip_endian_double((char *) &result[9+16+ t*24]);	// z
			}
		}
	}
	else
	{
		for (t=0; t<numb_points;t++)
		{
			memcpy(&result[9+16*t],&((&line->points)[t]), 16);  //need to copy each pt individually
			if (flipbytes)
			{
				flip_endian_double((char *) &result[9   + t*16]);	// x
				flip_endian_double((char *) &result[9+8 + t*16]);	// y
			}
		}
	}
	return result;
}

// expects an array of pointers to lines
// calls wkb_line() to do most of the work
char	*wkb_multiline(LINE3D **lines,int32 *size, int numb_lines, bool flipbytes, char byte_order,bool use3d)
{
	int	total_points = 0;
	int	total_size=0;
	int	size_part = 0;

	int 	t;
	char	*result,*new_spot;


	uint32	type ;
	char		*c_type = (char *) &type;

	uint32	nlines = numb_lines;
	char		*c_nlines = (char *) &nlines;

	if (use3d)
	{
		type = WKB3DOFFSET + 5;
	}
	else
	{
		type =  5;
	}

	if (flipbytes)
	{
		flip_endian_int32((char *) &type);
		flip_endian_int32((char *) &nlines);
	}

	//how many points in the entire multiline()
	for (t=0;t<numb_lines; t++)
	{
		total_points += lines[t]->npoints;
	}


	if (use3d)
	{
		total_size = 9+ 9*numb_lines + 24*total_points;
	}
	else
	{
		total_size = 9 + 9*numb_lines + 16*total_points;
	}

	*size = total_size;

	result = palloc (total_size);



	result[0] = byte_order;
	result[1] = c_type[0];		//type
	result[2] = c_type[1];
	result[3] = c_type[2];
	result[4] = c_type[3];

	result[5] = c_nlines[0];	//num linestrings
	result[6] = c_nlines[1];
	result[7] = c_nlines[2];
	result[8] = c_nlines[3];



	new_spot = result + 9;//where to put the next linestring

		for (t=0;t<numb_lines;t++)  //for each linestring
		{
			wkb_line( lines[t], &size_part, flipbytes, byte_order, use3d, new_spot);
			new_spot += size_part; //move to next slot
		}
	return result;
}


//mem should be zero or hold enough space
char	*wkb_polygon(POLYGON3D	*poly,int32 *size, bool flipbytes, char byte_order,bool use3d, char *mem)
{
	int	t, u,total_points =0;
	uint32	type ;
	char		*c_type = (char *) &type;
	uint32	numRings,npoints;
	char		*c_numRings = (char *) &numRings;
	char		*c_npoints = (char *) &npoints;
	int		offset;
	POINT3D	*pts;
	int		point_offset;
	char 	*result;


	//where the polygon's points are (double aligned)

	pts = (POINT3D *) ( (char *)&(poly->npoints[poly->nrings] )  );
	pts = (POINT3D *) MAXALIGN(pts);



	numRings = poly->nrings;

	//total points in all the rings
	for (t=0; t< poly->nrings; t++)
	{
		total_points += poly->npoints[t];
	}

	if (use3d)
	{
		*size = 9 + 4*poly->nrings + 24*total_points;
		type = WKB3DOFFSET + 3;
	}
	else
	{
		*size = 9 + 4*poly->nrings + 16*total_points;
		type = 3;
	}

	if (mem == NULL)
		result = palloc (*size);
	else
		result = mem;


	if (flipbytes)
	{
		flip_endian_int32((char *) &type);
		flip_endian_int32((char *) &numRings);
	}



	result[0] = byte_order;
	result[1] = c_type[0];		//type
	result[2] = c_type[1];
	result[3] = c_type[2];
	result[4] = c_type[3];

	result[5] = c_numRings[0];	//npoints
	result[6] = c_numRings[1];
	result[7] = c_numRings[2];
	result[8] = c_numRings[3];


	if (use3d)
	{
		offset = 9; //where the actual linearring structure is going
		point_offset = 0;
		for (t=0;t<poly->nrings; t++) //for each ring
		{
			npoints = poly->npoints[t];
			if (flipbytes)
				flip_endian_int32((char *) &npoints);
			result[offset]   = c_npoints[0];  //num points in this ring
			result[offset+1] = c_npoints[1];
			result[offset+2] = c_npoints[2];
			result[offset+3] = c_npoints[3];

			memcpy(&result[offset+4], &pts[point_offset], 24*poly->npoints[t]);
			if (flipbytes)
			{
				for (u=0;u<poly->npoints[t];u++)  //for each point
				{
					flip_endian_double((char *) &result[offset+4+u*24]   );
					flip_endian_double((char *) &result[offset+4+u*24+8] );
					flip_endian_double((char *) &result[offset+4+u*24]+16);
				}
			}
			point_offset += poly->npoints[t];     // reset offsets
			offset +=  4 +  poly->npoints[t]* 24;
		}
	}
	else
	{
		offset = 9; // where the 1st linearring goes
		point_offset = 0;
		for (t=0;t<poly->nrings; t++)
		{
			npoints = poly->npoints[t];
			if (flipbytes)
				flip_endian_int32((char *) &npoints);
			result[offset]   = c_npoints[0];
			result[offset+1] = c_npoints[1];
			result[offset+2] = c_npoints[2];
			result[offset+3] = c_npoints[3];

				for (u=0;u<poly->npoints[t];u++)
				{
					memcpy(&result[offset+4 + u*16], &pts[point_offset+u], 16);
					if (flipbytes)
					{
						flip_endian_double((char *) &result[offset+4+u*16]   );
						flip_endian_double((char *) &result[offset+4+u*16+8] );
					}
				}

			point_offset += poly->npoints[t];
			offset +=  4 +  poly->npoints[t]* 16;
		}

	}
	return result;
}


//expects a list of pointer to polygon3d
char	*wkb_multipolygon(POLYGON3D	**polys,int numb_polys,int32 *size, bool flipbytes, char byte_order,bool use3d)
{

	int	t,u;
	int	total_points = 0;
	int	total_size=0;
	int	size_part = 0;
	int	total_rings = 0;
	char	*result,*new_spot;
	uint32	type;

	char		*c_type = (char *) &type;

	uint32	npolys = numb_polys;
	char		*c_npolys = (char *) &npolys;

	if (use3d)
	{
		type = WKB3DOFFSET + 6;
	}
	else
	{
		type =  6;
	}


	if (flipbytes)
	{
		flip_endian_int32( (char *) &type);
		flip_endian_int32((char *) &npolys);
	}




	//find total # rings and total points
	for (t=0;t<numb_polys; t++)
	{
		total_rings += polys[t]->nrings;
		for (u=0;u<polys[t]->nrings; u++)
		{
			total_points += polys[t]->npoints[u];
		}
	}


	if (use3d)
	{
		total_size =9 +  9*numb_polys + 24*total_points + 4*total_rings;
	}
	else
	{
		total_size = 9 + 9*numb_polys + 16*total_points + 4*total_rings;
	}

	*size = total_size;

	result = palloc (total_size);

	result[0] = byte_order;
	result[1] = c_type[0];		//type
	result[2] = c_type[1];
	result[3] = c_type[2];
	result[4] = c_type[3];

	result[5] = c_npolys[0];	//number of polygons
	result[6] = c_npolys[1];
	result[7] = c_npolys[2];
	result[8] = c_npolys[3];



	new_spot = result+9;  //where 1st polygon goes

		for (t=0;t<numb_polys;t++)  //for each polygon
		{
			wkb_polygon( polys[t], &size_part, flipbytes, byte_order, use3d, new_spot);
			new_spot +=  size_part; //more to next slot
		}
	return result;

}


//passes work off to the correct function
// then makes a simple postgres type - the first 4 bytes is an int32 with the size of the structure
// and the rest is the wkb info.
//
// 3d-ness is determined by looking at the is3d flag in the GEOMETRY structure

char	*to_wkb(GEOMETRY *geom, bool flip_endian)
{
	char	*result, *sub_result;
	int 	size;

	if (geom->type == COLLECTIONTYPE)
	{
		sub_result=	to_wkb_collection(geom,  flip_endian, &size);
		size += 4; //size info
		result = palloc (size );
		memcpy(result+4, sub_result, size-4);
		memcpy(result, &size, 4);
		pfree(sub_result);
		return result;
	}
	else
	{
		sub_result=	to_wkb_sub(geom,  flip_endian, &size);
		size += 4; //size info
		result = palloc (size );
		memcpy(result+4, sub_result, size-4);
		memcpy(result, &size, 4);
		pfree(sub_result);
		return result;
	}

}

//make a wkb chunk out of a geometrycollection
char *to_wkb_collection(GEOMETRY *geom, bool flip_endian, int32 *end_size)
{
	char	*result, **sub_result;
	int	sub_size;
	int	*sizes;
	int	t;
	int32		*offsets1;

	int32		type;
	int		total_size =0;
	int		offset;
	char		byte_order;
	int		coll_type=7;
	char		*c_type = (char *)&coll_type;
	int		nobj = geom->nobjs;
	char		*c_nobj = (char *) &nobj;

//printf("making a wkb of a collections\n");


	//determing byte order and if numbers need endian change
	if (BYTE_ORDER == BIG_ENDIAN)
	{
		if (flip_endian)
		{
			byte_order = 1;
		}
		else
		{
			byte_order = 0;
		}
	}
	else
	{
		if (flip_endian)
		{
			byte_order = 0;
		}
		else
		{
			byte_order = 1;
		}
	}

	//for sub part of the geometry structure
	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;


	// we make a list of smaller wkb chunks in sub_result[]
	// and that wkb chunk's size in sizes[]

	sub_result = NULL;
	if (geom->nobjs >0)
		sub_result = palloc( sizeof(char *) * geom->nobjs);
	sizes = NULL;
	if (geom->nobjs >0)
		sizes = palloc( sizeof(int) * geom->nobjs);


	for (t=0; t<geom->nobjs; t++)	//for each part of the collections, do the work in another function
	{
		type = geom->objType[t];

		if (type == POINTTYPE)
		{
			sub_result[t] = (  wkb_point((POINT3D *) ((char *) geom +offsets1[t] ) ,
					&sub_size, flip_endian, byte_order, geom->is3d));
			sizes[t] = sub_size;
			total_size += sub_size;
		}

		if (type == LINETYPE)
		{

			sub_result[t] = (  wkb_line((LINE3D *) ((char *) geom +offsets1[t] ) ,
				 &sub_size, flip_endian, byte_order, geom->is3d, NULL));
			sizes[t] = sub_size;
			total_size += sub_size;
		}
		if (type == POLYGONTYPE)
		{

			sub_result[t] = (  wkb_polygon((POLYGON3D *) ((char *) geom +offsets1[t] ) ,
				 &sub_size, flip_endian, byte_order, geom->is3d, NULL));
			sizes[t] = sub_size;
			total_size += sub_size;
		}
	}

	result = palloc( total_size +9); // need geometrycollection header

	if (flip_endian)
	{
		flip_endian_int32((char *) &coll_type);
		flip_endian_int32((char *) &nobj);
	}

	//could use a memcpy, but...

	result[0] = byte_order;
	result[1] = c_type[0];		//type
	result[2] = c_type[1];
	result[3] = c_type[2];
	result[4] = c_type[3];

	result[5] = c_nobj[0];	//number of part to collection
	result[6] = c_nobj[1];
	result[7] = c_nobj[2];
	result[8] = c_nobj[3];


	offset =9;  //start after the geometrycollection header


	for (t=0;t<geom->nobjs; t++)
	{
		memcpy(result+ offset, sub_result[t], sizes[t]);
		pfree(sub_result[t]);
		offset += sizes[t];
	}

		//free temp structures
	if (sub_result != NULL)
		pfree( sub_result);
	if (sizes != NULL)
		pfree( sizes);

		//total size of the wkb
	*end_size = total_size+9;



	//decode_wkb(result, &junk);
	return result;
}


//convert geometry to WKB, flipping endian if appropriate
//
//  This handles POINT, LINESTRING, POLYGON, MULTIPOINT,MULTILINESTRING, MULTIPOLYGON
//
// GEOMETRYCOLLECTION is not handled by this function see to_wkb_collection()
//
// flip_endian - request an endian swap (relative to current machine)
// wkb_size    - return size of wkb

char	*to_wkb_sub(GEOMETRY *geom, bool flip_endian, int32 *wkb_size)
{

	char	byte_order;
	char	*result = NULL;
	int		t;


	int32		*offsets1;
	LINE3D	**linelist;
	POLYGON3D	**polylist;


	//determine WKB byte order flag
	if (BYTE_ORDER == BIG_ENDIAN)	//server is running on a big endian machine
	{
		if (flip_endian)
		{
			byte_order = 1;  //as per WKB specification
		}
		else
		{
			byte_order = 0;
		}
	}
	else
	{
		if (flip_endian)
		{
			byte_order = 0;
		}
		else
		{
			byte_order = 1;
		}
	}

	// for searching in the geom struct
	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;


		// for each of the possible geometry types, we pass it off to a worker function

	if (geom->type == POINTTYPE)
	{

		result = (  wkb_point((POINT3D *) ((char *) geom +offsets1[0] ) ,
					wkb_size, flip_endian, byte_order, geom->is3d));
	}

	if (geom->type == MULTIPOINTTYPE)
	{

		result = (  wkb_multipoint((POINT3D *) ((char *) geom +offsets1[0] ) ,
				geom->nobjs,	wkb_size, flip_endian, byte_order, geom->is3d));
	}

	if (geom->type == LINETYPE)
	{

		result = (  wkb_line((LINE3D *) ((char *) geom +offsets1[0] ) ,
				 wkb_size, flip_endian, byte_order, geom->is3d, NULL));
	}


	if (geom->type == MULTILINETYPE)
	{
		//make a list of lines
		linelist = NULL;
		if (geom->nobjs >0)
			linelist = palloc( sizeof(LINE3D *) * geom->nobjs);
		for (t=0;t<geom->nobjs; t++)
		{
			linelist[t] = (LINE3D *) ((char *) geom +offsets1[t] ) ;
		}
		result = (  wkb_multiline( linelist,
				 wkb_size, geom->nobjs, flip_endian, byte_order, geom->is3d));
	}

	if (geom->type == POLYGONTYPE)
	{

		result = (  wkb_polygon((POLYGON3D *) ((char *) geom +offsets1[0] ) ,
				 wkb_size, flip_endian, byte_order, geom->is3d, NULL));
	}


	if (geom->type == MULTIPOLYGONTYPE)
	{
		//make a list of polygons
		polylist = NULL;
		if (geom->nobjs >0)
			polylist = palloc( sizeof(POLYGON3D *) * geom->nobjs);
		for (t=0;t<geom->nobjs; t++)
		{
			polylist[t] = (POLYGON3D *) ((char *) geom +offsets1[t] ) ;
		}
		result = (  wkb_multipolygon( polylist,geom->nobjs,
				 wkb_size,  flip_endian, byte_order, geom->is3d));
	}

	//decode_wkb(result, &junk);

	return (result);
}


//convert binary geometry into OGIS well know binary format with NDR (little endian) formating
// see http://www.opengis.org/techno/specs/99-049.rtf page 3-24 for specification
//
// 3d geometries are encode as in OGR by adding WKB3DOFFSET to the type.  Points are then 24bytes (X,Y,Z)
// instead of 16 bytes (X,Y)
//
//dont do any flipping of endian   asbinary_simple(GEOMETRY)
PG_FUNCTION_INFO_V1(asbinary_simple);
Datum asbinary_simple(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_POINTER(to_wkb(geom, FALSE));
}


//convert binary geometry into OGIS well know binary format with NDR (little endian) formating
// see http://www.opengis.org/techno/specs/99-049.rtf page 3-24 for specification
//
// 3d geometries are encode as in OGR by adding WKB3DOFFSET to the type.  Points are then 24bytes (X,Y,Z)
// instead of 16 bytes (X,Y)
//
//flip if required    asbinary_specify(GEOMETRY,'xdr') or asbinary_specify(GEOMETRY,'ndr')
PG_FUNCTION_INFO_V1(asbinary_specify);
Datum asbinary_specify(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		 text      			*type = PG_GETARG_TEXT_P(1);


	if (VARSIZE(type) < 7)  // 4 (size header) + 3 (text length)
	{
		elog(ERROR,"asbinary(geometry, <type>) - type should be 'XDR' or 'NDR'.  type length is %i",VARSIZE(type) -4);
		PG_RETURN_NULL();
	}

	if  ( ( strncmp(VARDATA(type) ,"xdr",3) == 0 ) || (strncmp(VARDATA(type) ,"XDR",3) == 0) )
	{
//printf("requested XDR\n");
		if (BYTE_ORDER == BIG_ENDIAN)
			PG_RETURN_POINTER(to_wkb(geom, FALSE));
		else
			PG_RETURN_POINTER(to_wkb(geom, TRUE));
	}
	else
	{
//printf("requested NDR\n");
		if (BYTE_ORDER == LITTLE_ENDIAN)
			PG_RETURN_POINTER(to_wkb(geom, FALSE));
		else
			PG_RETURN_POINTER(to_wkb(geom, TRUE));
	}
}

//takes a text argument and parses it to make a geometry
PG_FUNCTION_INFO_V1(geometry_text);
Datum geometry_text(PG_FUNCTION_ARGS)
{
	char		*input = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char		*cstring;
	void		*result;

	if (*((int *) input) == 4)
	{
		//empty string
		PG_RETURN_NULL();
	}
	cstring = palloc(*((int *) input));  // length+4

	memcpy(cstring, &input[4], ( *((int *) input))-4);
	cstring[( *((int *) input))-4] = 0; // nullterminate

	result = DatumGetPointer(	DirectFunctionCall1(geometry_in,PointerGetDatum(cstring))) ;

	PG_RETURN_POINTER (result);
//NOTE: this could cause problem since input isnt null terminated - usually this will not
//      cause a problem because the parse will not go further than the end of the string.
//       FIXED.
}



//make a geometry with one obj in it (of size new_obj_size)
// type should be POINTTYPE, LINETYPE or POLYGONTYPE
// if you want to change the object's type to something else (ie GEOMETRYCOLLECTION), do
// that after with geom->type = GEOMETRYCOLLECTION
// this does  calculate the bvol
//
// sets the SRID and grid info to the given values
//
// do not call this with type = GEOMETRYCOLLECTION

GEOMETRY	*make_oneobj_geometry(int sub_obj_size, char *sub_obj, int type, bool is3d,int SRID, double scale, double offx, double offy)
{
	int size = sizeof(GEOMETRY) + 4 + sub_obj_size ;
	GEOMETRY	*result = palloc(size);
	char		*sub_obj_loc;
	BOX3D		*bbox;

	result->size = size;
	result->nobjs = 1;
	result->type = type;
	result->is3d = is3d;


	result->SRID = SRID;
	result->scale = scale;
	result->offsetX = offx;
	result->offsetY = offy;

	result->objType[0] = type;
	if (type == MULTIPOINTTYPE)
		result->objType[0] = POINTTYPE;
	if (type == MULTILINETYPE)
		result->objType[0] = LINETYPE;
	if (type == MULTIPOLYGONTYPE)
		result->objType[0] = POLYGONTYPE;

	if (type == COLLECTIONTYPE)
	{
		pfree(result);
		return(NULL); //error
	}

	sub_obj_loc = (char *)  &result->objType[2];
	sub_obj_loc  = (char *) MAXALIGN(sub_obj_loc);

	result->objType[1] = sub_obj_loc  - (char *) result; //result->objType[1] is where objOffset is

	//copy in the subobject
	memcpy(sub_obj_loc , sub_obj, sub_obj_size);

	bbox = bbox_of_geometry(result);
	memcpy(&result->bvol,bbox, sizeof(BOX3D) ); //make bounding box

	return result;
}


//find the size of the subobject and return it
int	size_subobject (char *sub_obj, int type)
{
	if (type == POINTTYPE)
	{
		return (sizeof(POINT3D));
	}
	if (type == LINETYPE)
	{
		return(sizeof(LINE3D) +  sizeof(POINT3D) * ( ((LINE3D *)sub_obj)->npoints ));
	}
	if (type==POLYGONTYPE)
	{
		POLYGON3D	*poly = (POLYGON3D *) sub_obj;
		int		t,points=0;

		for (t=0;t<poly->nrings;t++)
		{
			points += poly->npoints[t];
		}
		if  (   ( (long) ( &poly->npoints[poly->nrings] )) == (MAXALIGN(&poly->npoints[poly->nrings] ) ) )
			return (sizeof(POLYGON3D) + 4*(poly->nrings-1) +  sizeof(POINT3D)*(points-1) );  //no extra align
		else
			return (sizeof(POLYGON3D) + 4*(poly->nrings-1) +  sizeof(POINT3D)*(points-1) +4 );
	}

	return (-1);//unknown sub-object type
}


//produce a new geometry, which is the old geometry with another object stuck in it
// This will try to make the geometry's type is correct (move POINTTYPE to MULTIPOINTTYPE or
//   change to GEOMETRYCOLLECTION)
//
// this does NOT calculate the bvol - you should set it with "bbox_of_geometry"
//
// type is the type of the subobject
// do not call this as with type = GEOMETRYCOLLECTION
//
// doesnt change the is3d flag
// doesnt change the SRID or other attributes

GEOMETRY	*add_to_geometry(GEOMETRY *geom,int sub_obj_size, char *sub_obj, int type)
{
	int		size,t;
	int		size_obj,next_offset;
	GEOMETRY	*result;
	int32		*old_offsets, *new_offsets;

	//all the offsets could cause re-alignment problems, so need to deal with each one
	size = geom->size +4*(geom->nobjs +1) /*byte align*/
			+sub_obj_size + 4 /*ObjType[]*/ +4 /*new offset*/;

	result = (GEOMETRY *) palloc(size);
	result->size = size;
	result->is3d = geom->is3d;
	result->SRID = geom->SRID;
	result->offsetX = geom->offsetX;
	result->offsetY = geom->offsetY;
	result->scale = geom->scale;

	//accidently sent in a single-entity type but gave it a multi-entity type
	//  re-type it as single-entity
	if (type == MULTIPOINTTYPE)
		type = POINTTYPE;
	if (type == MULTILINETYPE)
		type = LINETYPE;
	if (type == MULTIPOLYGONTYPE)
		type = POLYGONTYPE;


	// change to the simplest possible type that supports the type being added
	if  (geom->type == POINTTYPE || geom->type == MULTIPOINTTYPE)
	{
	 	if (type == POINTTYPE)
			result->type  = MULTIPOINTTYPE;
		else
			result->type  = COLLECTIONTYPE;
	}
	if  (geom->type == LINETYPE || geom->type == MULTILINETYPE)
	{
	 	if (type == LINETYPE)
			result->type  = MULTILINETYPE;
		else
			result->type  = COLLECTIONTYPE;
	}
	if  (geom->type == POLYGONTYPE || geom->type == MULTIPOLYGONTYPE)
	{
	 	if (type == POLYGONTYPE)
			result->type  = MULTIPOLYGONTYPE;
		else
			result->type  = COLLECTIONTYPE;
	}
	if (geom->type == COLLECTIONTYPE)
		result->type = COLLECTIONTYPE;

	// now result geometry's type and sub-object's type is okay
	// we have to setup the geometry

	result->nobjs = geom->nobjs+1;

	for (t=0; t< geom->nobjs; t++)
	{
		result->objType[t] = geom->objType[t];
	}

//printf("about to copy geomes\n");
//printf("result is at %p and is %i bytes long\n",result,result->size);
//printf("geom is at %p and is %i bytes long\n",geom,geom->size);

	old_offsets =& 	geom->objType[geom->nobjs] ;
	new_offsets =& 	result->objType[result->nobjs] ;
	next_offset = ( (char *) &new_offsets[result->nobjs] ) - ( (char *) result) ;
	next_offset = MAXALIGN(next_offset);

	//have to re-set the offsets and copy in the sub-object data
	for (t=0; t< geom->nobjs; t++)
	{
		//where is this going to go?
		new_offsets[t] = next_offset;

		size_obj = 	size_subobject ((char *) (((char *) geom) + old_offsets[t] ), geom->objType[t]);

		next_offset += size_obj;
		next_offset = MAXALIGN(next_offset); // make sure its aligned properly


//printf("coping %i bytes from %p to %p\n", size_obj,( (char *) geom) + old_offsets[t],((char *) result)  + new_offsets[t]   );
		memcpy( ((char *) result)  + new_offsets[t] , ( (char *) geom) + old_offsets[t], size_obj);
//printf("copying done\n");

	}

//printf("copying in new object\n");

	//now, put in the new data
	result->objType[ result->nobjs -1 ] = type;
	new_offsets[ result->nobjs -1 ] = next_offset;
	memcpy(  ((char *) result)  + new_offsets[result->nobjs-1] ,sub_obj , sub_obj_size);

//printf("returning\n");

	return result;
}



//make a polygon obj
// size is return in arg "size"
POLYGON3D	*make_polygon(int nrings, int *pts_per_ring, POINT3D *pts, int npoints, int *size)
{
		POLYGON3D	*result;
		int		t;
		POINT3D	*inside_poly_pts;


	*size = sizeof(POLYGON3D) + 4 /*align*/
				+ 4*(nrings-1)/*npoints struct*/
				+ sizeof(POINT3D) *(npoints-1) /*points struct*/ ;

 	result= (POLYGON3D *) palloc (*size);
	result->nrings = nrings;


	for (t=0;t<nrings;t++)
	{
		result->npoints[t] = pts_per_ring[t];
	}

	inside_poly_pts = (POINT3D *) ( (char *)&(result->npoints[result->nrings] )  );
	inside_poly_pts = (POINT3D *) MAXALIGN(inside_poly_pts);

	memcpy(inside_poly_pts, pts, npoints *sizeof(POINT3D) );

	return result;
}

void set_point( POINT3D *pt,double x, double y, double z)
{
	pt->x = x;
	pt->y = y;
	pt->z = z;
}

//make a line3d object
// return size of structure in 'size'
LINE3D	*make_line(int	npoints, POINT3D	*pts, int	*size)
{
	LINE3D	*result;

	*size = sizeof(LINE3D) + (npoints)*sizeof(POINT3D);

	result= (LINE3D *) palloc (*size);

	result->npoints = npoints;
	memcpy(	result->points, pts, npoints*sizeof(POINT3D) );

	return result;
}

//given one byte, populate result with two byte representing
// the hex number
// ie deparse_hex( 255, mystr)
//		-> mystr[0] = 'F' and mystr[1] = 'F'
// no error checking done
void deparse_hex(unsigned char str, unsigned char *result)
{
	int	input_high;
	int  input_low;

	input_high = (str>>4);
	input_low = (str & 0x0F);

	switch (input_high)
	{
		case 0:
			result[0] = '0';
			break;
		case 1:
			result[0] = '1';
			break;
		case 2:
			result[0] = '2';
			break;
		case 3:
			result[0] = '3';
			break;
		case 4:
			result[0] = '4';
			break;
		case 5:
			result[0] = '5';
			break;
		case 6:
			result[0] = '6';
			break;
		case 7:
			result[0] = '7';
			break;
		case 8:
			result[0] = '8';
			break;
		case 9:
			result[0] = '9';
			break;
		case 10:
			result[0] = 'A';
			break;
		case 11:
			result[0] = 'B';
			break;
		case 12:
			result[0] = 'C';
			break;
		case 13:
			result[0] = 'D';
			break;
		case 14:
			result[0] = 'E';
			break;
		case 15:
			result[0] = 'F';
			break;
	}

	switch (input_low)
	{
		case 0:
			result[1] = '0';
			break;
		case 1:
			result[1] = '1';
			break;
		case 2:
			result[1] = '2';
			break;
		case 3:
			result[1] = '3';
			break;
		case 4:
			result[1] = '4';
			break;
		case 5:
			result[1] = '5';
			break;
		case 6:
			result[1] = '6';
			break;
		case 7:
			result[1] = '7';
			break;
		case 8:
			result[1] = '8';
			break;
		case 9:
			result[1] = '9';
			break;
		case 10:
			result[1] = 'A';
			break;
		case 11:
			result[1] = 'B';
			break;
		case 12:
			result[1] = 'C';
			break;
		case 13:
			result[1] = 'D';
			break;
		case 14:
			result[1] = 'E';
			break;
		case 15:
			result[1] = 'F';
			break;
	}
}


//given a string with at least 2 chars in it, convert them to
// a byte value.  No error checking done!
unsigned char	parse_hex(char *str)
{
	//do this a little brute force to make it faster

	unsigned char		result_high = 0;
	unsigned char		result_low = 0;

	switch (str[0])
	{
		case '0' :
			result_high = 0;
			break;
		case '1' :
			result_high = 1;
			break;
		case '2' :
			result_high = 2;
			break;
		case '3' :
			result_high = 3;
			break;
		case '4' :
			result_high = 4;
			break;
		case '5' :
			result_high = 5;
			break;
		case '6' :
			result_high = 6;
			break;
		case '7' :
			result_high = 7;
			break;
		case '8' :
			result_high = 8;
			break;
		case '9' :
			result_high = 9;
			break;
		case 'A' :
			result_high = 10;
			break;
		case 'B' :
			result_high = 11;
			break;
		case 'C' :
			result_high = 12;
			break;
		case 'D' :
			result_high = 13;
			break;
		case 'E' :
			result_high = 14;
			break;
		case 'F' :
			result_high = 15;
			break;
	}
	switch (str[1])
	{
		case '0' :
			result_low = 0;
			break;
		case '1' :
			result_low = 1;
			break;
		case '2' :
			result_low = 2;
			break;
		case '3' :
			result_low = 3;
			break;
		case '4' :
			result_low = 4;
			break;
		case '5' :
			result_low = 5;
			break;
		case '6' :
			result_low = 6;
			break;
		case '7' :
			result_low = 7;
			break;
		case '8' :
			result_low = 8;
			break;
		case '9' :
			result_low = 9;
			break;
		case 'A' :
			result_low = 10;
			break;
		case 'B' :
			result_low = 11;
			break;
		case 'C' :
			result_low = 12;
			break;
		case 'D' :
			result_low = 13;
			break;
		case 'E' :
			result_low = 14;
			break;
		case 'F' :
			result_low = 15;
			break;
	}
	return (unsigned char) ((result_high<<4) + result_low);
}


// input is a string with hex chars in it.  Convert to binary and put in the result
PG_FUNCTION_INFO_V1(WKB_in);
Datum WKB_in(PG_FUNCTION_ARGS)
{
	char	   		*str = PG_GETARG_CSTRING(0);
	WellKnownBinary 	*result;
	int			size;
	int			t;
	int			input_str_len;

//printf("wkb_in called\n");

	input_str_len = strlen(str);

	if  ( ( ( (int)(input_str_len/2.0) ) *2.0) != input_str_len)
	{
		elog(ERROR,"WKB_in parser - should be even number of characters!");
		PG_RETURN_NULL();
	}

 	if (strspn(str,"0123456789ABCDEF") != strlen(str) )
	{
		elog(ERROR,"WKB_in parser - input contains bad characters.  Should only have '0123456789ABCDEF'!");
		PG_RETURN_NULL();
	}
	size = (input_str_len/2) + 4;
	result = (WellKnownBinary *) palloc(size);
	result->size = size;

	for (t=0;t<input_str_len/2;t++)
	{
		((unsigned char *)result)[t+4] = parse_hex( &str[t*2]) ;
	}
	PG_RETURN_POINTER(result);
}


//given a WKB structure, convert it to Hex and put it in a string
PG_FUNCTION_INFO_V1(WKB_out);
Datum WKB_out(PG_FUNCTION_ARGS)
{
	WellKnownBinary		      *WKB = (WellKnownBinary *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char					*result;
	int					size_result;
	int					t;

//printf("wkb_out called\n");

	size_result = (WKB->size - 4) *2 +1; //+1 for null char
	result = palloc (size_result);
	result[size_result-1] = 0; //null terminate

	for (t=0; t< (WKB->size -4); t++)
	{
		deparse_hex( ((unsigned char *) WKB)[4 + t], &result[t*2]);
	}
	PG_RETURN_CSTRING(result);
}


#if USE_VERSION > 73
/*
 * This function must advance the StringInfo.cursor pointer
 * and leave it at the end of StringInfo.buf. If it fails
 * to do so the backend will raise an exception with message:
 * ERROR:  incorrect binary data format in bind parameter #
 *
 */
PG_FUNCTION_INFO_V1(WKB_recv);
Datum WKB_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
        bytea *result;

	//elog(NOTICE, "WKB_recv start");

	/* Add VARLENA size info to make it a valid varlena object */
	result = (bytea *)palloc(buf->len+VARHDRSZ);
	VARATT_SIZEP(result) = buf->len+VARHDRSZ;
	memcpy(VARATT_DATA(result), buf->data, buf->len);

	/* Set cursor to the end of buffer (so the backend is happy) */
	buf->cursor = buf->len;

	//elog(NOTICE, "WKB_recv end (returning result)");
        PG_RETURN_POINTER(result);
}
#endif

PG_FUNCTION_INFO_V1(WKBtoBYTEA);
Datum WKBtoBYTEA(PG_FUNCTION_ARGS)
{
		WellKnownBinary		      *WKB = (WellKnownBinary *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		bytea					  *result;

		result = (bytea*) palloc(WKB->size);
		memcpy(result,WKB, WKB->size);

		PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(geometry2box);
Datum geometry2box(PG_FUNCTION_ARGS)
{
	 GEOMETRY *g = (GEOMETRY *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

//elog(NOTICE,"geometry2box - ymax is %.15g",g->bvol.URT.y);

	PG_RETURN_POINTER(convert_box3d_to_box(&g->bvol) );

}

//create_histogram2d(BOX3D, boxesPerSide)
// returns a histgram with 0s in all the boxes.
PG_FUNCTION_INFO_V1(create_histogram2d);
Datum create_histogram2d(PG_FUNCTION_ARGS)
{
	BOX3D  *bbox = (BOX3D *) PG_GETARG_POINTER(0);
	int32	boxesPerSide=  PG_GETARG_INT32(1);
	HISTOGRAM2D		*histo;
	int size,t;


	if ( (boxesPerSide <1) || (boxesPerSide >50) )
	{
			elog(ERROR,"create_histogram2d - boxesPerSide is too small or big.\n");
			PG_RETURN_NULL() ;
	}


	size =  sizeof(HISTOGRAM2D) +    (boxesPerSide*boxesPerSide-1)*4  ;

	histo = (HISTOGRAM2D *) palloc (size);
	histo->size = size;

	histo->xmin = bbox->LLB.x;
	histo->ymin = bbox->LLB.y;


	histo->xmax = bbox->URT.x;
	histo->ymax = bbox->URT.y;

	histo->avgFeatureArea = 0;

	histo->boxesPerSide = boxesPerSide;

	for (t=0;t<boxesPerSide*boxesPerSide; t++)
	{
		histo->value[t] =0;
	}

	//elog(NOTICE,"create_histogram2d returning");

	PG_RETURN_POINTER(histo);

}


//text form of HISTOGRAM2D is:
// 'HISTOGRAM2D(xmin,ymin,xmax,ymax,boxesPerSide;value[0],value[1],...')
//    note the ";" in the middle (for easy parsing)
//  I dont expect anyone to actually create one by hand
PG_FUNCTION_INFO_V1(histogram2d_in);
Datum histogram2d_in(PG_FUNCTION_ARGS)
{
	char	   		*str = PG_GETARG_CSTRING(0);
	HISTOGRAM2D	    *histo ;
	int nitems;
	double xmin,ymin,xmax,ymax;
	int boxesPerSide;
	double avgFeatureArea;
	char *str2,*str3;
	long datum;

	int t;

	while (isspace((unsigned char) *str))
		str++;

	if (strstr(str,"HISTOGRAM2D(") != str)
	{
			elog(ERROR,"histogram2d parser - doesnt start with 'HISTOGRAM2D(\n");
			PG_RETURN_NULL() ;
	}
	if (strstr(str,";") == NULL)
	{
			elog(ERROR,"histogram2d parser - doesnt have a ; in sring!\n");
			PG_RETURN_NULL() ;
	}

	nitems = sscanf(str,"HISTOGRAM2D(%lf,%lf,%lf,%lf,%i,%lf;",&xmin,&ymin,&xmax,&ymax,&boxesPerSide,&avgFeatureArea);

	if (nitems != 6)
	{
			elog(ERROR,"histogram2d parser - couldnt parse initial portion of histogram!\n");
			PG_RETURN_NULL() ;
	}

	if  ( (boxesPerSide > 50) || (boxesPerSide <1) )
	{
			elog(ERROR,"histogram2d parser - boxesPerSide is too big or too small\n");
			PG_RETURN_NULL() ;
	}

	str2 = strstr(str,";");
	str2++;

	if (str2[0] ==0)
	{
			elog(ERROR,"histogram2d parser - no histogram values\n");
			PG_RETURN_NULL() ;
	}

	histo = (HISTOGRAM2D *) palloc (sizeof(HISTOGRAM2D) +    (boxesPerSide*boxesPerSide-1)*4 );
	histo->size = sizeof(HISTOGRAM2D) +    (boxesPerSide*boxesPerSide-1)*4  ;

	for (t=0;t<boxesPerSide*boxesPerSide;t++)
	{
		datum = strtol(str2,&str3,10); // str2=start of int, str3=end of int, base 10
		// str3 points to "," or ")"
		if (str3[0] ==0)
		{
			elog(ERROR,"histogram2d parser - histogram values prematurely ended!\n");
			PG_RETURN_NULL() ;
		}
		histo->value[t] = (unsigned int) datum;
		str2= str3+1; //move past the "," or ")"
	}
	histo->xmin = xmin;
	histo->xmax = xmax;
	histo->ymin = ymin;
	histo->ymax = ymax;
	histo->avgFeatureArea = avgFeatureArea;
	histo->boxesPerSide = boxesPerSide;

	PG_RETURN_POINTER(histo);
}



//text version
PG_FUNCTION_INFO_V1(histogram2d_out);
Datum histogram2d_out(PG_FUNCTION_ARGS)
{
	//char *result;
	//result = palloc(200);
	//sprintf(result,"HISTOGRAM2D(0,0,100,100,2;11,22,33,44)");
	//PG_RETURN_CSTRING(result);

		HISTOGRAM2D   *histo = (HISTOGRAM2D *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		char	*result;
		int t;
		char	temp[100];
		int size;

		size = 26+6*MAX_DIGS_DOUBLE + histo->boxesPerSide*histo->boxesPerSide* (MAX_DIGS_DOUBLE+1);
		result = palloc(size);

		sprintf(result,"HISTOGRAM2D(%.15g,%.15g,%.15g,%.15g,%i,%.15g;",
					histo->xmin,histo->ymin,histo->xmax,histo->ymax,histo->boxesPerSide,histo->avgFeatureArea );

		//elog(NOTICE,"so far: %s",result);
		//elog(NOTICE,"buffsize=%i, size=%i",size,histo->size);

		for (t=0;t<histo->boxesPerSide*histo->boxesPerSide;t++)
		{
			if (t!=((histo->boxesPerSide*histo->boxesPerSide)-1))
				sprintf(temp,"%u,", histo->value[t]);
			else
				sprintf(temp,"%u", histo->value[t]);
			strcat(result,temp);
		}

		strcat(result,")");
		//elog(NOTICE,"about to return string (len=%i): -%s-",strlen(result),result);

		PG_RETURN_CSTRING(result);

}



int getint(char *c)
{
	return *((int*)c);
}

double getdouble(char *c)
{
	return *((double*)c);
}

//void	flip_endian_double(char	*dd);
//void		flip_endian_int32(char 	*ii);


//select geometry(asbinary('POINT(1 2 3)','XDR'));
//select geometry(asbinary('POINT(1 2)','XDR'));
//select geometry(asbinary('POINT(1 2 3)','NDR'));
//select geometry(asbinary('POINT(1 2)','NDR'));

//select geometry(asbinary('LINESTRING(1 2, 3 4)','XDR'));
//select geometry(asbinary('LINESTRING(1 2, 3 4)','NDR'));
//select geometry(asbinary('LINESTRING(1 2 5 , 3 4 6)','XDR'));
//select geometry(asbinary('LINESTRING(1 2 5 , 3 4 6)','NDR'));


//select geometry(asbinary('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','XDR'));
//select geometry(asbinary('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))','NDR'));
//select geometry(asbinary('POLYGON((0 0 0, 10 0, 10 10, 0 10, 0 0))','XDR'));
//select geometry(asbinary('POLYGON((0 0 0, 10 0, 10 10, 0 10, 0 0))','NDR'));

//select geometry(asbinary('POLYGON((5 5, 15 5, 15 7, 5 7, 5 5 ),(6 6,6.5 6, 6.5 6.5,6 6.5,6 6))','NDR'));
//select geometry(asbinary('POLYGON((5 5, 15 5, 15 7, 5 7, 5 5 ),(6 6,6.5 6, 6.5 6.5,6 6.5,6 6))','XDR'));
//select geometry(asbinary('POLYGON((5 5 0, 15 5, 15 7, 5 7, 5 5 ),(6 6,6.5 6, 6.5 6.5,6 6.5,6 6))','NDR'));
//select geometry(asbinary('POLYGON((5 5 0, 15 5, 15 7, 5 7, 5 5 ),(6 6,6.5 6, 6.5 6.5,6 6.5,6 6))','XDR'));


//select geometry(asbinary('MULTIPOINT(0 0, 10 0, 10 10, 0 10, 0 0)','NDR'));
//select geometry(asbinary('MULTIPOINT(0 0, 10 0, 10 10, 0 10, 0 0 0)','NDR'));
//select geometry(asbinary('MULTIPOINT(0 0, 10 0, 10 10, 0 10, 0 0)','XDR'));
//select geometry(asbinary('MULTIPOINT(0 0, 10 0, 10 10, 0 10, 0 0 0)','NDR'));


//select geometry(asbinary('MULTILINESTRING((5 5, 10 10),(1 1, 2 2) )','NDR'));
//select geometry(asbinary('MULTILINESTRING((5 5, 10 10),(1 1, 2 2 0) )','NDR'));
//select geometry(asbinary('MULTILINESTRING((5 5, 10 10),(1 1, 2 2) )','XDR'));
//select geometry(asbinary('MULTILINESTRING((5 5, 10 10),(1 1, 2 2 0) )','XDR'));


//select geometry(asbinary('MULTIPOLYGON(((5 5, 15 5, 15 7, 5 7, 5 5)),((1 1,1 2,2 2,1 2, 1 1)))','NDR'));
//select geometry(asbinary('MULTIPOLYGON(((5 5, 15 5, 15 7, 5 7, 5 5)),((1 1,1 2,2 2,1 2, 1 1 0)))','NDR'));
//select geometry(asbinary('MULTIPOLYGON(((5 5, 15 5, 15 7, 5 7, 5 5)),((1 1,1 2,2 2,1 2, 1 1)))','XDR'));
//select geometry(asbinary('MULTIPOLYGON(((5 5, 15 5, 15 7, 5 7, 5 5)),((1 1,1 2,2 2,1 2, 1 1 0)))','XDR'));


//select geometry(asbinary('GEOMETRYCOLLECTION(POINT(1 2),POINT(3 4))','NDR'));
//select geometry(asbinary('GEOMETRYCOLLECTION(POINT(1 2 3),LINESTRING(1 2, 3 4),POLYGON((0 0, 10 0, 10 10, 0 10, 0 0)))','NDR'));
// geometryfromWKB(<WKB>, [<SRID>] )
PG_FUNCTION_INFO_V1(geometryfromWKB_SRID);
Datum geometryfromWKB_SRID(PG_FUNCTION_ARGS)
{
		char   *wkb_input = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int    SRID = PG_GETARG_INT32(1);
		char *wkb = &wkb_input[4];
		int wkb_size = *((int *) wkb_input);
		int bytes_read;
		GEOMETRY *result;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;

//elog(NOTICE,"geometryfromWKB:: size = %i",wkb_size);

	result = WKBtoGeometry(wkb,wkb_size,&bytes_read);

	if (result == NULL)
	{
		PG_RETURN_NULL();
	}
	else
	{
		result->SRID = SRID;
		PG_RETURN_POINTER(result);
	}
}

PG_FUNCTION_INFO_V1(PointfromWKB_SRID);
Datum PointfromWKB_SRID(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometryfromWKB_SRID,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != POINTTYPE)
		{
			elog(ERROR,"PointfromWKB:: WKB isnt POINT");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(LinefromWKB_SRID);
Datum LinefromWKB_SRID(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometryfromWKB_SRID,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != LINETYPE)
		{
			elog(ERROR,"LinefromWKB_SRID:: WKB isnt LINESTRING");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(PolyfromWKB_SRID);
Datum PolyfromWKB_SRID(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometryfromWKB_SRID,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != POLYGONTYPE)
		{
			elog(ERROR,"PolyfromWKB_SRID:: WKB isnt POLYGON");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(MPointfromWKB_SRID);
Datum MPointfromWKB_SRID(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometryfromWKB_SRID,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != MULTIPOINTTYPE)
		{
			elog(ERROR,"MPointfromWKB_SRID:: WKB isnt MULTIPOINT");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(MLinefromWKB_SRID);
Datum MLinefromWKB_SRID(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometryfromWKB_SRID,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != MULTILINETYPE)
		{
			elog(ERROR,"MLinefromWKB_SRID:: WKB isnt MULTILINESTRING");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(MPolyfromWKB_SRID);
Datum MPolyfromWKB_SRID(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometryfromWKB_SRID,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != MULTIPOLYGONTYPE)
		{
			elog(ERROR,"MPolyfromWKB_SRID:: WKB isnt MULTIPOLYGON");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(GCfromWKB_SRID);
Datum GCfromWKB_SRID(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometryfromWKB_SRID,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != COLLECTIONTYPE)
		{
			elog(ERROR,"MPolyfromWKB_SRID:: WKB isnt GEOMETRYCOLLECTION");
		}
		PG_RETURN_POINTER(geom);
}




//convert a WKB (that has length bytes)
// to a GEOMETRY.  This function read bytes_read bytes during the operation
//  This function will call itself "recursively" on GeometryCollections
GEOMETRY *WKBtoGeometry(char *WKB, int length, int *bytes_read)
{
			char myByteOrder;
			char wkbByteOrder;
			int  wkbType;
			char	is3d;

*bytes_read = 0;
if (length<5)
	elog(ERROR,"WKB:: insufficient bytes in stream");


if ( BYTE_ORDER == BIG_ENDIAN )
	myByteOrder=0;
else
	myByteOrder=1;

wkbByteOrder = *(WKB);

if (!( (wkbByteOrder==0) || (wkbByteOrder==1) ))
{
	elog(ERROR,"WKB is not valid - endian code = %i", (int) wkbByteOrder);
	return NULL;
}

WKB ++; // skip to next byte
(*bytes_read)++;

if (myByteOrder == wkbByteOrder)
{
	wkbType = getint(WKB);
}
else
{
	flip_endian_int32( WKB );
	wkbType = getint(WKB);
}
WKB += 4;
(*bytes_read) += 4;

//elog(NOTICE,"my byte order is %i",myByteOrder);
//elog(NOTICE,"WKB byte order is %i",wkbByteOrder);
//elog(NOTICE,"WKB type is %i",wkbType);

is3d = 0;

switch (wkbType)
{
	case (0x80000000 +1):  //point 3d
			is3d = 1;
	case 1:  //point 2d
			if ( (length-(*bytes_read)) < (16+is3d*8))
				elog(ERROR,"WKB:: insufficient bytes in stream");
			{
				POINT3D pt;
				if (myByteOrder == wkbByteOrder)
				{
					pt.x = getdouble(WKB);
					WKB+=8;
					(*bytes_read)+=8;
					pt.y = getdouble(WKB);
					WKB+=8;
					(*bytes_read)+=8;
					if (is3d)
					{
						pt.z = getdouble(WKB);
						WKB+=8;
						(*bytes_read)+=8;
					}
					else
					{
						pt.z=0;
					}
				}
				else
				{
					flip_endian_double(WKB);
					pt.x = getdouble(WKB);
					WKB+=8;
					(*bytes_read)+=8;
					flip_endian_double(WKB);
					pt.y = getdouble(WKB);
					WKB+=8;
					(*bytes_read)+=8;
					if (is3d)
					{
						flip_endian_double(WKB);
						pt.z = getdouble(WKB);
						WKB+=8;
						(*bytes_read)+=8;
					}
					else
					{
						pt.z =0;
					}
				}
				return make_oneobj_geometry(sizeof(POINT3D),
											(char *) &pt,
											POINTTYPE,  is3d, -1,1.0, 0.0, 0.0
						);
			}


			break;
	case (0x80000000 +2):  //line 3d
			is3d = 1;
	case 2: //line 2d
			if ( (length-(*bytes_read)) < (4))
				elog(ERROR,"WKB:: insufficient bytes in stream");
				{
						int npoints,t;
						POINT3D	*pts;
						int size;
						LINE3D	*line;
						if (myByteOrder == wkbByteOrder)
						{
							npoints= getint(WKB);
							WKB+=4;
							(*bytes_read)+=4;
							if ( (length-(*bytes_read)) < ((16+is3d*8))*npoints)
								elog(ERROR,"WKB:: insufficient bytes in stream");

							pts = palloc( npoints *sizeof(POINT3D));
							for (t=0;t<npoints;t++)
							{
								pts[t].x = getdouble(WKB);
								WKB+=8;
								(*bytes_read)+=8;
								pts[t].y = getdouble(WKB);
								WKB+=8;
								(*bytes_read)+=8;
								if (is3d)
								{
									pts[t].z = getdouble(WKB);
									WKB+=8;
									(*bytes_read)+=8;
								}
								else
								{
									pts[t].z =0;
								}
							}
							//make_line(int  npoints, POINT3D    *pts, int   *size)
							line = make_line(npoints, pts,&size);
							return make_oneobj_geometry(size,
														(char *) line,
														LINETYPE,  is3d, -1,1.0, 0.0, 0.0
											);
						}
						else
						{
							flip_endian_int32(WKB);
							npoints= getint(WKB);
							WKB+=4;
							(*bytes_read)+=4;
							if ( (length-(*bytes_read)) < ((16+is3d*8))*npoints)
								elog(ERROR,"WKB:: insufficient bytes in stream");
							pts = palloc( npoints *sizeof(POINT3D));
							for (t=0;t<npoints;t++)
							{
								flip_endian_double(WKB);
								pts[t].x = getdouble(WKB);
								WKB+=8;
								(*bytes_read)+=8;
								flip_endian_double(WKB);
								pts[t].y = getdouble(WKB);
								WKB+=8;
								(*bytes_read)+=8;
								if (is3d)
								{
									flip_endian_double(WKB);
									pts[t].z = getdouble(WKB);
									WKB+=8;
									(*bytes_read)+=8;
								}
								else
								{
									pts[t].z =0;
								}
							}
							//make_line(int  npoints, POINT3D    *pts, int   *size)
							line = make_line(npoints, pts,&size);
							return make_oneobj_geometry(size,
														(char *) line,
														LINETYPE,  is3d, -1,1.0, 0.0, 0.0
												);
						}
					}
			break;
	case (0x80000000 +3):  //polygon 3d
			is3d =1;
	case 3: //polygon
		if ( (length-(*bytes_read)) < (4))
				elog(ERROR,"WKB:: insufficient bytes in stream");
		{
				int nrings;
				POINT3D** list_list_points;
				int32  *points_per_ring;
				int t;
				int total_points =0;
				POLYGON3D *poly;
				int size;
				POINT3D *pts;
				int point_offset;

			if (myByteOrder == wkbByteOrder)
			{
				nrings= getint(WKB);
				WKB+=4;
				(*bytes_read)+=4;
				list_list_points = palloc( sizeof(POINT3D*) * nrings);
				points_per_ring  = palloc( sizeof(int32*) * nrings);
				for (t=0;t<nrings;t++)
				{
					int bytes, numbPoints;

					list_list_points[t] =
							wkb_linearring(WKB, is3d, 0, &numbPoints, &bytes,length-(*bytes_read));
					points_per_ring[t] = numbPoints;
					total_points += numbPoints;
					WKB += bytes;
					(*bytes_read)+=bytes;
				}

			}
			else
			{
				flip_endian_int32(WKB);
				nrings= getint(WKB);
				WKB+=4;
				(*bytes_read)+=4;
				list_list_points = palloc( sizeof(POINT3D*) * nrings);
				points_per_ring  = palloc( sizeof(int32*) * nrings);
				for (t=0;t<nrings;t++)
				{
					int bytes, numbPoints;

					list_list_points[t] =
							wkb_linearring(WKB, is3d, 1, &numbPoints, &bytes,length-(*bytes_read));
					points_per_ring[t] = numbPoints;
					total_points += numbPoints;
					WKB += bytes;
					(*bytes_read)+=bytes;
				}
			}
			//compact point arrays
			pts = palloc ( sizeof(POINT3D) * total_points);
			point_offset  = 0;
			for (t=0;t<nrings;t++)
			{
				memcpy( &pts[point_offset],list_list_points[t], sizeof(POINT3D)*points_per_ring[t]);
				point_offset += points_per_ring[t];
			}
			poly = make_polygon( nrings, points_per_ring, pts, total_points, &size);
			return make_oneobj_geometry(size,
										(char *) poly,
										POLYGONTYPE,  is3d, -1,1.0, 0.0, 0.0
					);
		}

			break;
	case (0x80000000 +4):  //multipoint 3d
	case 4: //multipoint
				{
					int ngeoms,t;
					GEOMETRY *so_far,*so_far2;
					GEOMETRY *one_obj;

					int mybytes_read;
					int other_bytes_read;

					if (myByteOrder != wkbByteOrder)
					{
						flip_endian_int32(WKB);
					}
					ngeoms= getint(WKB);
					WKB+=4;
					(*bytes_read)+=4;
					if (ngeoms ==0)
					{
						GEOMETRY *result= makeNullGeometry(-1);
						result->type = MULTIPOINTTYPE;
						return result;
					}

						mybytes_read = *bytes_read;
						so_far = WKBtoGeometry(WKB, length - mybytes_read, &other_bytes_read);
						(*bytes_read) =  mybytes_read + other_bytes_read;
						WKB += other_bytes_read;
						so_far->type = MULTIPOINTTYPE;

						if (ngeoms ==1)
							return so_far;

					for (t=1;t<ngeoms;t++) // already done the first
					{
						mybytes_read = *bytes_read;
						one_obj = WKBtoGeometry(WKB, length - mybytes_read, &other_bytes_read);
						(*bytes_read) =  mybytes_read + other_bytes_read;
						WKB += other_bytes_read;
						so_far2 =
							(GEOMETRY *) DatumGetPointer(
										DirectFunctionCall2(collector,
												PointerGetDatum(so_far),PointerGetDatum(one_obj)
											)
									);
						pfree(one_obj);
						pfree(so_far);
						so_far = so_far2;
					}
					return so_far;
				}
	case (0x80000000 +5):  //multiline 3d
	case 5: //multiline
			{
					int ngeoms,t;
					GEOMETRY *so_far,*so_far2;
					GEOMETRY *one_obj;

					int mybytes_read;
					int other_bytes_read;

					if (myByteOrder != wkbByteOrder)
					{
						flip_endian_int32(WKB);
					}
					ngeoms= getint(WKB);
					WKB+=4;
					(*bytes_read)+=4;
					if (ngeoms ==0)
					{
						GEOMETRY *result= makeNullGeometry(-1);
						result->type = MULTILINETYPE;
						return result;
					}

						mybytes_read = *bytes_read;
						so_far = WKBtoGeometry(WKB, length - mybytes_read, &other_bytes_read);
						(*bytes_read) =  mybytes_read + other_bytes_read;
						WKB += other_bytes_read;
						so_far->type = MULTILINETYPE;

						if (ngeoms ==1)
							return so_far;

					for (t=1;t<ngeoms;t++) // already done the first
					{
						mybytes_read = *bytes_read;
						one_obj = WKBtoGeometry(WKB, length - mybytes_read, &other_bytes_read);
						(*bytes_read) =  mybytes_read + other_bytes_read;
						WKB += other_bytes_read;
						so_far2 =
							(GEOMETRY *) DatumGetPointer(
										DirectFunctionCall2(collector,
												PointerGetDatum(so_far),PointerGetDatum(one_obj)
											)
									);
						pfree(one_obj);
						pfree(so_far);
						so_far = so_far2;
					}
					return so_far;
				}
	case (0x80000000 +6):  //multipolygon 3d
	case 6: //multipolygon
			{
				int ngeoms,t;
				GEOMETRY *so_far,*so_far2;
				GEOMETRY *one_obj;

				int mybytes_read;
				int other_bytes_read;

				if (myByteOrder != wkbByteOrder)
				{
					flip_endian_int32(WKB);
				}
				ngeoms= getint(WKB);
				WKB+=4;
				(*bytes_read)+=4;
				if (ngeoms ==0)
				{
					GEOMETRY *result= makeNullGeometry(-1);
					result->type = MULTIPOLYGONTYPE;
					return result;
				}

					mybytes_read = *bytes_read;
					so_far = WKBtoGeometry(WKB, length - mybytes_read, &other_bytes_read);
					(*bytes_read) =  mybytes_read + other_bytes_read;
					WKB += other_bytes_read;
					so_far->type = MULTIPOLYGONTYPE;

					if (ngeoms ==1)
						return so_far;

				for (t=1;t<ngeoms;t++) // already done the first
				{
					mybytes_read = *bytes_read;
					one_obj = WKBtoGeometry(WKB, length - mybytes_read, &other_bytes_read);
					(*bytes_read) =  mybytes_read + other_bytes_read;
					WKB += other_bytes_read;
					so_far2 =
						(GEOMETRY *) DatumGetPointer(
									DirectFunctionCall2(collector,
											PointerGetDatum(so_far),PointerGetDatum(one_obj)
										)
								);
					pfree(one_obj);
					pfree(so_far);
					so_far = so_far2;
				}
				return so_far;
			}
	case (0x80000000 +7):  //geometry collection 3d
	case 7: //geometry collection
			{
				int ngeoms,t;
				GEOMETRY *so_far,*so_far2;
				GEOMETRY *one_obj;

				int mybytes_read;
				int other_bytes_read;

				if (myByteOrder != wkbByteOrder)
				{
					flip_endian_int32(WKB);
				}
				ngeoms= getint(WKB);
				WKB+=4;
				(*bytes_read)+=4;
				if (ngeoms ==0)
				{
					GEOMETRY *result= makeNullGeometry(-1);
					return result;
				}

					mybytes_read = *bytes_read;
					so_far = WKBtoGeometry(WKB, length - mybytes_read, &other_bytes_read);
					(*bytes_read) =  mybytes_read + other_bytes_read;
					WKB += other_bytes_read;
					so_far->type = COLLECTIONTYPE;

					if (ngeoms ==1)
						return so_far;

				for (t=1;t<ngeoms;t++) // already done the first
				{
					mybytes_read = *bytes_read;
					one_obj = WKBtoGeometry(WKB, length - mybytes_read, &other_bytes_read);
					(*bytes_read) =  mybytes_read + other_bytes_read;
					WKB += other_bytes_read;
					so_far2 =
						(GEOMETRY *) DatumGetPointer(
									DirectFunctionCall2(collector,
											PointerGetDatum(so_far),PointerGetDatum(one_obj)
										)
								);
					pfree(one_obj);
					pfree(so_far);
					so_far = so_far2;
				}
				return so_far;
			}
	default:
		elog(ERROR,"unknown WKB type: %i",wkbType);
		return NULL;
}
return NULL;
}

// take some wkt (and if its 3d or needs endianflip) and return as list of points
//  also return the number of points and its size in bytes
//
POINT3D *wkb_linearring(char *WKB,char is3d, char flip_endian, int *numbPoints, int *bytes,int bytes_in_stream)
{
	int t;

	if (bytes_in_stream < 4)
		elog(ERROR,"WKB:: insufficient bytes in stream");

	if (flip_endian)
	{
		POINT3D	*pts;
		int npoints;

		flip_endian_int32(WKB);
		npoints= getint(WKB);
		WKB+=4;
		if ( (bytes_in_stream-4) < ((16+is3d*8))*npoints)
					elog(ERROR,"WKB:: insufficient bytes in stream");
		pts = palloc( npoints *sizeof(POINT3D));
		for (t=0;t<npoints;t++)
		{
			flip_endian_double(WKB);
			pts[t].x = getdouble(WKB);
			WKB+=8;
			flip_endian_double(WKB);
			pts[t].y = getdouble(WKB);
			WKB+=8;
			if (is3d)
			{
				flip_endian_double(WKB);
				pts[t].z = getdouble(WKB);
				WKB+=8;
			}
			else
			{
				pts[t].z =0;
			}
		}
		*numbPoints = npoints;
		if (is3d)
		{
			*bytes = 4+8*3*npoints;
		}
		else
		{
			*bytes = 4+8*2*npoints;
		}
		return pts;
	}
	else
	{
		POINT3D	*pts;
		int npoints;

		npoints= getint(WKB);
		WKB+=4;
		if ( (bytes_in_stream-4) < ((16+is3d*8))*npoints)
					elog(ERROR,"WKB:: insufficient bytes in stream");
		pts = palloc( npoints *sizeof(POINT3D));
		for (t=0;t<npoints;t++)
		{
			pts[t].x = getdouble(WKB);
			WKB+=8;
			pts[t].y = getdouble(WKB);
			WKB+=8;
			if (is3d)
			{
				pts[t].z = getdouble(WKB);
				WKB+=8;
			}
			else
			{
				pts[t].z =0;
			}
		}
		*numbPoints = npoints;
		if (is3d)
		{
			*bytes = 4+8*3*npoints;
		}
		else
		{
			*bytes = 4+8*2*npoints;
		}
		return pts;
	}
}



PG_FUNCTION_INFO_V1(geometry_from_text_poly);
Datum geometry_from_text_poly(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometry_from_text,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != POLYGONTYPE)
		{
			elog(ERROR,"geometry_from_text_poly:: WKT isnt POLYGON");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(geometry_from_text_line);
Datum geometry_from_text_line(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometry_from_text,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != LINETYPE)
		{
			elog(ERROR,"geometry_from_text_line:: WKT isnt LINESTRING");
		}
		PG_RETURN_POINTER(geom);
}


PG_FUNCTION_INFO_V1(geometry_from_text_point);
Datum geometry_from_text_point(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometry_from_text,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != POINTTYPE)
		{
			elog(ERROR,"geometry_from_text_point:: WKT isnt POINT");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(geometry_from_text_mpoint);
Datum geometry_from_text_mpoint(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometry_from_text,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != MULTIPOINTTYPE)
		{
			elog(ERROR,"geometry_from_text_mpoint:: WKT isnt MULTIPOINT");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(geometry_from_text_mline);
Datum geometry_from_text_mline(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometry_from_text,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != MULTILINETYPE)
		{
			elog(ERROR,"geometry_from_text_mline:: WKT isnt MULTILINESTRING");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(geometry_from_text_mpoly);
Datum geometry_from_text_mpoly(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometry_from_text,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != MULTIPOLYGONTYPE)
		{
			elog(ERROR,"geometry_from_text_mpoly:: WKT isnt MULTIPOLYGON");
		}
		PG_RETURN_POINTER(geom);
}

PG_FUNCTION_INFO_V1(geometry_from_text_gc);
Datum geometry_from_text_gc(PG_FUNCTION_ARGS)
{
		int SRID;
		GEOMETRY *geom;

		if ( ! PG_ARGISNULL(1) )
			SRID = PG_GETARG_INT32(1);
		else
			SRID = -1;


		geom = (GEOMETRY *) DatumGetPointer(
					DirectFunctionCall2(geometry_from_text,
							PG_GETARG_DATUM(0),Int32GetDatum(SRID)
						));
		if (geom->type != COLLECTIONTYPE)
		{
			elog(ERROR,"geometry_from_text_gc:: WKT isnt GEOMETRYCOLLECTION");
		}
		PG_RETURN_POINTER(geom);
}


//returns a GEOMETRYCOLLECTION(EMPTY)
// bbox of this object is BOX3D(0 0 0, 0 0 0)
//   but should be treated as NULL
GEOMETRY *makeNullGeometry(int SRID)
{
		int size = sizeof(GEOMETRY);
		GEOMETRY	*result = palloc(size);


		memset(result,0, size ); // init to 0s

		result->size = size;
		result->nobjs = 0;
		result->type = COLLECTIONTYPE;
		result->is3d = false;


		result->SRID = SRID;
		result->scale = 1.0;
		result->offsetX = 0;
		result->offsetY = 0;

		memset(&result->bvol,0, sizeof(BOX3D) );  //make bbox :: BOX3D(0 0 0, 0 0 0)

	return result;
}
