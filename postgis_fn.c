/*************************************************************
 postGIS - geometric types for postgres

 This software is copyrighted (2001).
 
 This is free software; you can redistribute it and/or modify
 it under the GNU General Public Licence.  See the file "COPYING".

 More Info?  See the documentation, join the mailing list 
 (postgis@yahoogroups.com), or see the web page
 (http://postgis.refractions.net).

 *************************************************************/



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
#include "utils/elog.h"


//Norman Vine found this problem for compiling under cygwin
//  it defines BYTE_ORDER and LITTLE_ENDIAN 

#ifdef __CYGWIN__
#include <sys/param.h>       // FOR ENDIAN DEFINES
#endif


#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


// #define DEBUG_GIST
//#define DEBUG_GIST2




/**************************************************************************
 * GENERAL PURPOSE GEOMETRY FUNCTIONS
 **************************************************************************/


double line_length2d(LINE3D *line)
{
	int	i;
	POINT3D	*frm,*to;
	double	dist = 0.0;

	if (line->npoints <2)
		return 0.0;	//must have >1 point to make sense

	frm = &line->points[0];

	for (i=1; i<line->npoints;i++)
	{
		to = &line->points[i];
	
		dist += sqrt( ( (frm->x - to->x)*(frm->x - to->x) )  +
			  	  ( (frm->y - to->y)*(frm->y - to->y) ) );

		frm = to;
	}
	return dist;
}

double line_length3d(LINE3D *line)
{
	int	i;
	POINT3D	*frm,*to;
	double	dist = 0.0;

	if (line->npoints <2)
		return 0.0;	//must have >1 point to make sense

	frm = &line->points[0];

	for (i=1; i<line->npoints;i++)
	{
		to = &line->points[i];
	
		dist += sqrt( ( (frm->x - to->x)*(frm->x - to->x) )  +
				((frm->y - to->y)*(frm->y - to->y) ) + 
				((frm->z - to->z)*(frm->z - to->z) ) );

		frm = to;
	}
	return dist;
}


//find the "length of a geometry"
// length3d(point) = 0
// length3d(line) = length of line
// length3d(polygon) = 0  -- could make sense to return sum(ring perimeter)
// uses euclidian 3d length
 
PG_FUNCTION_INFO_V1(length3d);
Datum length3d(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	LINE3D			*line;
	double			dist = 0.0;

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;


	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];
		if (type1 == LINETYPE)	//LINESTRING
		{
			line = (LINE3D *) o1;
			dist += line_length3d(line);
		}
	}
	PG_RETURN_FLOAT8(dist);
}

//find the "length of a geometry"
// length3d(point) = 0
// length3d(line) = length of line
// length3d(polygon) = 0  -- could make sense to return sum(ring perimeter)
// uses euclidian 2d length
 
PG_FUNCTION_INFO_V1(length2d);
Datum length2d(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	LINE3D			*line;
	double			dist = 0.0;

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;


	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];
		if (type1 == LINETYPE)	//LINESTRING
		{
			line = (LINE3D *) o1;
			dist += line_length2d(line);
		}
	}
	PG_RETURN_FLOAT8(dist);
}


//find the 2d area of the outer ring - sum (area 2d of inner rings)
// Could use a more numerically stable calculator...
double polygon_area2d_old(POLYGON3D *poly1)
{
	double	poly_area=0.0, ringarea=0.0;
	int i,j,ring,pt_offset;
	POINT3D	*pts1;


	pts1 = (POINT3D *) ( (char *)&(poly1->npoints[poly1->nrings] )  );
	pts1 = (POINT3D *) MAXALIGN(pts1);


	
	pt_offset = 0; //index to first point in ring
	for (ring = 0; ring < poly1->nrings; ring++)
	{
		ringarea = 0.0;
		
		for (i=0;i<(poly1->npoints[ring]);i++) 
    		{ 
			j = (i+1) % (poly1->npoints[ring]);
			ringarea  += pts1[pt_offset+ i].x * pts1[pt_offset+j].y - pts1[pt_offset+ i].y * pts1[pt_offset+j].x; 
		} 

		ringarea  /= 2.0; 

		ringarea  = fabs(ringarea );
		if (ring != 0)	//outer
			ringarea  = -1.0*ringarea ; // its a hole
		
		poly_area += ringarea;

		pt_offset += poly1->npoints[ring];	
	}

	return poly_area;
}



//calculate the area of all the subobj in a polygon
// area(point) = 0
// area (line) = 0
// area(polygon) = find its 2d area
PG_FUNCTION_INFO_V1(area2d);
Datum area2d(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	POLYGON3D			*poly;
	double			area = 0.0;

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];
		if (type1 == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;
			area += polygon_area2d_old(poly);
		}
	}
	PG_RETURN_FLOAT8(area);
}

double 	polygon_perimeter3d(POLYGON3D	*poly1)
{
	double	poly_perimeter=0.0, ring_perimeter=0.0;
	int i,ring,pt_offset;
	POINT3D	*pts1;
	POINT3D	*to,*frm;


	pts1 = (POINT3D *) ( (char *)&(poly1->npoints[poly1->nrings] )  );
	pts1 = (POINT3D *) MAXALIGN(pts1);
	

	
	pt_offset = 0; //index to first point in ring
	for (ring = 0; ring < poly1->nrings; ring++)
	{
		ring_perimeter = 0.0;
		

		frm = &pts1[pt_offset];

		for (i=1; i<poly1->npoints[ring];i++)
		{
			to = &pts1[pt_offset+ i];
	
			ring_perimeter += sqrt( ( (frm->x - to->x)*(frm->x - to->x) )  +
				((frm->y - to->y)*(frm->y - to->y) ) + 
				((frm->z - to->z)*(frm->z - to->z) ) );

			frm = to;
		}
		
		poly_perimeter += ring_perimeter;

		pt_offset += poly1->npoints[ring];	
	}

	return poly_perimeter;

}

double 	polygon_perimeter2d(POLYGON3D	*poly1)
{
	double	poly_perimeter=0.0, ring_perimeter=0.0;
	int i,ring,pt_offset;
	POINT3D	*pts1;
	POINT3D	*to,*frm;


	pts1 = (POINT3D *) ( (char *)&(poly1->npoints[poly1->nrings] )  );
	pts1 = (POINT3D *) MAXALIGN(pts1);

	
	pt_offset = 0; //index to first point in ring
	for (ring = 0; ring < poly1->nrings; ring++)
	{
		ring_perimeter = 0.0;
		

		frm = &pts1[pt_offset];

		for (i=1; i<poly1->npoints[ring];i++)
		{
			to = &pts1[pt_offset+ i];
	
			ring_perimeter += sqrt( ( (frm->x - to->x)*(frm->x - to->x) )  +
				((frm->y - to->y)*(frm->y - to->y) )   );

			frm = to;
		}
		
		poly_perimeter += ring_perimeter;

		pt_offset += poly1->npoints[ring];	
	}

	return poly_perimeter;

}



//calculate the perimeter of polys (sum of length of all rings)
PG_FUNCTION_INFO_V1(perimeter3d);
Datum perimeter3d(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	POLYGON3D			*poly;
	double			area = 0.0;

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];
		if (type1 == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;
			area += polygon_perimeter3d(poly);
		}
	}
	PG_RETURN_FLOAT8(area);
}

//calculate the perimeter of polys (sum of length of all rings)
PG_FUNCTION_INFO_V1(perimeter2d);
Datum perimeter2d(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	POLYGON3D			*poly;
	double			area = 0.0;

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];
		if (type1 == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;
			area += polygon_perimeter2d(poly);
		}
	}
	PG_RETURN_FLOAT8(area);
}


// cn_PnPoly(): crossing number test for a point in a polygon
//      input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      returns: 0 = outside, 1 = inside
//
//	Our polygons have first and last point the same, 
//

int PIP( POINT3D *P, POINT3D *V, int n )
{
    int    cn = 0;    // the crossing number counter
 		int i;

	double vt;

    // loop through all edges of the polygon

    	for (i=0; i< (n-1) ; i++) 
	{    // edge from V[i] to V[i+1]

       	if (((V[i].y <= P->y) && (V[i+1].y > P->y))    // an upward crossing
       		 || ((V[i].y > P->y) && (V[i+1].y <= P->y))) // a downward crossing
	 	{ 

       	   vt = (double)(P->y - V[i].y) / (V[i+1].y - V[i].y);
         	   if (P->x < V[i].x + vt * (V[i+1].x - V[i].x)) 			// P.x <intersect
           	     		++cn;   // a valid crossing of y=P.y right of P.x
      	}	
    }
    return (cn&1);    // 0 if even (out), and 1 if odd (in)

}





