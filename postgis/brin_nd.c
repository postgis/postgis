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

Datum gidx_brin_inclusion_add_value(BrinDesc *bdesc, BrinValues *column, Datum
		newval, bool isnull, int max_dims);

/*
 * As for the GiST case, geographies are converted into GIDX before
 * they are added to the other index keys
 */
PG_FUNCTION_INFO_V1(geog_brin_inclusion_add_value);
Datum
geog_brin_inclusion_add_value(PG_FUNCTION_ARGS)
{
	BrinDesc   *bdesc = (BrinDesc *) PG_GETARG_POINTER(0);
	BrinValues *column = (BrinValues *) PG_GETARG_POINTER(1);
	Datum newval = PG_GETARG_DATUM(2);
	bool            isnull = PG_GETARG_BOOL(3);

	PG_RETURN_DATUM(gidx_brin_inclusion_add_value(bdesc, column, newval, isnull,
					2));
}


PG_FUNCTION_INFO_V1(geom3d_brin_inclusion_add_value);
Datum
geom3d_brin_inclusion_add_value(PG_FUNCTION_ARGS)
{
	BrinDesc   *bdesc = (BrinDesc *) PG_GETARG_POINTER(0);
	BrinValues *column = (BrinValues *) PG_GETARG_POINTER(1);
	Datum newval = PG_GETARG_DATUM(2);
	bool		isnull = PG_GETARG_BOOL(3);

	PG_RETURN_DATUM(gidx_brin_inclusion_add_value(bdesc, column, newval, isnull,
					3));
}

PG_FUNCTION_INFO_V1(geom4d_brin_inclusion_add_value);
Datum
geom4d_brin_inclusion_add_value(PG_FUNCTION_ARGS)
{
	BrinDesc   *bdesc = (BrinDesc *) PG_GETARG_POINTER(0);
	BrinValues *column = (BrinValues *) PG_GETARG_POINTER(1);
	Datum newval = PG_GETARG_DATUM(2);
	bool		isnull = PG_GETARG_BOOL(3);

	PG_RETURN_DATUM(gidx_brin_inclusion_add_value(bdesc, column, newval, isnull,
					4));
}

Datum
gidx_brin_inclusion_add_value(__attribute__((__unused__)) BrinDesc *bdesc,
		BrinValues *column, Datum newval, bool isnull, int max_dims)
{
	char gboxmem[GIDX_MAX_SIZE];
	GIDX *gidx_geom, *gidx_key;
	int dims_geom, dims_key, i;

	Assert(max_dims <= GIDX_MAX_DIM);

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
	 * No need for further processing if the block range is already initialized
	 * and is marked as containing unmergeable values.
	 */
	if (!column->bv_allnulls &&
			DatumGetBool(column->bv_values[INCLUSION_UNMERGEABLE]))
		PG_RETURN_BOOL(false);

	/* create a new GIDX in stack memory, maximum dimensions */
	gidx_geom = (GIDX *) gboxmem;

	/*
	 * check other cases where it is not possible to retrieve a box
	 */
	if (gserialized_datum_get_gidx_p(newval, gidx_geom) == LW_FAILURE)
	{
		/*
		 * Empty entries have to be supported in the opclass: test the passed
		 * new value for emptiness; if it returns true, we need to set the
		 * "contains empty" flag in the element (unless already set).
		 */
		if (is_gserialized_from_datum_empty(newval))
		{
			if (!DatumGetBool(column->bv_values[INCLUSION_CONTAINS_EMPTY]))
			{
				column->bv_values[INCLUSION_CONTAINS_EMPTY] = BoolGetDatum(true);
				PG_RETURN_BOOL(true);
			}

			PG_RETURN_BOOL(false);
		} else
		{
			/*
			 * in case the entry is not empty and it is not possible to
			 * retrieve a box, raise an error
			 */
			elog(ERROR, "Error while extracting the gidx from the geom");
		}
	}

	/* Get the actual dimension of the geometry */
	dims_geom = GIDX_NDIMS(gidx_geom);

	/* if the recorded value is null, we just need to store the GIDX */
	if (column->bv_allnulls)
	{
		/*
		 * It's not safe to summarize geometries of different number of
		 * dimensions in the same range.  We therefore fix the number of
		 * dimension for this range by storing the bounding box of the first
		 * geometry found as is, being careful not to store more dimension than
		 * defined in the opclass.
		 */
		if (dims_geom > max_dims)
		{
			/*
			 * Diminush the varsize to only store the maximum number of
			 * dimensions allowed by the opclass
			 */
			SET_VARSIZE(gidx_geom, VARHDRSZ + max_dims * 2 * sizeof(float));
			dims_geom = max_dims;
		}

		column->bv_values[INCLUSION_UNION] = datumCopy((Datum) gidx_geom, false,
				GIDX_SIZE(dims_geom));
		column->bv_values[INCLUSION_UNMERGEABLE] = BoolGetDatum(false);
		column->bv_values[INCLUSION_CONTAINS_EMPTY] = BoolGetDatum(false);
		column->bv_allnulls = false;
		PG_RETURN_BOOL(true);
	}

	gidx_key = (GIDX *) column->bv_values[INCLUSION_UNION];
	dims_key = GIDX_NDIMS(gidx_key);

	/*
	 * Mark the datum as unmergeable if its number of dimension is not the same
	 * as the one stored in the key of the current range
	 */
	if (dims_key != dims_geom)
	{
		column->bv_values[INCLUSION_UNMERGEABLE] = BoolGetDatum(true);
		PG_RETURN_BOOL(true);
	}

	/* Check if the stored bounding box already contains the geometry's one */
	if (gidx_contains(gidx_key, gidx_geom))
			PG_RETURN_BOOL(false);

	/*
	 * Otherwise, we need to enlarge the stored GIDX to make it contains the
	 * current geometry.  As we store a GIDX with a fixed number of dimensions,
	 * we just need adjust min and max
	 */
	for ( i = 0; i < dims_key; i++ )
	{
		/* Adjust minimums */
		GIDX_SET_MIN(gidx_key, i,
				Min(GIDX_GET_MIN(gidx_key,i),GIDX_GET_MIN(gidx_geom,i)));
		/* Adjust maximums */
		GIDX_SET_MAX(gidx_key, i,
				Max(GIDX_GET_MAX(gidx_key,i),GIDX_GET_MAX(gidx_geom,i)));
	}

	PG_RETURN_BOOL(true);
}
