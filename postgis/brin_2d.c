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

PG_FUNCTION_INFO_V1(geom2d_brin_inclusion_add_value);
Datum
geom2d_brin_inclusion_add_value(PG_FUNCTION_ARGS)
{
	BrinValues *column = (BrinValues *) PG_GETARG_POINTER(1);
	Datum newval = PG_GETARG_DATUM(2);
	bool		isnull = PG_GETARG_BOOL(3);
	BOX2DF box_geom, *box_key;

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

	if (gserialized_datum_get_box2df_p(newval, &box_geom) == LW_FAILURE)
		elog(ERROR, "Error while extracting the box2df from the geom");

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