//this one is easy - is the point inside a box?
bool point_truely_inside(POINT3D	*point, BOX3D	*box)
{
	return ( 
			(point->x >= box->LLB.x) && (point->x <= box->URT.x)  && 
		      (point->y >= box->LLB.y) && (point->y <= box->URT.y)  
		 );
}


// see if a linestring is inside a box - check to see if each its line
//	segments are inside the box  
// NOTE: all this is happening in 2d - third dimention is ignored
//
//use a modified CohenSutherland line clipping algoritm
//
//Step one - compute outcodes for Pi and Pi+1
//
//
// 			|		|
//		1001	|   1000	|  1010
//			|		|
//      ----------+-----------+------------
// 			| (inside)	|
//		0001	|  		|  0010
//			|   0000	|
//      ----------+-----------+------------
// 			|		|
//		0101	|   0100	|  0110
//			|		|
//
// If Pi or Pi+1 have code 0000 then this line seg is inside the box  (trivial accept)
//  if Pi && Pi+1 != 0 then seg is NOT in the box (trivial reject)
//
//	Otherwise the line seg might traverse through the box.
//		=> see if the seg intersects any of the walls of the box.
//

int	compute_outcode( POINT3D *p, BOX3D *box)
{
	int	Code=0;

//printf("compute outcode:\n");
//print_box(box);
//printf("point = [%g,%g]\n",p->x,p->y);
	if (p->y > box->URT.y )
		Code = 8;
  	else
  	{
	  if (p->y <box->LLB.y )
	  {
		Code = 4;
	  }
  	}
  	if (p->x > box->URT.x )
  	{
	  return (Code+2);
 	}
 	if (p->x <box->LLB.x )
 	{
	  return (Code+1);
 	}

 	return (Code);		
}


bool lineseg_inside_box( POINT3D *P1, POINT3D *P2, BOX3D *box)
{
	int	outcode_p1, outcode_p2;
	double	Ax,Ay, Bx,By, Cx,Cy, Dx,Dy;
	double	r,s;

	outcode_p1 = compute_outcode(P1, box);
	if (outcode_p1 ==0)
		return TRUE;
	
	outcode_p2 = compute_outcode(P2, box);
	if (outcode_p2 ==0)
		return TRUE;
	if ((outcode_p1 & outcode_p2) != 0)
		return FALSE;


	if ((outcode_p1 + outcode_p2) == 12)
		return TRUE; //vertically slices box

	if ((outcode_p1 + outcode_p2) == 3)
		return TRUE; //horizontally slices box


//printf("have to do boundry calcs\n");

	// know that its a diagonal line

	//this is the tough case - it may or maynot intersect the box
	//see if it intersects the horizonal box lines

	// from comp.graphics.algo's faq:
	/*
	       (Ay-Cy)(Dx-Cx)-(Ax-Cx)(Dy-Cy) 
	r =  ------------------------------------
         	  (Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx) 

		(Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay) 
	s = 	-----------------------------------
		(Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx) 

	*/
	// if  0<= r&s <=1 then intersection exists
	
	Ax = P1->x; Ay = P1->y;
	Bx = P2->x; By = P2->y;

	//top hor part of box
	Cx = box->LLB.x; Cy= box->URT.y;
	Dx = box->URT.x; Dy= box->URT.y;

		r= ((Ay-Cy)*(Dx-Cx)-(Ax-Cx)*(Dy-Cy) )/
			((Bx-Ax)*(Dy-Cy)-(By-Ay)*(Dx-Cx) ) ;

		s=  ((Ay-Cy)*(Bx-Ax)-(Ax-Cx)*(By-Ay))/
			((Bx-Ax)*(Dy-Cy)-(By-Ay)*(Dx-Cx) ) ;

	if  ( (r>=0) && (r<=1) && (s>=0) && (s<=1)    )
		return TRUE;

	//bottom hor part of box
	Cx = box->LLB.x; Cy= box->LLB.y;
	Dx = box->URT.x; Dy= box->LLB.y;

		r= ((Ay-Cy)*(Dx-Cx)-(Ax-Cx)*(Dy-Cy) )/
			((Bx-Ax)*(Dy-Cy)-(By-Ay)*(Dx-Cx) ) ;

		s=  ((Ay-Cy)*(Bx-Ax)-(Ax-Cx)*(By-Ay))/
			((Bx-Ax)*(Dy-Cy)-(By-Ay)*(Dx-Cx) ) ;
	if  ( (r>=0) && (r<=1) && (s>=0) && (s<=1)    )
		return TRUE;

	return FALSE;
	
}


//give the work to an easier process
bool	linestring_inside_box(POINT3D *pts, int npoints, BOX3D *box)
{
	POINT3D	*frm,*to;
	int		i;

	if (npoints <2)
		return FALSE;	//must have >1 point to make sense

	frm = &pts[0];

	for (i=1; i< npoints;i++)
	{
		to = &pts[i];
	
		if (lineseg_inside_box( frm, to,  box)) 
			return TRUE;

		frm = to;
	}
	return FALSE;
}


//this ones the most complex
//	1. treat the outer ring as a line and see if its inside
//		+ if it is, then poly is inside box
//	2. The polygon could be completely contained inside the box
//		+ line detector (step 1) would have found this
//	3. The polygon could be completely surrounding the box
//		+ point-in-polygon of one of the box corners
//		+ COMPLEXITY: box could be totally inside a single hole
//			+test all holes like step 1.  Any that pass arent "bad" holes
//			+  each of the bad holes do a point-in-polygon if true =-> totally in hole

bool polygon_truely_inside(POLYGON3D	*poly, BOX3D *box)
{
	bool		outer_ring_inside,outer_ring_surrounds;
	POINT3D	*pts1;
	POINT3D	test_point;
	int32		ring;
	int		point_offset;
	
	pts1 = (POINT3D *) ( (char *)&(poly->npoints[poly->nrings] )  );
	pts1 = (POINT3D *) MAXALIGN(pts1);


	//step 1 (and 2)
	outer_ring_inside = linestring_inside_box(pts1, poly->npoints[0], box);	

	if (outer_ring_inside)
		return TRUE;

	test_point.x = box->LLB.x;
	test_point.y = box->LLB.y;
		//.z unimportant
	
	outer_ring_surrounds = PIP(&test_point, pts1,poly->npoints[0] ); 

	if (!(outer_ring_surrounds))
		return FALSE;	// disjoing

	//printf("polygon - have to check holes\n");

	//step 3

	point_offset = poly->npoints[0];
	for (ring =1; ring< poly->nrings; ring ++)
	{
		if (!( linestring_inside_box(&pts1[point_offset], poly->npoints[ring], box) ))
		{
			//doesnt intersect the box
			// so if one of the corners of the box is inside the hole, it totally
			//   surrounds the box

			if (PIP(&test_point, &pts1[point_offset],poly->npoints[ring] ) )
				return FALSE;
		}
		point_offset += poly->npoints[ring];
	}
	
	return TRUE;
}





//This is more complicated because all the points in the line can be outside the box,
// but one of the edges can cross into the box.
//  We send the work off to another function because its generically usefull for
//   polygons too
bool line_truely_inside( LINE3D *line, BOX3D 	*box)
{
	return	linestring_inside_box(&line->points[0], line->npoints, box);
}

//is anything in geom1 really inside the the bvol (2d only) defined in geom2?
// send each object to its proper handler
PG_FUNCTION_INFO_V1(truly_inside);
Datum truly_inside(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		      *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX3D				*the_bbox;
	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	POLYGON3D			*poly;
	LINE3D			*line;
	POINT3D			*point;

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	the_bbox = &geom2->bvol;

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object 

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];

		if (type1 == POINTTYPE)	//point
		{
			point = (POINT3D *) o1;
			if ( point_truely_inside(point, the_bbox))
				PG_RETURN_BOOL(TRUE);
		}

		if (type1 == LINETYPE)	//line
		{
			line = (LINE3D *) o1;
			if ( line_truely_inside(line, the_bbox))
				PG_RETURN_BOOL(TRUE);
		}

		if (type1 == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;

			if ( polygon_truely_inside(poly, the_bbox))
				PG_RETURN_BOOL(TRUE);
		}
	}
	PG_RETURN_BOOL(FALSE);
}



//number of points in an object
PG_FUNCTION_INFO_V1(npoints);
Datum npoints(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j,i;
	POLYGON3D			*poly;
	LINE3D			*line;
	int32				numb_points = 0;

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object 

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];

		if (type1 == POINTTYPE)	//point
		{
			numb_points ++;
		}

		if (type1 == LINETYPE)	//line
		{
			line = (LINE3D *) o1;
			numb_points += line->npoints;
		}

		if (type1 == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;
			for (i=0; i<poly->nrings;i++)
			{
				numb_points += poly->npoints[i];
			}
		}
	}

	PG_RETURN_INT32(numb_points);
}

