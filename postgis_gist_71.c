
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
 * Revision 1.4  2004/04/28 22:26:02  pramsey
 * Fixed spelling mistake in header text.
 *
 * Revision 1.3  2003/08/08 18:19:20  dblasby
 * Conformance changes.
 * Removed junk from postgis_debug.c and added the first run of the long
 * transaction locking support.  (this will change - dont use it)
 * conformance tests were corrected
 * some dos cr/lf removed
 * empty geometries i.e. GEOMETRYCOLLECT(EMPTY) added (with indexing support)
 * pointN(<linestring>,1) now returns the first point (used to return 2nd)
 *
 * Revision 1.2  2003/07/01 18:30:55  pramsey
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
#include "utils/elog.h"

//Norman Vine found this problem for compiling under cygwin
//  it defines BYTE_ORDER and LITTLE_ENDIAN

#ifdef __CYGWIN__
#include <sys/param.h>       // FOR ENDIAN DEFINES
#endif

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

// #define DEBUG_GIST


//for GIST index
typedef char* (*BINARY_UNION)(char*, char*, int*);
typedef float (*SIZE_BOX)(char*);
typedef Datum (*RDF)(PG_FUNCTION_ARGS);

GISTENTRY *ggeometry_compress(PG_FUNCTION_ARGS);
GEOMETRYKEY *ggeometry_union(PG_FUNCTION_ARGS);
GIST_SPLITVEC * ggeometry_picksplit(PG_FUNCTION_ARGS);
bool ggeometry_consistent(PG_FUNCTION_ARGS);
float * ggeometry_penalty(PG_FUNCTION_ARGS);
bool * ggeometry_same(PG_FUNCTION_ARGS);

char * ggeometry_binary_union(char *r1, char *r2, int *sizep);
float size_geometrykey( char *pk );

Datum ggeometry_inter(PG_FUNCTION_ARGS);

/*
** Common rtree-function (for all ops)
*/
char * rtree_union(bytea *entryvec, int *sizep, BINARY_UNION bu);
float * rtree_penalty(GISTENTRY *origentry, GISTENTRY *newentry, float *result, BINARY_UNION bu, SIZE_BOX sb);
GIST_SPLITVEC * rtree_picksplit(bytea *entryvec, GIST_SPLITVEC *v, int keylen, BINARY_UNION bu, RDF interop, SIZE_BOX sb);
bool rtree_internal_consistent(BOX *key, BOX *query, StrategyNumber strategy);


GISTENTRY *rtree_decompress(PG_FUNCTION_ARGS);


//restriction in the GiST && operator

Datum postgis_gist_sel(PG_FUNCTION_ARGS)
{
        PG_RETURN_FLOAT8(0.000005);
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
GISTENTRY *ggeometry_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry=(GISTENTRY*)PG_GETARG_POINTER(0);
	GISTENTRY *retval;

	if ( entry->leafkey) {
		retval = palloc(sizeof(GISTENTRY));
		if ( entry->pred ) {

			GEOMETRY *in;
			GEOMETRYKEY *r;
			BOX	*thebox;

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_compress called on geometry\n");
#endif

			in = (GEOMETRY*)PG_DETOAST_DATUM(PointerGetDatum(entry->pred));

	if (in->nobjs ==0)  // this is the EMPTY geometry
	{
			//elog(NOTICE,"found an empty geometry");
			// dont bother adding this to the index
			PG_RETURN_POINTER(entry);
	}

			r = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
			r->size = sizeof(GEOMETRYKEY);
			r->SRID = in->SRID;
			thebox = convert_box3d_to_box(&in->bvol);
			memcpy( (void*)&(r->key), (void*)thebox, sizeof(BOX) );
			if ( (char*)in != entry->pred )
			{
				pfree( in );
				pfree(thebox);
			}

			gistentryinit(*retval, (char*)r, entry->rel, entry->page,
				entry->offset, sizeof(GEOMETRYKEY),FALSE);

		} else {
			gistentryinit(*retval, NULL, entry->rel, entry->page,
				entry->offset, 0,FALSE);
		}
	} else {
		retval = entry;
	}
	return( retval );
}

