
/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 * $Log$
 * Revision 1.11  2004/04/28 22:26:02  pramsey
 * Fixed spelling mistake in header text.
 *
 * Revision 1.10  2004/04/08 17:05:20  dblasby
 * Somehow the memory leak changes I made got removed - I've re-added them.
 *
 * Revision 1.9  2004/04/08 17:00:27  dblasby
 * Changed ggeometry_consistent to be aware of NULL queries.  Ie.
 * select * from <table> where the_geom &&  null::geometry;
 *
 * This tends to happen when you're joining two tables using && and the table
 * has NULLs in it.
 *
 * Revision 1.8  2004/03/05 18:16:47  strk
 * Applied Mark Cave-Ayland patch
 *
 * Revision 1.7  2004/02/25 13:17:31  strk
 * RTContainedBy and RTOverlap strategries implemented locally with a pgbox_overlap function
 *
 * Revision 1.6  2003/08/08 18:19:20  dblasby
 * Conformance changes.
 * Removed junk from postgis_debug.c and added the first run of the long
 * transaction locking support.  (this will change - dont use it)
 * conformance tests were corrected
 * some dos cr/lf removed
 * empty geometries i.e. GEOMETRYCOLLECT(EMPTY) added (with indexing support)
 * pointN(<linestring>,1) now returns the first point (used to return 2nd)
 *
 * Revision 1.5  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 *
 **********************************************************************
 *
 * GiST indexing functions for pgsql >= 7.2
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
#include "utils/elog.h"

//Norman Vine found this problem for compiling under cygwin
//  it defines BYTE_ORDER and LITTLE_ENDIAN

#ifdef __CYGWIN__
#include <sys/param.h>       // FOR ENDIAN DEFINES
#endif

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

//#define DEBUG_GIST

//for GIST index
typedef char* (*BINARY_UNION)(char*, char*, int*);
typedef float (*SIZE_BOX)(char*);
typedef Datum (*RDF)(PG_FUNCTION_ARGS);


BOX *convert_box3d_to_box(BOX3D *in);
Datum ggeometry_compress(PG_FUNCTION_ARGS);
Datum ggeometry_consistent(PG_FUNCTION_ARGS);
static bool rtree_internal_consistent(BOX *key, BOX *query,StrategyNumber strategy);
static bool rtree_leaf_consistent(BOX *key, BOX *query,StrategyNumber strategy);
Datum rtree_decompress(PG_FUNCTION_ARGS);
Datum gbox_union(PG_FUNCTION_ARGS);
Datum gbox_picksplit(PG_FUNCTION_ARGS);
Datum gbox_penalty(PG_FUNCTION_ARGS);
Datum gbox_same(PG_FUNCTION_ARGS);
static float size_box(Datum box);
extern void convert_box3d_to_box_p(BOX3D *in,BOX *out);


int debug = 0;

//puts result in pre-allocated "out"
void convert_box3d_to_box_p(BOX3D *in,BOX *out)
{

		out->high.x = in->URT.x;
		out->high.y = in->URT.y;

		out->low.x = in->LLB.x;
		out->low.y = in->LLB.y;
}


BOX	*convert_box3d_to_box(BOX3D *in)
{
		BOX	*out = palloc (sizeof (BOX) );

		out->high.x = in->URT.x;
		out->high.y = in->URT.y;

		out->low.x = in->LLB.x;
		out->low.y = in->LLB.y;

	return out;
}

