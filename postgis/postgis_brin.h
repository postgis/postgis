#include "postgres.h"
#include "fmgr.h"

#include "../postgis_config.h"

/*#define POSTGIS_DEBUG_LEVEL 4*/

#include "liblwgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For debugging macros. */

#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "access/brin_tuple.h"
#include "utils/datum.h"
#include "gserialized_gist.h"

#define INCLUSION_UNION				0
#define INCLUSION_UNMERGEABLE		1
#define INCLUSION_CONTAINS_EMPTY	2

bool is_gserialized_from_datum_empty(Datum the_datum);