PG_FUNCTION_INFO_V1(ggeometry_consistent);
bool ggeometry_consistent(PG_FUNCTION_ARGS)
{
    GISTENTRY *entry = (GISTENTRY*) PG_GETARG_POINTER(0);
    GEOMETRY *query       = (GEOMETRY*)	    PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
    StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);
    BOX		*thebox;
    /*
    ** if entry is not leaf, use gbox_internal_consistent,
    ** else use gbox_leaf_consistent
    */

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_consistent called\n");
#endif

    if ( ! (entry->pred && query) )
	return FALSE;

	thebox = convert_box3d_to_box( &(query->bvol) );

	if(    ((GEOMETRYKEY *)(entry->pred))->SRID != query->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs (ggeometry_consistent)\n");
		PG_RETURN_BOOL(FALSE);
	}

    PG_RETURN_BOOL(rtree_internal_consistent((BOX*)&( ((GEOMETRYKEY *)(entry->pred))->key ),
		thebox, strategy));
}


PG_FUNCTION_INFO_V1(ggeometry_union);
GEOMETRYKEY *ggeometry_union(PG_FUNCTION_ARGS)
{
	GEOMETRYKEY		*result;

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_union called\n");
#endif

	result = (GEOMETRYKEY*)
		rtree_union(
			(bytea*) PG_GETARG_POINTER(0),
			(int*) PG_GETARG_POINTER(1),
			ggeometry_binary_union
		);

    return result;
}


PG_FUNCTION_INFO_V1(ggeometry_penalty);
float *ggeometry_penalty(PG_FUNCTION_ARGS)
{
#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_penalty called\n");
#endif

    return rtree_penalty(
	(GISTENTRY*) PG_GETARG_POINTER(0),
	(GISTENTRY*) PG_GETARG_POINTER(1),
	(float*) PG_GETARG_POINTER(2),
	ggeometry_binary_union,
	size_geometrykey
    );
}

PG_FUNCTION_INFO_V1(ggeometry_picksplit);
GIST_SPLITVEC *ggeometry_picksplit(PG_FUNCTION_ARGS)
{
#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_picksplit called\n");
#endif


    return rtree_picksplit(
	(bytea*)PG_GETARG_POINTER(0),
	(GIST_SPLITVEC*)PG_GETARG_POINTER(1),
	sizeof(GEOMETRYKEY),
	ggeometry_binary_union,
	ggeometry_inter,
	size_geometrykey
    );
}

PG_FUNCTION_INFO_V1(ggeometry_same);
bool *ggeometry_same(PG_FUNCTION_ARGS)
{

  GEOMETRYKEY *b1 = (GEOMETRYKEY*) PG_GETARG_POINTER(0);
  GEOMETRYKEY *b2 = (GEOMETRYKEY*) PG_GETARG_POINTER(1);

  bool *result = (bool*) PG_GETARG_POINTER(2);

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_same called\n");
#endif


  if ( b1 && b2 )
   	*result = DatumGetBool( DirectFunctionCall2( box_same,
		PointerGetDatum(&(b1->key)),
		PointerGetDatum(&(b2->key))) );
  else
	*result = ( b1==NULL && b2==NULL ) ? TRUE : FALSE;
  return(result);
}

PG_FUNCTION_INFO_V1(ggeometry_inter);
Datum ggeometry_inter(PG_FUNCTION_ARGS) {
  	GEOMETRYKEY *b1 = (GEOMETRYKEY*) PG_GETARG_POINTER(0);
  	GEOMETRYKEY*b2 = (GEOMETRYKEY*) PG_GETARG_POINTER(1);
	char *interd;

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_inter called\n");
#endif


    	interd = DatumGetPointer(DirectFunctionCall2(
			rt_box_inter,
			PointerGetDatum( &(b1->key) ),
			PointerGetDatum( &(b2->key) )) );

	if (interd) {
		GEOMETRYKEY *tmp = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
		tmp->size = sizeof(GEOMETRYKEY);

		memcpy( (void*)&(tmp->key), (void*)interd, sizeof(BOX) );
		tmp->SRID = b1->SRID;
		pfree( interd );
		PG_RETURN_POINTER( tmp );
	} else
		PG_RETURN_POINTER( NULL );
}