PG_FUNCTION_INFO_V1(ggeometry_compress);
Datum ggeometry_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry=(GISTENTRY*)PG_GETARG_POINTER(0);
	GISTENTRY *retval;

	if ( entry->leafkey)
	{
		retval = palloc(sizeof(GISTENTRY));
		if ( DatumGetPointer(entry->key) != NULL )
		{

			GEOMETRY *in;
			BOX	*r;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: ggeometry_compress called on geometry\n");
	fflush( stdout );
#endif

			in = (GEOMETRY*)PG_DETOAST_DATUM( entry->key );

			if (in->nobjs ==0)  // this is the EMPTY geometry
			{
				//elog(NOTICE,"found an empty geometry");
				// dont bother adding this to the index
				PG_RETURN_POINTER(entry);
			}
			else
			{
				r = convert_box3d_to_box(&in->bvol);
				if ( in != (GEOMETRY*)DatumGetPointer(entry->key) )
				{
					pfree( in );
				}

				gistentryinit(*retval, PointerGetDatum(r), entry->rel, entry->page,
					entry->offset, sizeof(BOX), FALSE);
			}

		}
		else
		{
			gistentryinit(*retval, (Datum) 0, entry->rel, entry->page,
				entry->offset, 0,FALSE);
		}
	}
	else
	{
		retval = entry;
	}
	PG_RETURN_POINTER(retval);
}

PG_FUNCTION_INFO_V1(ggeometry_consistent);
Datum ggeometry_consistent(PG_FUNCTION_ARGS)
{
    GISTENTRY *entry = (GISTENTRY*) PG_GETARG_POINTER(0);
    GEOMETRY *query ;
    StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);
    BOX		thebox;
	bool result;

	/*
    ** if entry is not leaf, use rtree_internal_consistent,
    ** else use rtree_leaf_consistent
    */

   if (   ( (Pointer *) PG_GETARG_DATUM(1) ) == NULL)
    {
		//elog(NOTICE,"ggeometry_consistent:: got null query!");
		PG_RETURN_BOOL(false); // null query - this is screwy!
	}

	query = (GEOMETRY*) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));



#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: ggeometry_consistent called\n");
	fflush( stdout );
#endif

    if ( ! (DatumGetPointer(entry->key) != NULL && query) )
		PG_RETURN_BOOL(FALSE);

	convert_box3d_to_box_p( &(query->bvol) , &thebox);

	if (GIST_LEAF(entry))
		result = rtree_leaf_consistent((BOX *) DatumGetPointer(entry->key), &thebox, strategy );
	else
		result = rtree_internal_consistent((BOX *) DatumGetPointer(entry->key), &thebox, strategy );

	PG_FREE_IF_COPY(query, 1);
	PG_RETURN_BOOL(result);
}


static bool
rtree_internal_consistent(BOX *key,
			BOX *query,
			StrategyNumber strategy)
{
    bool retval;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: rtree_internal_consist called\n");
	fflush( stdout );
#endif

    switch(strategy) {
    case RTLeftStrategyNumber:
    case RTOverLeftStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_overleft, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTOverlapStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_overlap, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTOverRightStrategyNumber:
    case RTRightStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_overright, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTSameStrategyNumber:
    case RTContainsStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_contain, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    case RTContainedByStrategyNumber:
      retval = DatumGetBool( DirectFunctionCall2( box_overlap, PointerGetDatum(key), PointerGetDatum(query) ) );
      break;
    default:
      retval = FALSE;
    }
    return(retval);
}


static bool
rtree_leaf_consistent(BOX *key,
			BOX *query,
			StrategyNumber strategy)
{
    bool retval;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: rtree_leaf_consist called\n");
	fflush( stdout );
#endif

	switch (strategy)
	{
		case RTLeftStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box_left, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTOverLeftStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box_overleft, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTOverlapStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box_overlap, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTOverRightStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box_overright, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTRightStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box_right, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTSameStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box_same, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTContainsStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box_contain, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		case RTContainedByStrategyNumber:
			retval = DatumGetBool(DirectFunctionCall2(box_contained, PointerGetDatum(key), PointerGetDatum(query)));
			break;
		default:
			retval = FALSE;
	}
	return (retval);
}


PG_FUNCTION_INFO_V1(rtree_decompress);
Datum rtree_decompress(PG_FUNCTION_ARGS)
{
    PG_RETURN_POINTER(PG_GETARG_POINTER(0));
}