//number of rings in an object
PG_FUNCTION_INFO_V1(nrings);
Datum nrings(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	POLYGON3D			*poly;
	int32				numb_rings = 0;

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object 

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];

		if (type1 == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;
			numb_rings += poly->nrings;		
		}
	}

	PG_RETURN_INT32(numb_rings);
}



void	translate_points(POINT3D *pt, int npoints,double x_off, double y_off, double z_off)
{
	int i;

//printf("translating %i points by [%g,%g,%g]\n",npoints,x_off,y_off,z_off);
	if (npoints <1)
		return; //nothing to do

	for (i=0;i<npoints;i++)
	{
//printf("before: [%g,%g,%g]\n", pt[i].x, pt[i].y,pt[i].z);

		pt[i].x += x_off;
		pt[i].y += y_off;
		pt[i].z += z_off;

//printf("after: [%g,%g,%g]\n", pt[i].x, pt[i].y,pt[i].z);
	}
}



//translate geometry
PG_FUNCTION_INFO_V1(translate);
Datum translate(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY			*geom1;
	double			x_off =  PG_GETARG_FLOAT8(1);
	double			y_off =  PG_GETARG_FLOAT8(2);
	double			z_off =  PG_GETARG_FLOAT8(3);
	int32				*offsets1;
	char				*o1;
	int32				type,j,i;
	POLYGON3D			*poly;
	POINT3D			*point,*pts;
	LINE3D			*line;
	int				numb_points;




	//make a copy of geom so we can return a new version

	geom1 = (GEOMETRY *) palloc (geom->size);
	memcpy(geom1, geom, geom->size);			//Will handle SRID and grid


	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object 

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type=  geom1->objType[j];

		if (type == POINTTYPE)	//point
		{
			point = (POINT3D *) o1;
			translate_points(point, 1,x_off,y_off,z_off);
		}

		if (type == LINETYPE)	//line
		{
			line = (LINE3D *) o1;
			translate_points(&(line->points[0]), line->npoints,x_off,y_off,z_off);
		}

		if (type == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;
			//find where the points are and where to put them
			numb_points =0;
			for (i=0; i<poly->nrings;i++)
			{
				numb_points += poly->npoints[i];
			}
			pts = (POINT3D *) ( (char *)&(poly->npoints[poly->nrings] )  );
			pts = (POINT3D *) MAXALIGN(pts);
			translate_points(pts, numb_points,x_off,y_off,z_off);


		}
	}
	//translate the bounding box as well
	geom1->bvol.LLB.x += x_off;
	geom1->bvol.LLB.y += y_off;
	geom1->bvol.LLB.z += z_off;
	geom1->bvol.URT.x += x_off;
	geom1->bvol.URT.y += y_off;
	geom1->bvol.URT.z += z_off;

	PG_RETURN_POINTER(geom1);
}




// set the is3d flag so the system think the geometry is 2d or 3d

PG_FUNCTION_INFO_V1(force_2d);
Datum force_2d(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY			*result;

	result = (GEOMETRY *) palloc(geom->size);
	memcpy(result,geom, geom->size);
	
	result->is3d = FALSE;
	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(force_3d);
Datum force_3d(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY			*result;

	result = (GEOMETRY *) palloc(geom->size);
	memcpy(result,geom, geom->size);

	result->is3d = TRUE;
	PG_RETURN_POINTER(result);

}


//NULL safe bbox union
// combine_bbox(BOX3D, geometry) => union BOX3D and geometry->bvol
// For finding the extent of a set of rows using the extent() aggregate

PG_FUNCTION_INFO_V1(combine_bbox);
Datum combine_bbox(PG_FUNCTION_ARGS)
{
		 Pointer        	 box3d_ptr = PG_GETARG_POINTER(0);
		 Pointer        	 geom_ptr =  PG_GETARG_POINTER(1);
		 BOX3D			*a,*b;
		 GEOMETRY			*geom;
		BOX3D				*box;

	if  ( (box3d_ptr == NULL) && (geom_ptr == NULL) )
	{
		PG_RETURN_NULL(); // combine_bbox(null,null) => null
	}

	if (box3d_ptr == NULL)
	{
		geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		box = (BOX3D *) palloc(sizeof(BOX3D)); 
		memcpy(box, &(geom->bvol), sizeof(BOX3D) );

		PG_RETURN_POINTER(box);  // combine_bbox(null, geometry) => geometry->bvol
	}

	if (geom_ptr == NULL)
	{
		box = (BOX3D *) palloc(sizeof(BOX3D)); 
		memcpy(box, (char *) PG_GETARG_DATUM(0) , sizeof(BOX3D) );
		

		PG_RETURN_POINTER( box ); // combine_bbox(BOX3D, null) => BOX3D
	}

	//combine_bbox(BOX3D, geometry) => union(BOX3D, geometry->bvol)

	box = (BOX3D *) palloc(sizeof(BOX3D));

	a = (BOX3D *) PG_GETARG_DATUM(0) ;
	b= &(( (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1)) )->bvol);

	box->LLB.x = b->LLB.x;
	box->LLB.y = b->LLB.y;
	box->LLB.z = b->LLB.z;

	box->URT.x = b->URT.x;
	box->URT.y = b->URT.y;
	box->URT.z = b->URT.z;

	if (a->LLB.x < b->LLB.x)
		box->LLB.x = a->LLB.x;
	if (a->LLB.y < b->LLB.y)
		box->LLB.y = a->LLB.y;
	if (a->LLB.z < b->LLB.z)
		box->LLB.z = a->LLB.z;

	if (a->URT.x > b->URT.x)
		box->URT.x = a->URT.x;
	if (a->URT.y > b->URT.y)
		box->URT.y = a->URT.y;
	if (a->URT.z > b->URT.z)
		box->URT.z = a->URT.z;

	PG_RETURN_POINTER(  box  );
}


//return 2 for 2d geometries or 3 for 3d geometries
PG_FUNCTION_INFO_V1(dimension);
Datum dimension(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (geom->is3d)
		PG_RETURN_INT32(3);
	else
		PG_RETURN_INT32(2);
}

//returns a string representation of this geometry's type
PG_FUNCTION_INFO_V1(geometrytype);
Datum geometrytype(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		char				*text_ob = palloc(20+4);
		char				*result = text_ob+4;
		int32				size;
		

	memset(result,0,20);

	if (geom->type == POINTTYPE)
		strcpy(result,"POINT");
	if (geom->type == MULTIPOINTTYPE)
		strcpy(result,"MULTIPOINT");

	if (geom->type == LINETYPE)
		strcpy(result,"LINESTRING");
	if (geom->type == MULTILINETYPE)
		strcpy(result,"MULTILINESTRING");

	if (geom->type == POLYGONTYPE)
		strcpy(result,"POLYGON");
	if (geom->type == MULTIPOLYGONTYPE)
		strcpy(result,"MULTIPOLYGON");

	if (geom->type == COLLECTIONTYPE)
		strcpy(result,"GEOMETRYCOLLECTION");

	if (strlen(result) == 0)
		strcpy(result,"UNKNOWN");

	size = strlen(result) +4 ;

	memcpy(text_ob, &size,4); // size of string
	

	PG_RETURN_POINTER(text_ob);

}


//makes a polygon of a features bvol - 1st point = LL 3rd=UR
// 2d only
//
// create new geometry of type polygon, 1 ring, 5 points

PG_FUNCTION_INFO_V1(envelope);
Datum envelope(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY			*result;
	POLYGON3D			*poly;
	POINT3D			pts[5];	//5 points around box
	int				pts_per_ring[1];
	int				poly_size;
	
	//use LLB's z value (we're going to set is3d to false)

	set_point( &pts[0], geom->bvol.LLB.x , geom->bvol.LLB.y , geom->bvol.LLB.z );
	set_point( &pts[1], geom->bvol.URT.x , geom->bvol.LLB.y , geom->bvol.LLB.z );
	set_point( &pts[2], geom->bvol.URT.x , geom->bvol.URT.y , geom->bvol.LLB.z );
	set_point( &pts[3], geom->bvol.LLB.x , geom->bvol.URT.y , geom->bvol.LLB.z );
	set_point( &pts[4], geom->bvol.LLB.x , geom->bvol.LLB.y , geom->bvol.LLB.z );
	
	pts_per_ring[0] = 5; //ring has 5 points

		//make a polygon
	poly = make_polygon(1, pts_per_ring, pts, 5, &poly_size);

	result = make_oneobj_geometry(poly_size, (char *)poly, POLYGONTYPE, FALSE,geom->SRID, geom->scale, geom->offsetX, geom->offsetY);

	PG_RETURN_POINTER(result);	

}