char *ggeometry_binary_union(char *r1, char *r2, int *sizep)
{
    GEOMETRYKEY *retval;

#ifdef DEBUG_GIST2
	printf("GIST: ggeometry_binary_union called\n");
#endif

    if ( ! (r1 && r2) ) {
	if ( r1 ) {
		retval = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
		memcpy( (void*)retval, (void*)r1, sizeof(GEOMETRYKEY) );
    		*sizep = sizeof(GEOMETRYKEY);
	} else if ( r2 ) {
		retval = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
		memcpy( (void*)retval, (void*)r2, sizeof(GEOMETRYKEY) );
    		*sizep = sizeof(GEOMETRYKEY);
	} else {
		*sizep = 0;
		retval = NULL;
	}
    } else {
    	BOX *key = (BOX*)DatumGetPointer( DirectFunctionCall2(
		rt_box_union,
		PointerGetDatum( &(((GEOMETRYKEY*)r1)->key) ),
		PointerGetDatum( &(((GEOMETRYKEY*)r2)->key) )) );
	retval = (GEOMETRYKEY*)palloc( sizeof(GEOMETRYKEY) );
	retval->SRID = ((GEOMETRYKEY *) r1)->SRID;
	memcpy( (void*)&(retval->key), (void*)key, sizeof(BOX) );
	pfree( key );
    	*sizep = retval->size = sizeof(GEOMETRYKEY);
    }
    return (char*)retval;
}


float size_geometrykey( char *pk ) {

#ifdef DEBUG_GIST2
	printf("GIST: size_geometrykey called\n");
#endif

    if ( pk ) {
	float size;
    	DirectFunctionCall2( rt_box_size,
		PointerGetDatum( &(((GEOMETRYKEY*)pk)->key) ),
		PointerGetDatum( &size ) );
	return size;
    } else
	return 0.0;
}

char *rtree_union(bytea *entryvec, int *sizep, BINARY_UNION bu)
{
    int numranges, i;
    char *out, *tmp;

#ifdef DEBUG_GIST2
	printf("GIST: rtree_union called\n");
#endif

    numranges = (VARSIZE(entryvec) - VARHDRSZ)/sizeof(GISTENTRY);
    tmp = (char *)(((GISTENTRY *)(VARDATA(entryvec)))[0]).pred;
    out = NULL;

    for (i = 1; i < numranges; i++) {
	out = (*bu)(tmp, (char *)
				 (((GISTENTRY *)(VARDATA(entryvec)))[i]).pred,
				 sizep);
	if (i > 1 && tmp) pfree(tmp);
	tmp = out;
    }

    return(out);
}

float *rtree_penalty(GISTENTRY *origentry, GISTENTRY *newentry, float *result, BINARY_UNION bu, SIZE_BOX sb)
{
    char * ud;
    float tmp1;
    int sizep;

#ifdef DEBUG_GIST2
	printf("GIST: rtree_penalty called\n");
#endif



    ud = (*bu)( origentry->pred, newentry->pred, &sizep );
    tmp1 = (*sb)( ud );
    if (ud) pfree(ud);

    *result = tmp1 - (*sb)( origentry->pred );
    return(result);
}

