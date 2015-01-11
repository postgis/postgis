/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2014 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/
 
 #include "liblwgeom_internal.h"
 #include "lwgeom_log.h"

typedef struct
{
	double area;
	int prev;
	int next;
} areanode;

/**

Structure to hold pointarray and it's arealist
*/
typedef struct
{
	const POINTARRAY *inpts;
	areanode *initial_arealist;
	double *res_arealist;
} EFFECTIVE_AREAS;

/**

Calculate the area of a triangle in 2d
*/
static double triarea2d(const double *P1, const double *P2, const double *P3)
{
	LWDEBUG(2, "Entered  triarea2d");
	double ax,bx,ay,by, area;
	
	ax=P1[0]-P2[0];
	bx=P3[0]-P2[0];
	ay=P1[1]-P2[1];
	by=P3[1]-P2[1];
	
	area= fabs(0.5*(ax*by-ay*bx));
	
	LWDEBUGF(2, "Calculated area is %f",(float) area);
	return area;
}

/**

Calculate the area of a triangle in 3d space
*/
static double triarea3d(const double *P1, const double *P2, const double *P3)
{
	LWDEBUG(2, "Entered  triarea3d");
	double ax,bx,ay,by,az,bz,cx,cy,cz, area;
	
	ax=P1[0]-P2[0];
	bx=P3[0]-P2[0];
	ay=P1[1]-P2[1];
	by=P3[1]-P2[1];
	az=P1[2]-P2[2];
	bz=P3[2]-P2[2];	
	
	cx = ay*bz - az*by;
	cy = az*bx - ax*bz;
	cz = ax*by - ay*bx;

	area = fabs(0.5*(sqrt(cx*cx+cy*cy+cz*cz)));	
	
	LWDEBUGF(2, "Calculated area is %f",(float) area);
	return area;
}

/**

To get the effective area, we have to check what area a point results in when all smaller areas are eliminated
*/
static void tune_areas(EFFECTIVE_AREAS ea)
{
	LWDEBUG(2, "Entered  tune_areas");
	const double *P1;
	const double *P2;
	const double *P3;
	double area;
	
	int npoints=ea.inpts->npoints;
	int i;
	int current, prevcurrent, nextcurrent;
	int is3d = FLAGS_GET_Z(ea.inpts->flags);
		
	do
	{
		/*i is our cursor*/
		i=0;
		
		/*current is the point that currently gives the smallest area*/
		current=0;
		
		do
		{			
			i=ea.initial_arealist[i].next;
			if(ea.initial_arealist[i].area<ea.initial_arealist[current].area)
			{
				current=i;
			}		
		}while(i<npoints-2);
		/*We have found the smallest area. That is the resulting effective area for the "current" point*/
		ea.res_arealist[current]=ea.initial_arealist[current].area;
		
		LWDEBUGF(2, "Smallest area was to point %d",current);		
		
		/*The found smallest area point is now viewwed as elimnated and we have to recalculate the area the adjacent (ignoring earlier elimnated points) points gives*/
		prevcurrent=ea.initial_arealist[current].prev;
		nextcurrent=ea.initial_arealist[current].next;
		
		P2= (double*)getPoint_internal(ea.inpts, prevcurrent);
		P3= (double*)getPoint_internal(ea.inpts, nextcurrent);
		
		if(prevcurrent>0)
		{		
		
			LWDEBUGF(2, "calculate previous, %d",prevcurrent);				
			P1= (double*)getPoint_internal(ea.inpts, ea.initial_arealist[prevcurrent].prev);
			if(is3d)
				area=triarea3d(P1, P2, P3);
			else
				area=triarea2d(P1, P2, P3);			
			ea.initial_arealist[prevcurrent].area = FP_MAX(area,ea.res_arealist[current]);
		}
		if(nextcurrent<npoints-1)
		{
			LWDEBUGF(2, "calculate next, %d",nextcurrent);	
			P1=P2;
			P2=P3;	
			
			P3= (double*)getPoint_internal(ea.inpts, ea.initial_arealist[nextcurrent].next);
			if(is3d)
				area=triarea3d(P1, P2, P3);
			else
				area=triarea2d(P1, P2, P3);	
			ea.initial_arealist[nextcurrent].area = FP_MAX(area,ea.res_arealist[current]);
			
			LWDEBUG(2, "Back from area_calc");
		}
		
		/*rearrange the nodes so the eliminated point will be ingored on the next run*/
		ea.initial_arealist[prevcurrent].next = ea.initial_arealist[current].next;
		ea.initial_arealist[nextcurrent].prev = ea.initial_arealist[current].prev;
		
		/*Check if we are finnished*/
		if(prevcurrent==0&&nextcurrent==npoints-1)
			current=0;
	}while(current>0);
	return;	
}