//X(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its X value.  
//Return NULL if there is no POINT(..) in GEOMETRY
PG_FUNCTION_INFO_V1(x_point);
Datum x_point(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char				*o;
	int				type1,j;
	int32				*offsets1;
	

	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	//now have to do a scan of each object 

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == POINTTYPE)	//point
		{
			PG_RETURN_FLOAT8( ((POINT3D *)o)->x) ;
		}

	}
	PG_RETURN_NULL();	
}

//Y(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its Y value.  
//Return NULL if there is no POINT(..) in GEOMETRY
PG_FUNCTION_INFO_V1(y_point);
Datum y_point(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char				*o;
	int				type1,j;
	int32				*offsets1;
	

	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	//now have to do a scan of each object 

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == POINTTYPE)	//point
		{
			PG_RETURN_FLOAT8( ((POINT3D *)o)->y) ;
		}

	}
	PG_RETURN_NULL();	
}

//Z(GEOMETRY) -- find the first POINT(..) in GEOMETRY, returns its Z value.  
//Return NULL if there is no POINT(..) in GEOMETRY
PG_FUNCTION_INFO_V1(z_point);
Datum z_point(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char				*o;
	int				type1,j;
	int32				*offsets1;
	

	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	//now have to do a scan of each object 

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == POINTTYPE)	//point
		{
			PG_RETURN_FLOAT8( ((POINT3D *)o)->z) ;
		}

	}
	PG_RETURN_NULL();	
}

//numpoints(GEOMETRY) -- find the first linestring in GEOMETRY, return
//the number of points in it.  Return NULL if there is no LINESTRING(..)
//in GEOMETRY
PG_FUNCTION_INFO_V1(numpoints_linestring);
Datum numpoints_linestring(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char				*o;
	int				type1,j;
	int32				*offsets1;
	
	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == LINETYPE)	//linestring
		{
			PG_RETURN_INT32( ((LINE3D *)o)->npoints) ;
		}

	}
	PG_RETURN_NULL();	
}

//pointn(GEOMETRY,INTEGER) -- find the first linestring in GEOMETRY,
//return the point at index INTEGER (0 is 1st point).  Return NULL if
//there is no LINESTRING(..) in GEOMETRY or INTEGER is out of bounds.
// keeps is3d flag

PG_FUNCTION_INFO_V1(pointn_linestring);
Datum pointn_linestring(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				wanted_index =PG_GETARG_INT32(1); 
	char				*o;
	int				type1,j;
	int32				*offsets1;
	LINE3D			*line;

	
	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == LINETYPE)	//linestring
		{
			line = (LINE3D *)o;
			if ( (wanted_index<0) || (wanted_index> (line->npoints-1) ) )
				PG_RETURN_NULL(); //index out of range
			//get the point, make a new geometry
			PG_RETURN_POINTER(
				make_oneobj_geometry(sizeof(POINT3D), 
						         (char *)&line->points[wanted_index],
							   POINTTYPE,  geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY) 
						);
		}

	}
	PG_RETURN_NULL();	
}

// exteriorRing(GEOMETRY) -- find the first polygon in GEOMETRY, return
//its exterior ring (as a linestring).  Return NULL if there is no
//POLYGON(..) in GEOMETRY. 

PG_FUNCTION_INFO_V1(exteriorring_polygon);
Datum exteriorring_polygon(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	char				*o;
	int				type1,j;
	int32				*offsets1;
	LINE3D			*line;
	POLYGON3D			*poly;
	POINT3D			*pts;
	int				size_line;

	
	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == POLYGONTYPE)	//polygon object
		{
			poly = (POLYGON3D *)o;
	  		pts = (POINT3D *) ( (char *)&(poly->npoints[poly->nrings] )  );
			pts = (POINT3D *) MAXALIGN(pts);

			line = make_line(poly->npoints[0], pts, &size_line);

			//get the ring, make a new geometry
			PG_RETURN_POINTER(
				make_oneobj_geometry(size_line,
						         (char *) line,
							   LINETYPE,  geom->is3d,geom->SRID,  geom->scale, geom->offsetX, geom->offsetY) 
						);
		}

	}
	PG_RETURN_NULL();
}

//NumInteriorRings(GEOMETRY) -- find the first polygon in GEOMETRY,
//return the number of interior rings.  Return NULL if there is no
//POLYGON(..) in GEOMETRY.

PG_FUNCTION_INFO_V1(numinteriorrings_polygon);
Datum numinteriorrings_polygon(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	char				*o;
	int				type1,j;
	int32				*offsets1;
	POLYGON3D			*poly;


	
	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == POLYGONTYPE)	//polygon object
		{
			poly = (POLYGON3D *)o;
	  		PG_RETURN_INT32( poly->nrings -1 ) ;
		}
	}
	PG_RETURN_NULL();
}

//        InteriorRingN(GEOMETRY,INTEGER) --  find the first polygon in GEOMETRY,
//return the interior ring at index INTEGER (as a linestring).  Return
//NULL if there is no POLYGON(..) in GEOMETRY or INTEGER is out of bounds.
// 1st ring = exerior ring

PG_FUNCTION_INFO_V1(interiorringn_polygon);
Datum interiorringn_polygon(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				wanted_index =PG_GETARG_INT32(1); 
	char				*o;
	int				type1,j,t,point_offset;
	int32				*offsets1;
	POLYGON3D			*poly;
	LINE3D			*line;
	POINT3D			*pts;
	int				size_line;

	
	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == POLYGONTYPE)	//polygon object
		{
			poly = (POLYGON3D *)o;

			if ( (wanted_index<0) || (wanted_index> (poly->nrings-2) ) )
				PG_RETURN_NULL(); //index out of range

			
	  		pts = (POINT3D *) ( (char *)&(poly->npoints[poly->nrings] )  );
			pts = (POINT3D *) MAXALIGN(pts);

			//find the 1st point in wanted ring
			point_offset=0;
			for (t=0; t< (wanted_index+1); t++)
			{
				point_offset += poly->npoints[t];	
			}

			line = make_line(poly->npoints[wanted_index+1], &pts[point_offset], &size_line);

			//get the ring, make a new geometry
			PG_RETURN_POINTER(
				make_oneobj_geometry(size_line,
						         (char *) line,
							   LINETYPE,  geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY) 
						);	  		
		}
	}
	PG_RETURN_NULL();
}


//        numgeometries(GEOMETRY) -- if GEOMETRY is a GEOMETRYCOLLECTION, return
//the number of geometries in it, otherwise return NULL.
//		if GEOMETRY is a MULTI* type, return the number of sub-types in it.

PG_FUNCTION_INFO_V1(numgeometries_collection);
Datum numgeometries_collection(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		if  ( (geom->type == COLLECTIONTYPE) || 
		      (geom->type == MULTIPOINTTYPE) || 
			(geom->type == MULTILINETYPE) || 
			(geom->type == MULTIPOLYGONTYPE)
		    )
			PG_RETURN_INT32( geom->nobjs ) ;
		else
			PG_RETURN_NULL();	
}


//geometryN(GEOMETRY, INTEGER) -- if GEOMETRY is a GEOMETRYCOLLECTION,
//return the sub-geometry at index INTEGER (0=first geometry), otherwise
//return NULL.   NOTE: MULTIPOINT, MULTILINESTRING,MULTIPOLYGON are
//converted to sets of POINT,LINESTRING, and POLYGON so the index may
//change.
// if GEOMETRY is a MULTI* type, return the Nth sub-geometry


PG_FUNCTION_INFO_V1(geometryn_collection);
Datum geometryn_collection(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int32				wanted_index =PG_GETARG_INT32(1); 
		int				type;
		int32				*offsets1;
		char				*o;



	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

		if  (!(( (geom->type == COLLECTIONTYPE) || 
		      (geom->type == MULTIPOINTTYPE) || 
			(geom->type == MULTILINETYPE) || 
			(geom->type == MULTIPOLYGONTYPE)
		    )))
				PG_RETURN_NULL();			


		if ( (wanted_index <0) || (wanted_index > (geom->nobjs-1) ) )
			PG_RETURN_NULL();	//bad index

		type = geom->objType[wanted_index];
		o = (char *) geom +offsets1[wanted_index] ;  
		
		if (type == POINTTYPE)
		{
				PG_RETURN_POINTER(
				make_oneobj_geometry(sizeof(POINT3D),
						         o,
							   POINTTYPE,  geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY) 
						);
		}
		if (type == LINETYPE)
		{
				PG_RETURN_POINTER(
				make_oneobj_geometry(	size_subobject (o, LINETYPE),
						         o,
							   LINETYPE,  geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY) 
						);
		}
		if (type == POLYGONTYPE)
		{
				PG_RETURN_POINTER(
				make_oneobj_geometry(	size_subobject (o, POLYGONTYPE),
						         o,
							   POLYGONTYPE,  geom->is3d, geom->SRID, geom->scale, geom->offsetX, geom->offsetY) 
						);
		}

		PG_RETURN_NULL();	
}