/*
** The GiST PickSplit method
** We use Guttman's poly time split algorithm
*/
GIST_SPLITVEC *rtree_picksplit(bytea *entryvec, GIST_SPLITVEC *v, int keylen, BINARY_UNION bu, RDF interop, SIZE_BOX sb)
{
    OffsetNumber i, j;
    char *datum_alpha, *datum_beta;
    char *datum_l, *datum_r;
    char *union_d, *union_dl, *union_dr;
    char *inter_d;
    bool firsttime;
    float size_alpha, size_beta, size_union, size_inter;
    float size_waste, waste;
    float size_l, size_r;
    int nbytes;
    int sizep;
    OffsetNumber seed_1 = 0, seed_2 = 0;
    OffsetNumber *left, *right;
    OffsetNumber maxoff;

#ifdef DEBUG_GIST2
	printf("GIST: rtree_picsplit called\n");
#endif

    maxoff = ((VARSIZE(entryvec) - VARHDRSZ)/sizeof(GISTENTRY)) - 2;
    nbytes =  (maxoff + 2) * sizeof(OffsetNumber);
    v->spl_left = (OffsetNumber *) palloc(nbytes);
    v->spl_right = (OffsetNumber *) palloc(nbytes);

    firsttime = true;
    waste = 0.0;

    for (i = FirstOffsetNumber; i < maxoff; i = OffsetNumberNext(i)) {
	datum_alpha = (char *)(((GISTENTRY *)(VARDATA(entryvec)))[i].pred);
	for (j = OffsetNumberNext(i); j <= maxoff; j = OffsetNumberNext(j)) {
	    datum_beta = (char *)(((GISTENTRY *)(VARDATA(entryvec)))[j].pred);

	    /* compute the wasted space by unioning these guys */
	    /* size_waste = size_union - size_inter; */
	    union_d = (*bu)( datum_alpha, datum_beta, &sizep );
	    if ( union_d ) {
		size_union = (*sb)(union_d);
		pfree(union_d);
	    } else
		size_union = 0.0;

	    if ( datum_alpha && datum_beta ) {
    	    	inter_d = DatumGetPointer(DirectFunctionCall2(
			interop,
			PointerGetDatum( datum_alpha ),
			PointerGetDatum( datum_beta )) );
		if ( inter_d ) {
			size_inter = (*sb)(inter_d);
			pfree(inter_d);
		} else
			size_inter = 0.0;
	    } else
		size_inter = 0.0;

	    size_waste = size_union - size_inter;

	    /*
	     *  are these a more promising split that what we've
	     *  already seen?
	     */

	    if (size_waste > waste || firsttime) {
		waste = size_waste;
		seed_1 = i;
		seed_2 = j;
		firsttime = false;
	    }
	}
    }

    left = v->spl_left;
    v->spl_nleft = 0;
    right = v->spl_right;
    v->spl_nright = 0;

    if ( ((GISTENTRY *)(VARDATA(entryvec)))[seed_1].pred ) {
	datum_l = (char*) palloc( keylen );
	memcpy( (void*)datum_l, (void*)(((GISTENTRY *)(VARDATA(entryvec)))[seed_1].pred ), keylen );
    } else
	datum_l = NULL;
    size_l  = (*sb)( datum_l );
    if ( ((GISTENTRY *)(VARDATA(entryvec)))[seed_2].pred ) {
	datum_r = (char*) palloc( keylen );
	memcpy( (void*)datum_r, (void*)(((GISTENTRY *)(VARDATA(entryvec)))[seed_2].pred ), keylen );
    } else
	datum_r = NULL;
    size_r  = (*sb)( datum_r );

    /*
     *  Now split up the regions between the two seeds.  An important
     *  property of this split algorithm is that the split vector v
     *  has the indices of items to be split in order in its left and
     *  right vectors.  We exploit this property by doing a merge in
     *  the code that actually splits the page.
     *
     *  For efficiency, we also place the new index tuple in this loop.
     *  This is handled at the very end, when we have placed all the
     *  existing tuples and i == maxoff + 1.
     */

    maxoff = OffsetNumberNext(maxoff);
    for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i)) {

	/*
	 *  If we've already decided where to place this item, just
	 *  put it on the right list.  Otherwise, we need to figure
	 *  out which page needs the least enlargement in order to
	 *  store the item.
	 */

	if (i == seed_1) {
	    *left++ = i;
	    v->spl_nleft++;
	    continue;
	} else if (i == seed_2) {
	    *right++ = i;
	    v->spl_nright++;
	    continue;
	}

	/* okay, which page needs least enlargement? */
	datum_alpha = (char *)(((GISTENTRY *)(VARDATA(entryvec)))[i].pred);
	union_dl = (*bu)( datum_l, datum_alpha, &sizep );
	union_dr = (*bu)( datum_r, datum_alpha, &sizep );
	size_alpha = (*sb)( union_dl );
	size_beta  = (*sb)( union_dr );

	/* pick which page to add it to */
	if (size_alpha - size_l < size_beta - size_r) {
	    pfree(datum_l);
	    pfree(union_dr);
	    datum_l = union_dl;
	    size_l = size_alpha;
	    *left++ = i;
	    v->spl_nleft++;
	} else {
	    pfree(datum_r);
	    pfree(union_dl);
	    datum_r = union_dr;
	    size_r = size_alpha;
	    *right++ = i;
	    v->spl_nright++;
	}
    }
    *left = *right = FirstOffsetNumber;	/* sentinel value, see dosplit() */

    v->spl_ldatum = datum_l;
    v->spl_rdatum = datum_r;

    return( v );
}


bool rtree_internal_consistent(BOX *key,
			BOX *query,
			StrategyNumber strategy)
{
    bool retval;

#ifdef DEBUG_GIST2
	printf("GIST: rtree_internal_consist called\n");
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
      retval = DatumGetBool( DirectFunctionCall2( box_right, PointerGetDatum(key), PointerGetDatum(query) ) );
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

PG_FUNCTION_INFO_V1(rtree_decompress);
GISTENTRY *rtree_decompress(PG_FUNCTION_ARGS)
{
    return((GISTENTRY*)PG_GETARG_POINTER(0));
}