/*
** The GiST Union method for boxes
** returns the minimal bounding box that encloses all the entries in entryvec
*/
PG_FUNCTION_INFO_V1(gbox_union);
Datum gbox_union(PG_FUNCTION_ARGS)
{
	bytea	   *entryvec = (bytea *) PG_GETARG_POINTER(0);
	int		   *sizep = (int *) PG_GETARG_POINTER(1);
	int			numranges,
				i;
	BOX		   *cur,
			   *pageunion;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: gbox_union called\n");
	fflush( stdout );
#endif

	numranges = (VARSIZE(entryvec) - VARHDRSZ) / sizeof(GISTENTRY);
	pageunion = (BOX *) palloc(sizeof(BOX));
	cur = DatumGetBoxP(((GISTENTRY *) VARDATA(entryvec))[0].key);
	memcpy((void *) pageunion, (void *) cur, sizeof(BOX));

	for (i = 1; i < numranges; i++)
	{
		cur = DatumGetBoxP(((GISTENTRY *) VARDATA(entryvec))[i].key);
		if (pageunion->high.x < cur->high.x)
			pageunion->high.x = cur->high.x;
		if (pageunion->low.x > cur->low.x)
			pageunion->low.x = cur->low.x;
		if (pageunion->high.y < cur->high.y)
			pageunion->high.y = cur->high.y;
		if (pageunion->low.y > cur->low.y)
			pageunion->low.y = cur->low.y;
	}
	*sizep = sizeof(BOX);

	PG_RETURN_POINTER(pageunion);
}

/*
** The GiST Penalty method for boxes
** As in the R-tree paper, we use change in area as our penalty metric
*/
PG_FUNCTION_INFO_V1(gbox_penalty);
Datum gbox_penalty(PG_FUNCTION_ARGS)
{
	GISTENTRY  *origentry = (GISTENTRY *) PG_GETARG_POINTER(0);
	GISTENTRY  *newentry = (GISTENTRY *) PG_GETARG_POINTER(1);
	float	   *result = (float *) PG_GETARG_POINTER(2);
	Datum		ud;
	float		tmp1;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: gbox_penalty called\n");
	fflush( stdout );
#endif

	ud = DirectFunctionCall2(rt_box_union, origentry->key, newentry->key);
	tmp1 = size_box(ud);
	if (DatumGetPointer(ud) != NULL)
		pfree(DatumGetPointer(ud));

	*result = tmp1 - size_box(origentry->key);
	PG_RETURN_POINTER(result);
}

typedef struct {
	BOX 	*key;
	int 	pos;
} KBsort;

static int
compare_KB(const void* a, const void* b) {
	BOX *abox = ((KBsort*)a)->key;
	BOX *bbox = ((KBsort*)b)->key;
	float sa = (abox->high.x - abox->low.x) * (abox->high.y - abox->low.y);
	float sb = (bbox->high.x - bbox->low.x) * (bbox->high.y - bbox->low.y);

	if ( sa==sb ) return 0;
	return ( sa>sb ) ? 1 : -1;
}