//force the geometry to be a geometrycollection type

PG_FUNCTION_INFO_V1(force_collection);
Datum force_collection(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY			*result;

	result = (GEOMETRY *) palloc(geom->size);
	memcpy(result,geom, geom->size);
	
	result->type = COLLECTIONTYPE;

	PG_RETURN_POINTER(result);
}



// point_inside_circle(geometry, Px, Py, d)
// returns true if there is a point in geometry whose distance to (Px,Py) is < d

PG_FUNCTION_INFO_V1(point_inside_circle);
Datum point_inside_circle(PG_FUNCTION_ARGS)
{

	GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char				*o;
	int				type1,j;
	int32				*offsets1;
	POINT3D			*pt;
	double			Px  = PG_GETARG_FLOAT8(1);
	double			Py =  PG_GETARG_FLOAT8(2);
	double			d =   PG_GETARG_FLOAT8(3);
	double			dd = d*d; //d squared

	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

	//now have to do a scan of each object 

	for (j=0; j< geom->nobjs; j++)		//for each object in geom
	{
		o = (char *) geom +offsets1[j] ;  
		type1=  geom->objType[j];

		if (type1 == POINTTYPE)	//point
		{
			//see if its within d of Px,Py
			pt = (POINT3D *) o;
			if  ( (   (pt->x - Px)*(pt->x - Px) + (pt->y - Py)*(pt->y - Py) ) < dd)
			{
				PG_RETURN_BOOL(TRUE);
			}
		}

	}
	PG_RETURN_BOOL(FALSE);	
}


