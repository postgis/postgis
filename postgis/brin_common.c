#include "postgis_brin.h"

bool
is_gserialized_from_datum_empty(Datum the_datum)
{
	GSERIALIZED *geom = (GSERIALIZED*)PG_DETOAST_DATUM(the_datum);

	if (gserialized_is_empty(geom) == LW_TRUE)
		return true;
	else
		return false;
}