/*
** The GiST PickSplit method
** New linear algorithm, see 'New Linear Node Splitting Algorithm for R-tree',
** C.H.Ang and T.C.Tan
*/
PG_FUNCTION_INFO_V1(gbox_picksplit);
Datum
gbox_picksplit(PG_FUNCTION_ARGS)
{
	bytea	   *entryvec = (bytea *) PG_GETARG_POINTER(0);
	GIST_SPLITVEC *v = (GIST_SPLITVEC *) PG_GETARG_POINTER(1);
	OffsetNumber i;
	OffsetNumber *listL,
			   *listR,
			   *listB,
			   *listT;
	BOX		   *unionL,
			   *unionR,
			   *unionB,
			   *unionT;
	int			posL,
				posR,
				posB,
				posT;
	BOX			pageunion;
	BOX		   *cur;
	char		direction = ' ';
	bool		allisequal = true;
	OffsetNumber maxoff;
	int			nbytes;

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: gbox_picksplit called\n");
	fflush( stdout );
#endif

	posL = posR = posB = posT = 0;
	maxoff = ((VARSIZE(entryvec) - VARHDRSZ) / sizeof(GISTENTRY)) - 1;

	cur = DatumGetBoxP(((GISTENTRY *) VARDATA(entryvec))[FirstOffsetNumber].key);
	memcpy((void *) &pageunion, (void *) cur, sizeof(BOX));

	/* find MBR */
	for (i = OffsetNumberNext(FirstOffsetNumber); i <= maxoff; i = OffsetNumberNext(i))
	{
		cur = DatumGetBoxP(((GISTENTRY *) VARDATA(entryvec))[i].key);
		if ( allisequal == true &&  (
				pageunion.high.x != cur->high.x ||
				pageunion.high.y != cur->high.y ||
				pageunion.low.x != cur->low.x ||
				pageunion.low.y != cur->low.y
			) )
			allisequal = false;

		if (pageunion.high.x < cur->high.x)
			pageunion.high.x = cur->high.x;
		if (pageunion.low.x > cur->low.x)
			pageunion.low.x = cur->low.x;
		if (pageunion.high.y < cur->high.y)
			pageunion.high.y = cur->high.y;
		if (pageunion.low.y > cur->low.y)
			pageunion.low.y = cur->low.y;
	}

	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	listL = (OffsetNumber *) palloc(nbytes);
	listR = (OffsetNumber *) palloc(nbytes);
	unionL = (BOX *) palloc(sizeof(BOX));
	unionR = (BOX *) palloc(sizeof(BOX));
	if (allisequal)
	{
		cur = DatumGetBoxP(((GISTENTRY *) VARDATA(entryvec))[OffsetNumberNext(FirstOffsetNumber)].key);
		if (memcmp((void *) cur, (void *) &pageunion, sizeof(BOX)) == 0)
		{
			v->spl_left = listL;
			v->spl_right = listR;
			v->spl_nleft = v->spl_nright = 0;
			memcpy((void *) unionL, (void *) &pageunion, sizeof(BOX));
			memcpy((void *) unionR, (void *) &pageunion, sizeof(BOX));

			for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
			{
				if (i <= (maxoff - FirstOffsetNumber + 1) / 2)
				{
					v->spl_left[v->spl_nleft] = i;
					v->spl_nleft++;
				}
				else
				{
					v->spl_right[v->spl_nright] = i;
					v->spl_nright++;
				}
			}
			v->spl_ldatum = BoxPGetDatum(unionL);
			v->spl_rdatum = BoxPGetDatum(unionR);

			PG_RETURN_POINTER(v);
		}
	}

	listB = (OffsetNumber *) palloc(nbytes);
	listT = (OffsetNumber *) palloc(nbytes);
	unionB = (BOX *) palloc(sizeof(BOX));
	unionT = (BOX *) palloc(sizeof(BOX));

#define ADDLIST( list, unionD, pos, num ) do { \
	if ( pos ) { \
		if ( unionD->high.x < cur->high.x ) unionD->high.x	= cur->high.x; \
		if ( unionD->low.x	> cur->low.x  ) unionD->low.x	= cur->low.x; \
		if ( unionD->high.y < cur->high.y ) unionD->high.y	= cur->high.y; \
		if ( unionD->low.y	> cur->low.y  ) unionD->low.y	= cur->low.y; \
	} else { \
			memcpy( (void*)unionD, (void*) cur, sizeof( BOX ) );  \
	} \
	list[pos] = num; \
	(pos)++; \
} while(0)

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		cur = DatumGetBoxP(((GISTENTRY *) VARDATA(entryvec))[i].key);
		if (cur->low.x - pageunion.low.x < pageunion.high.x - cur->high.x)
			ADDLIST(listL, unionL, posL,i);
		else
			ADDLIST(listR, unionR, posR,i);
		if (cur->low.y - pageunion.low.y < pageunion.high.y - cur->high.y)
			ADDLIST(listB, unionB, posB,i);
		else
			ADDLIST(listT, unionT, posT,i);
	}

	/* bad disposition, sort by ascending and resplit */
	if ( (posR==0 || posL==0) && (posT==0 || posB==0) ) {
		KBsort *arr = (KBsort*)palloc( sizeof(KBsort) * maxoff );
		posL = posR = posB = posT = 0;
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i)) {
			arr[i-1].key = DatumGetBoxP(((GISTENTRY *) VARDATA(entryvec))[i].key);
			arr[i-1].pos = i;
		}
		qsort( arr, maxoff, sizeof(KBsort), compare_KB );
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i)) {
			cur = arr[i-1].key;
			if (cur->low.x - pageunion.low.x < pageunion.high.x - cur->high.x)
				ADDLIST(listL, unionL, posL,arr[i-1].pos);
			else if ( cur->low.x - pageunion.low.x == pageunion.high.x - cur->high.x ) {
				if ( posL>posR )
					ADDLIST(listR, unionR, posR,arr[i-1].pos);
				else
					ADDLIST(listL, unionL, posL,arr[i-1].pos);
			} else
				ADDLIST(listR, unionR, posR,arr[i-1].pos);

			if (cur->low.y - pageunion.low.y < pageunion.high.y - cur->high.y)
				ADDLIST(listB, unionB, posB,arr[i-1].pos);
			else if ( cur->low.y - pageunion.low.y == pageunion.high.y - cur->high.y ) {
				if ( posB>posT )
					ADDLIST(listT, unionT, posT,arr[i-1].pos);
				else
					ADDLIST(listB, unionB, posB,arr[i-1].pos);
			} else
				ADDLIST(listT, unionT, posT,arr[i-1].pos);
		}
		pfree(arr);
	}

	/* which split more optimal? */
	if (Max(posL, posR) < Max(posB, posT))
		direction = 'x';
	else if (Max(posL, posR) > Max(posB, posT))
		direction = 'y';
	else
	{
		Datum		interLR = DirectFunctionCall2(rt_box_inter,
												  BoxPGetDatum(unionL),
												  BoxPGetDatum(unionR));
		Datum		interBT = DirectFunctionCall2(rt_box_inter,
												  BoxPGetDatum(unionB),
												  BoxPGetDatum(unionT));
		float		sizeLR,
					sizeBT;

		sizeLR = size_box(interLR);
		sizeBT = size_box(interBT);

		if (sizeLR < sizeBT)
			direction = 'x';
		else
			direction = 'y';
	}

	if (direction == 'x')
	{
		pfree(unionB);
		pfree(listB);
		pfree(unionT);
		pfree(listT);

		v->spl_left = listL;
		v->spl_right = listR;
		v->spl_nleft = posL;
		v->spl_nright = posR;
		v->spl_ldatum = BoxPGetDatum(unionL);
		v->spl_rdatum = BoxPGetDatum(unionR);
	}
	else
	{
		pfree(unionR);
		pfree(listR);
		pfree(unionL);
		pfree(listL);

		v->spl_left = listB;
		v->spl_right = listT;
		v->spl_nleft = posB;
		v->spl_nright = posT;
		v->spl_ldatum = BoxPGetDatum(unionB);
		v->spl_rdatum = BoxPGetDatum(unionT);
	}

	PG_RETURN_POINTER(v);
}

/*
** Equality method
*/
PG_FUNCTION_INFO_V1(gbox_same);
Datum gbox_same(PG_FUNCTION_ARGS)
{
	BOX		   *b1 = (BOX *) PG_GETARG_POINTER(0);
	BOX		   *b2 = (BOX *) PG_GETARG_POINTER(1);
	bool	   *result = (bool *) PG_GETARG_POINTER(2);

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: gbox_same called\n");
	fflush( stdout );
#endif

	if (b1 && b2)
		*result = DatumGetBool(DirectFunctionCall2(box_same, PointerGetDatum(b1), PointerGetDatum(b2)));
	else
		*result = (b1 == NULL && b2 == NULL) ? TRUE : FALSE;
	PG_RETURN_POINTER(result);
}

static float
size_box(Datum box)
{

#ifdef DEBUG_GIST
	elog(NOTICE,"GIST: size_box called\n");
	fflush( stdout );
#endif

	if (DatumGetPointer(box) != NULL)
	{
		float		size;

		DirectFunctionCall2(rt_box_size,
							box, PointerGetDatum(&size));
		return size;
	}
	else
		return 0.0;
}