//distance from p to line A->B
double distance_pt_seg(POINT3D *p, POINT3D *A, POINT3D *B)
{
	double	r,s;


	//if start==end, then use pt distance
	if (  ( A->x == B->x) && (A->y == B->y) )
		return distance_pt_pt(p,A);

	//otherwise, we use comp.graphics.algorithms Frequently Asked Questions method

	/*(1)     	      AC dot AB 
                   r = ---------
                         ||AB||^2 
		r has the following meaning:
		r=0 P = A 
		r=1 P = B 
		r<0 P is on the backward extension of AB 
		r>1 P is on the forward extension of AB 
		0<r<1 P is interior to AB 
	*/

	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	if (r<0)
		return (distance_pt_pt(p,A));
	if (r>1)
		return(distance_pt_pt(p,B));
	

	/*(2)
		     (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay) 
		s = ----------------------------- 
		             	L^2 
		
		Then the distance from C to P = |s|*L. 

	*/

	s = ((A->y-p->y)*(B->x-A->x)- (A->x-p->x)*(B->y-A->y) )/ ((B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );
	
	return abs(s) * sqrt(((B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) ));
}



// find the distance from AB to CD
double distance_seg_seg(POINT3D *A, POINT3D *B, POINT3D *C, POINT3D *D)
{

	double	s_top, s_bot,s;
	double	r_top, r_bot,r;


//printf("seg_seg [%g,%g]->[%g,%g]  by [%g,%g]->[%g,%g]  \n",A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);
		//A and B are the same point

	if (  ( A->x == B->x) && (A->y == B->y) )
		return distance_pt_seg(A,C,D);

		//U and V are the same point

	if (  ( C->x == D->x) && (C->y == D->y) )
		return distance_pt_seg(D,A,B);

	// AB and CD are line segments
	/* from comp.graphics.algo

	Solving the above for r and s yields 
				(Ay-Cy)(Dx-Cx)-(Ax-Cx)(Dy-Cy) 
	           r = ----------------------------- (eqn 1) 
				(Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)

		 	(Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay) 
		s = ----------------------------- (eqn 2) 
			(Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx) 
	Let P be the position vector of the intersection point, then 
		P=A+r(B-A) or 
		Px=Ax+r(Bx-Ax) 
		Py=Ay+r(By-Ay) 
	By examining the values of r & s, you can also determine some other limiting conditions: 
		If 0<=r<=1 & 0<=s<=1, intersection exists 
		r<0 or r>1 or s<0 or s>1 line segments do not intersect 
		If the denominator in eqn 1 is zero, AB & CD are parallel 
		If the numerator in eqn 1 is also zero, AB & CD are collinear. 

	*/
	r_top = (A->y-C->y)*(D->x-C->x) - (A->x-C->x)*(D->y-C->y) ;
	r_bot = (B->x-A->x)*(D->y-C->y) - (B->y-A->y)*(D->x-C->x) ;

	s_top = (A->y-C->y)*(B->x-A->x) - (A->x-C->x)*(B->y-A->y);
	s_bot = (B->x-A->x)*(D->y-C->y) - (B->y-A->y)*(D->x-C->x);

	if  ( (r_bot==0) || (s_bot == 0) )
	{
		return ( 
			min(distance_pt_seg(A,C,D), 
				min(distance_pt_seg(B,C,D), 
					min(distance_pt_seg(C,A,B),
						distance_pt_seg(D,A,B)    ) ) )
			 );
	}
	s = s_top/s_bot;
	r=  r_top/r_bot;

	if ((r<0) || (r>1) || (s<0) || (s>1) )
	{
		//no intersection
		return ( 
			min(distance_pt_seg(A,C,D), 
				min(distance_pt_seg(B,C,D), 
					min(distance_pt_seg(C,A,B),
						distance_pt_seg(D,A,B)    ) ) )
			 );
		
	}
	else 
		return -0; //intersection exists
	
}



//trivial 
double distance_pt_pt(POINT3D *p1, POINT3D *p2)
{
	//print_point_debug(p1);
	//print_point_debug(p2);
	return ( sqrt(  (p2->x - p1->x) * (p2->x - p1->x)  + (p2->y - p1->y) * (p2->y - p1->y)  ) );
}


//search all the segments of line to see which one is closest to p1
double distance_pt_line(POINT3D *p1, LINE3D *l2)
{
	double 	result = 99999999999.9,dist_this;
	bool		result_okay = FALSE; //result is a valid min
	int		t;
	POINT3D	*start,*end;

	start = &(l2->points[0]);

	for (t =1;t<l2->npoints;t++)
	{
		end = &(l2->points[t]);

		dist_this = distance_pt_seg(p1, start,end);
		if (result_okay)
			result= min(result,dist_this);
		else
		{
			result_okay = TRUE;
			result = dist_this;
		}

		start =end;
	}

	return result;
}



// test each segment of l1 against each segment of l2.  Return min
double distance_line_line(LINE3D *l1, LINE3D *l2)
{

	double 	result = 99999999999.9,dist_this;
	bool		result_okay = FALSE; //result is a valid min
	int		t,u;
	POINT3D	*start,*end;
	POINT3D	*start2,*end2;


	start = &(l1->points[0]);

	for (t =1;t<l1->npoints;t++)		//for each segment in L1
	{
		end = &(l1->points[t]);

		start2 = &(l2->points[0]);
		
		for (u=1; u< l2->npoints; u++)	//for each segment in L2
		{
			end2 = &(l2->points[u]);
				
				dist_this = distance_seg_seg(start,end,start2,end2);
//printf("line_line; seg %i * seg %i, dist = %g\n",t,u,dist_this);

				if (result_okay)
					result= min(result,dist_this);
				else
				{
					result_okay = TRUE;
					result = dist_this;
				}
				if (result <=0)
					return 0;	//intersection

			start2 = end2;
		}
		start = end;
	}

	return result;
}


// 1. see if pt in outer boundary.  if no, then treat the outer ring like a line
// 2. if in the boundary, test to see if its in a hole.  if so, then return dist to hole
double distance_pt_poly(POINT3D *p1, POLYGON3D *poly2)
{
	POINT3D	*pts; //pts array for polygon
	double	result;
	LINE3D	*line;
	int		junk,t;
	int		offset;


	  		pts = (POINT3D *) ( (char *)&(poly2->npoints[poly2->nrings] )  );
			pts = (POINT3D *) MAXALIGN(pts);


	if (PIP( p1, pts, poly2->npoints[0] ) )
	{
		//inside the outer ring.  scan though each of the inner rings looking to
		// see if its inside.  If not, distance =0.  Otherwise, distance =
		// pt to ring distance

		offset = poly2->npoints[0]; 	//where ring t starts;
		for (t=1; t<poly2->nrings;t++)	//foreach inner ring
		{
			if (	PIP( p1, &pts[offset], poly2->npoints[t] ) )
			{
					//inside a hole.  Distance = pt -> ring

					line = make_line (poly2->npoints[t] , &pts[offset] , &junk);
					result = distance_pt_line(p1, line);
					pfree(line);
					return result;	
			}
			offset += poly2->npoints[t];
		}	

		return 0; //its inside the polygon
	}
	else
	{
		//outside the outer ring.  Distance = pt -> ring

		line = make_line (poly2->npoints[0] , pts, &junk);
		result = distance_pt_line(p1, line);
		pfree(line);
		return result;
	}

	
}


// brute force.  Test l1 against each ring.  If there's an intersection then return 0 (crosses boundary)
// otherwise, test to see if a point inside outer rings of polygon, but not in inner rings. 
// if so, return 0  (line inside polygon)
// otherwise return min distance to a ring (could be outside polygon or inside a hole)
double distance_line_poly(LINE3D *l1, POLYGON3D *poly2)
{
	double	min_dist=9999999.0,this_dist;
	int		t,junk;
	LINE3D	*line;
	POINT3D	*pts; //pts array for polygon
	int		offset;



	  		pts = (POINT3D *) ( (char *)&(poly2->npoints[poly2->nrings] )  );
			pts = (POINT3D *) MAXALIGN(pts);

	offset =0;
	for (t=0;t<poly2->nrings; t++)
	{
		line = make_line (poly2->npoints[t] , &pts[offset] , &junk);
			this_dist = distance_line_line(l1, line);
		pfree(line);

//printf("line_poly; ring %i dist = %g\n",t,this_dist);
		if (t==0)
			min_dist = this_dist;
		else
			min_dist = min(min_dist,this_dist);

		if (min_dist <=0)
			return 0;		//intersection

		offset += poly2->npoints[t];
	}

	//no intersection, have to check if a point is inside the outer ring

	if (PIP( &l1->points[0], pts, poly2->npoints[0] ) )
	{
		//its in the polygon.  Have to check if its inside a hole

//printf("line_poly; inside outer ring\n");
		offset =poly2->npoints[0] ;
		for (t=1; t<poly2->nrings;t++)		//foreach inner ring
		{
			if (	PIP( &l1->points[0],  &pts[offset], poly2->npoints[t] ) )
			{
				//its inside a hole, then the actual distance is the min ring distance
//printf("line_poly; inside inner ring %i\n",t);				
				return min_dist;
			}
			offset += poly2->npoints[t];
		}	
		// not in hole, there for inside the polygon
		return 0;
	}
	else
	{
		//not in the outside ring, so min distance to a ring is the actual min distance
		return min_dist;
	}
	
}

// true if point is in poly (and not in its holes)
bool point_in_poly(POINT3D *p, POLYGON3D *poly)
{
	int	t;
	POINT3D	*pts1;
	int		offset;
	
		pts1 = (POINT3D *) ( (char *)&(poly->npoints[poly->nrings] )  );
		pts1 = (POINT3D *) MAXALIGN(pts1);

	if (PIP(p, pts1, poly->npoints[0]))
	{
		offset = poly->npoints[0];
		for (t=1;t<poly->nrings;t++)
		{
			if (PIP(p, &pts1[offset], poly->npoints[t]) )
			{
				return FALSE; //inside a hole
			}
			offset += poly->npoints[t];
		}
		return TRUE; //in outer ring, not in holes
	}
	else
		return FALSE; //not in outer ring
}

// brute force.  
// test to see if any rings intersect.  if yes, dist =0
// test to see if one inside the other and if they are inside holes.
// find min distance ring-to-ring
double distance_poly_poly(POLYGON3D *poly1, POLYGON3D *poly2)
{
	//foreach ring in Poly1
	// foreach ring in Poly2
	//   if intersect, return 0

	// if poly1 inside poly2 return 0
	// if poly2 inside poly1 return 0

	// otherwise return closest approach of rings

	int 	t,junk;
	POINT3D	*pts1,*pts2;
	int32		offset1;
	double	min_dist= 99999999999.9, this_dist;
	LINE3D	*line;


		pts1 = (POINT3D *) ( (char *)&(poly1->npoints[poly1->nrings] )  );
		pts1 = (POINT3D *) MAXALIGN(pts1);
		pts2 = (POINT3D *) ( (char *)&(poly2->npoints[poly2->nrings] )  );
		pts2 = (POINT3D *) MAXALIGN(pts2);


			//do this first as its a quick test

		//test if poly1 inside poly2
	if (point_in_poly(pts1, poly2) )
		return 0;

		//test if poly2 inside poly1
	if (point_in_poly(pts2, poly1) )
		return 0;


	offset1 =0;
	for (t=0; t<poly1->nrings; t++)	//for each ring in poly1
	{
		line = make_line (poly1->npoints[t] , &pts1[offset1] , &junk);
			this_dist = distance_line_poly(line, poly2);
		pfree(line);
//printf("poly_poly; ring %i dist = %g\n",t,this_dist);
		if (t==0)
			min_dist = this_dist;
		else
			min_dist = min(min_dist,this_dist);

		if (min_dist <=0)
			return 0;		//intersection


		offset1 += poly1->npoints[t];
	}

	//rings do not intersect

	return min_dist;
}

//minimum distance between objects in geom1 and geom2.  returns null if it doesnt exist (future null-safe version).
PG_FUNCTION_INFO_V1(distance);
Datum distance(PG_FUNCTION_ARGS)
{

	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		      *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	int	g1_i, g2_i;
	double		dist,this_dist = 0;
	bool			dist_set = FALSE; //true once dist makes sense.
	int32			*offsets1,*offsets2;	
	int			type1,type2;
	char			*o1,*o2;


	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	dist = 99999999999999999999.9; //very far


	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;
	offsets2 = (int32 *) ( ((char *) &(geom2->objType[0] ))+ sizeof(int32) * geom2->nobjs ) ;


	for (g1_i=0; g1_i < geom1->nobjs; g1_i++)
	{
		o1 = (char *) geom1 +offsets1[g1_i] ;  
		type1=  geom1->objType[g1_i];
		for (g2_i=0; g2_i < geom2->nobjs; g2_i++)
		{
			o2 = (char *) geom2 +offsets2[g2_i] ;  
			type2=  geom2->objType[g2_i];

			if  ( (type1 == POINTTYPE) && (type2 == POINTTYPE) )
			{
				this_dist = distance_pt_pt( (POINT3D *)o1, (POINT3D *)o2 );
			}
			if  ( (type1 == POINTTYPE) && (type2 == LINETYPE) )
			{
				this_dist = distance_pt_line( (POINT3D *)o1, (LINE3D *)o2 );
			}
			if  ( (type1 == POINTTYPE) && (type2 == POLYGONTYPE) )
			{
				this_dist = distance_pt_poly( (POINT3D *)o1, (POLYGON3D *)o2 );
			}
			if  ( (type1 == LINETYPE) && (type2 == LINETYPE) )
			{
				this_dist = distance_line_line( (LINE3D *)o1, (LINE3D *)o2 );
			}
			if  ( (type1 == LINETYPE) && (type2 == POLYGONTYPE) )
			{
				this_dist = distance_line_poly( (LINE3D *)o1, (POLYGON3D *)o2 );
			}
			if  ( (type1 == POLYGONTYPE) && (type2 == POLYGONTYPE) )
			{
				this_dist = distance_poly_poly( (POLYGON3D *)o1, (POLYGON3D *)o2 );
			}
				//trival versions based on above dist(a,b) = dist(b,a)

			if  ( (type1 == LINETYPE) && (type2 == POINTTYPE) )
			{
				this_dist = distance_pt_line( (POINT3D *)o2, (LINE3D *)o1 );
			}	
			if  ( (type1 == POLYGONTYPE) && (type2 == POINTTYPE) )
			{
				this_dist = distance_pt_poly( (POINT3D *)o2, (POLYGON3D *)o1 );
			}
			if  ( (type1 == POLYGONTYPE) && (type2 == LINETYPE) )
			{
				this_dist = distance_line_poly( (LINE3D *)o2, (POLYGON3D *)o1 );
			}

			if (dist_set)
			{
				dist = min(dist,this_dist);
			}
			else
			{
				dist = this_dist;   //first one through
				dist_set = TRUE;
			}

			if (dist <= 0.0)  
				PG_RETURN_FLOAT8( (double) 0.0); // no need to look for things closer
		}
	}
	if (dist <0)
		dist = 0; //computational error, may get -0.00000000001
	PG_RETURN_FLOAT8(dist);
}


//expand_bbox(bbox3d, d)
// returns a new bbox which is exanded d unit in all directions
PG_FUNCTION_INFO_V1(expand_bbox);
Datum expand_bbox(PG_FUNCTION_ARGS)
{
	BOX3D  *bbox   = (BOX3D *) PG_GETARG_POINTER(0);
	double  d 	   = PG_GETARG_FLOAT8(1);
	BOX3D	 *result = (BOX3D *) palloc(sizeof(BOX3D));


	result->LLB.x = bbox->LLB.x - d;
	result->LLB.y = bbox->LLB.y - d;
	result->LLB.z = bbox->LLB.z - d;


	result->URT.x = bbox->URT.x + d;
	result->URT.y = bbox->URT.y + d;
	result->URT.z = bbox->URT.z + d;
	

	PG_RETURN_POINTER(result);
}

//startpoint(geometry) :- if geometry is a linestring, return the first
//point.  Otherwise, return NULL.

PG_FUNCTION_INFO_V1(startpoint);
Datum startpoint(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		POINT3D			*pt;
		LINE3D			*line;
		int32				*offsets1;

	if (geom1->type != LINETYPE)
		PG_RETURN_NULL();

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;


	line = (LINE3D *) ( (char *) geom1 +offsets1[0]);

	pt = &line->points[0];  
	
	PG_RETURN_POINTER(
				make_oneobj_geometry(sizeof(POINT3D),
						         (char *) pt,
							   POINTTYPE,  geom1->is3d, geom1->SRID, geom1->scale, geom1->offsetX, geom1->offsetY) 
				);
}

//endpoint(geometry) :- if geometry is a linestring, return the last
//point.  Otherwise, return NULL.

PG_FUNCTION_INFO_V1(endpoint);
Datum endpoint(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		POINT3D			*pt;
		LINE3D			*line;
		int32				*offsets1;



	if (geom1->type != LINETYPE)
		PG_RETURN_NULL();

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;


	line = (LINE3D *) ( (char *) geom1 +offsets1[0]);


	pt = &line->points[line->npoints-1];  
	
	PG_RETURN_POINTER(
				make_oneobj_geometry(sizeof(POINT3D),
						         (char *) pt,
							   POINTTYPE,  geom1->is3d, geom1->SRID, geom1->scale, geom1->offsetX, geom1->offsetY) 
				);
}



//isclosed(geometry) :- if geometry is a linestring then returns 
//startpoint == endpoint.  If its not a linestring then return NULL.  If
//its a multilinestring, return true only if all the sub-linestrings have
//startpoint=endpoint.
//calculations always done in 3d (even if you do a force2d)


PG_FUNCTION_INFO_V1(isclosed);
Datum isclosed(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		POINT3D			*pt1,*pt2;
		LINE3D			*line;
		int32				*offsets1;
		int				t;

	if  (!((geom1->type == LINETYPE) || (geom1->type == MULTILINETYPE) ))
		PG_RETURN_NULL();

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;


	for (t=0; t< geom1->nobjs; t++)
	{
		line = (LINE3D *) ( (char *) geom1 +offsets1[t]);
		pt1 = &line->points[0];  
		pt2 = &line->points[line->npoints-1];  
		
		if ( (pt1->x != pt2->x) || (pt1->y != pt2->y) || (pt1->z != pt2->z) )
			PG_RETURN_BOOL(FALSE);	
	}
	PG_RETURN_BOOL(TRUE);
}

PG_FUNCTION_INFO_V1(centroid);
Datum centroid(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int32				*offsets1;
		POINT3D			*pts,*cent;
		POLYGON3D			*poly;
		int				t,v;
		int				num_points,num_points_tot;
		double			tot_x,tot_y,tot_z;
		GEOMETRY			*result;
		


	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	if  (!((geom1->type == POLYGONTYPE) || (geom1->type == MULTIPOLYGONTYPE) ))
		PG_RETURN_NULL();

	//find the centroid 
	num_points_tot = 0;
		tot_x = 0; tot_y =0; tot_z=0;
	
	for (t=0;t<geom1->nobjs;t++)
	{
		poly = (POLYGON3D *) ( (char *) geom1 +offsets1[t]);	
		pts = (POINT3D *) ( (char *)&(poly->npoints[poly->nrings] )  );
		pts = (POINT3D *) MAXALIGN(pts);
		num_points =poly->npoints[0];

		// just do the outer ring
	//	for (u=0;u<poly->nrings;u++)
	//	{
	//		num_points += poly->npoints[u];
	//	}

		num_points_tot += num_points-1; //last point = 1st point
		for (v=0;v<num_points-1;v++)
		{
			tot_x  += pts[v].x;	
			tot_y  += pts[v].y;
			tot_z  += pts[v].z;		
		}
	}
	cent = palloc(sizeof(POINT3D));
	set_point(cent, tot_x/num_points_tot, tot_y/num_points_tot,tot_z/num_points_tot);
	 result = (
				make_oneobj_geometry(sizeof(POINT3D),
						         (char *) cent,
							   POINTTYPE,  geom1->is3d, geom1->SRID, geom1->scale, geom1->offsetX, geom1->offsetY) 
				);
	pfree(cent);
	PG_RETURN_POINTER(result);

}

// max_distance(geom,geom)  (both geoms must be linestrings)
//find max distance between l1 and l2
// method: for each point in l1, find distance to l2.  
//		return max distance
//
// note: max_distance(l1,l2) != max_distance(l2,l1) 
// returns double

PG_FUNCTION_INFO_V1(max_distance);
Datum max_distance(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY		      *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		LINE3D			*l1,*l2;

		int32			*offsets1,*offsets2;
		
		int			t;
		POINT3D		*pt;	

		double		result,dist;

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	// only works with lines
	if  (  (geom1->type != LINETYPE) || (geom2->type != LINETYPE) )
		PG_RETURN_NULL();

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;
	offsets2 = (int32 *) ( ((char *) &(geom2->objType[0] ))+ sizeof(int32) * geom2->nobjs ) ;

	l1 =  (LINE3D *) ( (char *) geom1 +offsets1[0]  );
	l2 =  (LINE3D *) ( (char *) geom2 +offsets2[0]  ) ;

	result = -9999; // all distances should be positive

	for(t=0;t<l1->npoints;t++) //for each point in l1
	{
		pt = &l1->points[t];
		dist =  distance_pt_line(pt, l2);

		if (dist > result)
		{
			result = dist;
		}
	}

	if (result <0.0)
		result = 0;

	PG_RETURN_FLOAT8(result);
}

// optimistic_overlap(Polygon P1, Multipolygon MP2, double dist)
// returns true if P1 overlaps MP2
//   method: bbox check - is separation < dist?  no - return false (quick)
//                                               yes  - return distance(P1,MP2) < dist

PG_FUNCTION_INFO_V1(optimistic_overlap);
Datum optimistic_overlap(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY		      *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
		double			dist = PG_GETARG_FLOAT8(2);
		BOX3D				g1_bvol;
		double			calc_dist;

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"optimistic_overlap:Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	if (geom1->type != POLYGONTYPE)
	{
		elog(ERROR,"optimistic_overlap: first arg isnt a polygon\n");
		PG_RETURN_NULL();
	}

	if ( (geom2->type != POLYGONTYPE) &&  (geom2->type != MULTIPOLYGONTYPE) )
	{
		elog(ERROR,"optimistic_overlap: 2nd arg isnt a [multi-]polygon\n");
		PG_RETURN_NULL();
	}

	//bbox check

	memcpy(&g1_bvol, &geom1->bvol, sizeof(BOX3D) );
	
	g1_bvol.LLB.x = g1_bvol.LLB.x - dist;
	g1_bvol.LLB.y = g1_bvol.LLB.y - dist;
	
	g1_bvol.URT.x = g1_bvol.URT.x + dist;
	g1_bvol.URT.y = g1_bvol.URT.y + dist;
	
	//xmin = LLB.x, xmax = URT.x
	

	if (  (g1_bvol.LLB.x > geom2->bvol.URT.x) ||
		(g1_bvol.URT.x < geom2->bvol.LLB.x) ||
		(g1_bvol.LLB.y > geom2->bvol.URT.y) ||
		(g1_bvol.URT.y < geom2->bvol.LLB.y)
	   )
	{
		PG_RETURN_BOOL(FALSE);	//bbox not overlap
	}

	//compute distances
		//should be a fast calc if they actually do intersect
	calc_dist =	DatumGetFloat8 (DirectFunctionCall2(distance,	PointerGetDatum( geom1 ), 	PointerGetDatum( geom2 )));

	PG_RETURN_BOOL(calc_dist < dist);
}

//returns a list of points for a single polygon
// foreach segment
//  (1st and last points will be unaltered, but
//   there will be more points inbetween if segment length is 
POINT3D	*segmentize_ring(POINT3D	*points, double dist, int num_points_in, int *num_points_out)
{
	double	seg_distance;
	int	max_points, offset_new,offset_old;
	POINT3D	*result,*r;
	bool	keep_going;
	POINT3D	*last_point, *next_point;
	

		//initial storage
	max_points = 1000;	
	offset_new=0; //start at beginning of points list
	result = (POINT3D *) palloc (sizeof(POINT3D) * max_points);

	memcpy(result, points, sizeof(POINT3D) ); //1st point
	offset_new++;

	last_point = points;
	offset_old = 1;

	keep_going = 1;
	while(keep_going)
	{
		next_point = &points[offset_old];				

			//distance last->next > dist
		seg_distance = sqrt(
			(next_point->x-last_point->x)*(next_point->x-last_point->x) + 
			(next_point->y-last_point->y)*(next_point->y-last_point->y) ); 
		if (offset_new >= max_points)
		{
			//need to add new points to result
			r = result;
			result = (POINT3D *) palloc (sizeof(POINT3D) * max_points *2);//double size
			memcpy(result,r, sizeof(POINT3D)*max_points);
			max_points *=2;
			pfree(r);
		}
		
		if (seg_distance > dist)
		{
			//add a point at the distance location
			// and set last_point to this loc
			
			result[offset_new].x = last_point->x + (next_point->x-last_point->x)/seg_distance * dist;
			result[offset_new].y = last_point->y + (next_point->y-last_point->y)/seg_distance * dist;
			last_point = &result[offset_new];
			offset_new ++;
		}
		else
		{
			//everything fine, just add the next_point and pop forward
			result[offset_new].x = next_point->x;
			result[offset_new].y = next_point->y;
			offset_new++;
			offset_old++;
			last_point = next_point;
		}
		keep_going = (offset_old < num_points_in);
	}
	*num_points_out = offset_new;
	return result;
}

// select segmentize('POLYGON((0 0, 0 10, 5 5, 0 0))',1);

//segmentize(GEOMETRY P1, double maxlen)
//given a [multi-]polygon, return a new polygon with each segment at most a given length
PG_FUNCTION_INFO_V1(segmentize);
Datum segmentize(PG_FUNCTION_ARGS)
{
		GEOMETRY		*geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY		*result = NULL, *result2;
		double			maxdist = PG_GETARG_FLOAT8(1);
		int32			*offsets1,*p_npoints_ring;
		int				g1_i,r;
		POLYGON3D		*p,*poly;
		POINT3D			*polypts;
		int				num_polypts;
		POINT3D			*all_polypts;
		int				all_num_polypts,all_num_polypts_max;
		POINT3D			*p_points,*rr;
		int				new_size;
		bool			first_one;
		int				poly_size;
		BOX3D			*bbox;

	first_one = 1;


	if ( (geom1->type != POLYGONTYPE) &&  (geom1->type != MULTIPOLYGONTYPE) )
	{
		elog(ERROR,"segmentize: 1st arg isnt a [multi-]polygon\n");
		PG_RETURN_NULL();
	}



	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	for (g1_i=0; g1_i < geom1->nobjs; g1_i++)
	{

		all_num_polypts = 0;
		all_num_polypts_max = 1000;
		all_polypts = (POINT3D *) palloc (sizeof(POINT3D) * all_num_polypts_max );

		p = (POLYGON3D*)((char *) geom1 +offsets1[g1_i]) ;  // the polygon

		p_npoints_ring = (int32 *) palloc(sizeof(int32) * p->nrings);

		p_points = (POINT3D *) ( (char *)&(p->npoints[p->nrings] )  );
		p_points = (POINT3D *) MAXALIGN(p_points);

		for (r=0;r<p->nrings;r++)  //foreach ring in the polygon
		{
			polypts = segmentize_ring(p_points, maxdist, p->npoints[r], &num_polypts);
			if ( (all_num_polypts + num_polypts) < all_num_polypts_max )	
			{
				//just add
				memcpy( &all_polypts[all_num_polypts], polypts, sizeof(POINT3D) *num_polypts );
				all_num_polypts += num_polypts;
			}
			else 
			{
				//need more space
				new_size = all_num_polypts_max + num_polypts + 1000;
				rr = (POINT3D*) palloc(sizeof(POINT3D) * new_size);
				memcpy(rr,all_polypts, sizeof(POINT3D) *all_num_polypts);	
				memcpy(&rr[all_num_polypts], polypts , sizeof(POINT3D) *num_polypts);
				pfree(all_polypts);
				all_polypts = rr;
				all_num_polypts += num_polypts;
			}
			//set points in ring value
			pfree(polypts);
			p_npoints_ring[r] = num_polypts;
		} //for each ring
		
		poly = make_polygon(p->nrings, p_npoints_ring, all_polypts, all_num_polypts, &poly_size);

		if (first_one)
		{
			first_one = 0;
			result = make_oneobj_geometry(poly_size, (char *)poly, POLYGONTYPE, FALSE,geom1->SRID, geom1->scale, geom1->offsetX, geom1->offsetY);
			pfree(poly);
			pfree(all_polypts);
		}
		else
		{
			result2 = add_to_geometry(result,poly_size, (char *) poly, POLYGONTYPE);
			bbox = bbox_of_geometry( result2 ); // make bounding box
			memcpy( &result2->bvol, bbox, sizeof(BOX3D) ); // copy bounding box
			pfree(bbox); // free bounding box
			pfree(result);
			result = result2;
			pfree(poly);
			pfree(all_polypts);
		}	

	} // foreach polygon
	PG_RETURN_POINTER(result);
}


// collector( geom, geom ) returns a geometry which contains
// all the sub_objects from both of the argument geometries

// returned geometry is the simplest possible, based on the types
// of the colelct objects 
// ie. if all are of either X or multiX, then a multiX is returned
// bboxonly types are treated as null geometries (no sub_objects)
PG_FUNCTION_INFO_V1( collector );
Datum collector( PG_FUNCTION_ARGS )
{
	Pointer		geom1_ptr = PG_GETARG_POINTER(0);
	Pointer		geom2_ptr =  PG_GETARG_POINTER(1);
	GEOMETRY	*geom1, *geom2, *temp, *result;
	BOX3D		*bbox;
	int32		i, size, *offsets2;

	// return null if both geoms are null
	if ( (geom1_ptr == NULL) && (geom2_ptr == NULL) )
	{
		PG_RETURN_NULL();
	}

	// return a copy of the second geom if only first geom is null
	if (geom1_ptr == NULL)
	{
		geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
		result = (GEOMETRY *)palloc( geom2->size );
		memcpy( result, geom2, geom2->size );
		PG_RETURN_POINTER(result);
	}

	// return a copy of the first geom if only second geom is null
	if (geom2_ptr == NULL)
	{
		geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		result = (GEOMETRY *)palloc( geom1->size );
		memcpy( result, geom1, geom1->size );
		PG_RETURN_POINTER(result);
	}

	geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	if ( geom1->SRID != geom2->SRID )
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}
	result = (GEOMETRY *)palloc( geom1->size );
	memcpy( result, geom1, geom1->size );
		
	offsets2 = (int32 *)( ((char *)&(geom2->objType[0])) + sizeof( int32 ) * geom2->nobjs ) ;

	for (i=0; i<geom2->nobjs; i++)
	{
		if( i == geom2->nobjs-1 )
		{
			size = geom2->size - offsets2[i];
		}
		else 
		{
			size = offsets2[i+1] - offsets2[i];
		}
		temp = add_to_geometry( result, size, ((char *) geom2 + offsets2[i]), geom2->objType[i] );
		pfree( result );
		result = temp;
	}

	result->is3d = geom1->is3d || geom2->is3d;
	bbox = bbox_of_geometry( result ); // make bounding box
	memcpy( &result->bvol, bbox, sizeof(BOX3D) ); // copy bounding box
	pfree( bbox ); // free bounding box

	PG_RETURN_POINTER( result );
}

PG_FUNCTION_INFO_V1(box3d_xmin);
Datum box3d_xmin(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->LLB.x);
}

PG_FUNCTION_INFO_V1(box3d_ymin);
Datum box3d_ymin(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->LLB.y);
}

PG_FUNCTION_INFO_V1(box3d_zmin);
Datum box3d_zmin(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->LLB.z);
}

PG_FUNCTION_INFO_V1(box3d_xmax);
Datum box3d_xmax(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->URT.x);
}

PG_FUNCTION_INFO_V1(box3d_ymax);
Datum box3d_ymax(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->URT.y);
}

PG_FUNCTION_INFO_V1(box3d_zmax);
Datum box3d_zmax(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);

	PG_RETURN_FLOAT8(box1->URT.z);
}

PG_FUNCTION_INFO_V1(box3dtobox);
Datum box3dtobox(PG_FUNCTION_ARGS)
{
	BOX3D *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX *out;
	out = convert_box3d_to_box(box1);
	PG_RETURN_POINTER(out);
}

