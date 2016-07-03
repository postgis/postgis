#include "postgis_brin.h"

/*
 * As we index geometries but store either a BOX2DF or GIDX according to the
 * operator class, we need to overload the original brin_inclusion_add_value()
 * function to be able to do this. Other original mandatory support functions
 * doesn't need to be overloaded.
 *
 * The previous limitation might be lifted, but we also eliminate some overhead
 * by doing it this way, namely calling different functions through the
 * FunctionCallInvoke machinery for each heap tuple.
 */

static FmgrInfo *inclusion_get_procinfo(BrinDesc *bdesc, uint16 attno,
uint16 procnum);

static bool isempty(Datum the_datum);

PG_FUNCTION_INFO_V1(geom2d_brin_inclusion_add_value);
Datum
geom2d_brin_inclusion_add_value(PG_FUNCTION_ARGS)
{
        BrinDesc   *bdesc = (BrinDesc *) PG_GETARG_POINTER(0);
	BrinValues *column = (BrinValues *) PG_GETARG_POINTER(1);
	Datum      newval = PG_GETARG_DATUM(2);
	bool	   isnull = PG_GETARG_BOOL(3);
        Oid        colloid = PG_GET_COLLATION();
        FmgrInfo   *finfo;
        AttrNumber attno;
	BOX2DF     box_geom, *box_key;

	/*
	 * If the new value is null, we record that we saw it if it's the first
	 * one; otherwise, there's nothing to do.
	 */
	if (isnull)
	{
		if (column->bv_hasnulls)
			PG_RETURN_BOOL(false);

		column->bv_hasnulls = true;
		PG_RETURN_BOOL(true);
	}


        /*
         * If the opclass supports the concept of empty values, test the passed
         * new value for emptiness; if it returns true, we need to set the
         * "contains empty" flag in the element (unless already set).
         */
        if (gserialized_datum_get_box2df_p(newval, &box_geom) == LW_FAILURE) {
             if (isempty(newval)) {
                  attno = column->bv_attno;
  
                  finfo = inclusion_get_procinfo(bdesc, attno, PROCNUM_EMPTY);
                  if (finfo != NULL && DatumGetBool(FunctionCall1Coll(finfo, colloid, newval)))
                  {
                          if (!DatumGetBool(column->bv_values[INCLUSION_CONTAINS_EMPTY]))
                          {
                                 column->bv_values[INCLUSION_CONTAINS_EMPTY] = BoolGetDatum(true);
                                 PG_RETURN_BOOL(true);
                          }

                          PG_RETURN_BOOL(false);
                  }
             } else {
                  /* check other cases where it is not possible to retrieve a box*/
                  elog(ERROR, "Error while extracting the box2df from the geom");
             }
        }

	/* if the recorded value is null, we just need to store the box2df */
	if (column->bv_allnulls)
	{
		column->bv_values[INCLUSION_UNION] = datumCopy((Datum) &box_geom, false,
				sizeof(BOX2DF));
		column->bv_values[INCLUSION_UNMERGEABLE] = BoolGetDatum(false);
		column->bv_values[INCLUSION_CONTAINS_EMPTY] = BoolGetDatum(false);
		column->bv_allnulls = false;
		PG_RETURN_BOOL(true);
	}

	/*
	 * Otherwise, we need to enlarge the stored box2df to make it contains the
	 * current geometry
	 */
	box_key = (BOX2DF *) column->bv_values[INCLUSION_UNION];

	/* enlarge box2df */
	box_key->xmin = Min(box_key->xmin, box_geom.xmin);
	box_key->xmax = Max(box_key->xmax, box_geom.xmax);
	box_key->ymin = Min(box_key->ymin, box_geom.ymin);
	box_key->ymax = Max(box_key->ymax, box_geom.ymax);

	PG_RETURN_BOOL(false);
}

/*
 * Cache and return inclusion opclass support procedure
 *
 * Return the procedure corresponding to the given function support number
 * or null if it is not exists.
 */
static FmgrInfo *
inclusion_get_procinfo(BrinDesc *bdesc, uint16 attno, uint16 procnum)
{
        InclusionOpaque *opaque;
        uint16          basenum = procnum - PROCNUM_BASE;

        /*
         * We cache these in the opaque struct, to avoid repetitive syscache
         * lookups.
         */
        opaque = (InclusionOpaque *) bdesc->bd_info[attno - 1]->oi_opaque;

        /*
         * If we already searched for this proc and didn't find it, don't bother
         * searching again.
         */
        if (opaque->extra_proc_missing[basenum])
                return NULL;

        if (opaque->extra_procinfos[basenum].fn_oid == InvalidOid)
        {
                if (RegProcedureIsValid(index_getprocid(bdesc->bd_index, attno,
                                                                                                procnum)))
                {
                        fmgr_info_copy(&opaque->extra_procinfos[basenum],
                                                   index_getprocinfo(bdesc->bd_index, attno, procnum),
                                                   bdesc->bd_context);
                }
                else
                {
                        opaque->extra_proc_missing[basenum] = true;
                        return NULL;
                }
        }

        return &opaque->extra_procinfos[basenum];
}

static bool
isempty(Datum the_datum)
{
        GSERIALIZED *geom = (GSERIALIZED*)PG_DETOAST_DATUM(the_datum);
        LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
        bool empty = lwgeom_is_empty(lwgeom);

        lwgeom_free(lwgeom);
        return empty;
}
