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
	PG_RETURN_FLOAT4(dist);
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
	PG_RETURN_FLOAT4(dist);
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
	PG_RETURN_FLOAT4(area);
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
	PG_RETURN_FLOAT4(area);
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
	PG_RETURN_FLOAT4(area);
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
	memcpy(geom1, geom, geom->size);


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

	result = make_oneobj_geometry(poly_size, (char *)poly, POLYGONTYPE, FALSE);

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
			PG_RETURN_FLOAT4( ((POINT3D *)o)->x) ;
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
			PG_RETURN_FLOAT4( ((POINT3D *)o)->y) ;
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
			PG_RETURN_FLOAT4( ((POINT3D *)o)->z) ;
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
							   POINTTYPE,  geom->is3d) 
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
							   LINETYPE,  geom->is3d) 
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
							   LINETYPE,  geom->is3d) 
						);	  		
		}
	}
	PG_RETURN_NULL();
}


//        numgeometries(GEOMETRY) -- if GEOMETRY is a GEOMETRYCOLLECTION, return
//the number of geometries in it, otherwise return NULL.

PG_FUNCTION_INFO_V1(numgeometries_collection);
Datum numgeometries_collection(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

		if (geom->type == COLLECTIONTYPE)
			PG_RETURN_INT32( geom->nobjs ) ;
		else
			PG_RETURN_NULL();	
}


//geometryN(GEOMETRY, INTEGER) -- if GEOMETRY is a GEOMETRYCOLLECTION,
//return the sub-geometry at index INTEGER (0=first geometry), otherwise
//return NULL.   NOTE: MULTIPOINT, MULTILINESTRING,MULTIPOLYGON are
//converted to sets of POINT,LINESTRING, and POLYGON so the index may
//change.


PG_FUNCTION_INFO_V1(geometryn_collection);
Datum geometryn_collection(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		int32				wanted_index =PG_GETARG_INT32(1); 
		int				type;
		int32				*offsets1;
		char				*o;



	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;

		if (geom->type != COLLECTIONTYPE)
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
							   POINTTYPE,  geom->is3d) 
						);
		}
		if (type == LINETYPE)
		{
				PG_RETURN_POINTER(
				make_oneobj_geometry(	size_subobject (o, LINETYPE),
						         o,
							   LINETYPE,  geom->is3d) 
						);
		}
		if (type == POLYGONTYPE)
		{
				PG_RETURN_POINTER(
				make_oneobj_geometry(	size_subobject (o, POLYGONTYPE),
						         o,
							   POLYGONTYPE,  geom->is3d) 
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