static void ptarray_calc_areas(EFFECTIVE_AREAS ea)
{
	LWDEBUG(2, "Entered  ptarray_set_effective_area2d");
	int i;
	int npoints=ea.inpts->npoints;
	int is3d = FLAGS_GET_Z(ea.inpts->flags);

	const double *P1;
	const double *P2;
	const double *P3;	
		
	P1 = (double*)getPoint_internal(ea.inpts, 0);
	P2 = (double*)getPoint_internal(ea.inpts, 1);
	
	/*The first and last point shall always have the maximum effective area.*/
	ea.initial_arealist[0].area=ea.initial_arealist[npoints-1].area=DBL_MAX;
	ea.res_arealist[0]=ea.res_arealist[npoints-1]=DBL_MAX;
	
	ea.initial_arealist[0].next=1;
	ea.initial_arealist[0].prev=0;
	
	LWDEBUGF(2, "Time to iterate the pointlist with %d points",npoints);
	for (i=1;i<(npoints)-1;i++)
	{
		ea.initial_arealist[i].next=i+1;
		ea.initial_arealist[i].prev=i-1;
			LWDEBUGF(2, "i=%d",i);
		P3 = (double*)getPoint_internal(ea.inpts, i+1);

		if(is3d)
			ea.initial_arealist[i].area=triarea3d(P1, P2, P3);
		else
			ea.initial_arealist[i].area=triarea2d(P1, P2, P3);
		
		LWDEBUGF(2, "area = %f",ea.initial_arealist[i].area);
		P1=P2;
		P2=P3;
		
	}	
		ea.initial_arealist[npoints-1].next=npoints-1;
		ea.initial_arealist[npoints-1].prev=npoints-2;
	LWDEBUG(2, "Time to go on");
	
	for (i=1;i<(npoints)-1;i++)
	{
		ea.res_arealist[i]=-1;
	}
	
	tune_areas(ea);
	return ;
}


static POINTARRAY * ptarray_set_effective_area(POINTARRAY *inpts,double trshld)
{
	LWDEBUG(2, "Entered  ptarray_set_effective_area");
	int p;
	POINT4D pt;
	EFFECTIVE_AREAS ea;
	
	ea.initial_arealist = lwalloc(inpts->npoints*sizeof(areanode));
	LWDEBUGF(2, "sizeof(areanode)=%d",sizeof(areanode));
	ea.res_arealist = lwalloc(inpts->npoints*sizeof(double));
	ea.inpts=inpts;
	POINTARRAY *opts = ptarray_construct_empty(FLAGS_GET_Z(inpts->flags), 1, inpts->npoints);

	ptarray_calc_areas(ea);	
	
	/*Only return points with an effective area above the threashold*/
	for (p=0;p<ea.inpts->npoints;p++)
	{
		if(ea.res_arealist[p]>=trshld)
		{
			pt=getPoint4d(ea.inpts, p);
			pt.m=ea.res_arealist[p];
			ptarray_append_point(opts, &pt, LW_FALSE);
		}
	}	
			
		lwfree(ea.initial_arealist);
		lwfree(ea.res_arealist);
	return opts;
	
}

static LWLINE* lwline_set_effective_area(const LWLINE *iline, double trshld)
{
	LWDEBUG(2, "Entered  lwline_set_effective_area");

	LWLINE *oline = lwline_construct_empty(iline->srid, FLAGS_GET_Z(iline->flags), 1);

	/* Skip empty case */
	if( lwline_is_empty(iline) )
		return lwline_clone(iline);
			
	oline = lwline_construct(iline->srid, NULL, ptarray_set_effective_area(iline->points,trshld));
		
	oline->type = iline->type;
	return oline;
	
}


static LWPOLY* lwpoly_set_effective_area(const LWPOLY *ipoly, double trshld)
{
	LWDEBUG(2, "Entered  lwpoly_set_effective_area");
	int i;
	LWPOLY *opoly = lwpoly_construct_empty(ipoly->srid, FLAGS_GET_Z(ipoly->flags), 1);


	if( lwpoly_is_empty(ipoly) )
		return opoly; /* should we return NULL instead ? */

	for (i = 0; i < ipoly->nrings; i++)
	{
		/* Add ring to simplified polygon */
		if( lwpoly_add_ring(opoly, ptarray_set_effective_area(ipoly->rings[i],trshld)) == LW_FAILURE )
			return NULL;
	}


	opoly->type = ipoly->type;

	if( lwpoly_is_empty(opoly) )
		return NULL;	

	return opoly;
	
}


static LWCOLLECTION* lwcollection_set_effective_area(const LWCOLLECTION *igeom, double trshld)
{
	LWDEBUG(2, "Entered  lwcollection_set_effective_area");	
	int i;
	LWCOLLECTION *out = lwcollection_construct_empty(igeom->type, igeom->srid, FLAGS_GET_Z(igeom->flags), 1);

	if( lwcollection_is_empty(igeom) )
		return out; /* should we return NULL instead ? */

	for( i = 0; i < igeom->ngeoms; i++ )
	{
		LWGEOM *ngeom = lwgeom_set_effective_area(igeom->geoms[i],trshld);
		if ( ngeom ) out = lwcollection_add_lwgeom(out, ngeom);
	}

	return out;
}
 

LWGEOM* lwgeom_set_effective_area(const LWGEOM *igeom, double trshld)
{
	LWDEBUG(2, "Entered  lwgeom_set_effective_area");
	switch (igeom->type)
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		return lwgeom_clone(igeom);
	case LINETYPE:
		return (LWGEOM*)lwline_set_effective_area((LWLINE*)igeom, trshld);
	case POLYGONTYPE:
		return (LWGEOM*)lwpoly_set_effective_area((LWPOLY*)igeom, trshld);
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return (LWGEOM*)lwcollection_set_effective_area((LWCOLLECTION *)igeom, trshld);
	default:
		lwerror("lwgeom_simplify: unsupported geometry type: %s",lwtype_name(igeom->type));
	}
	return NULL;
}
 