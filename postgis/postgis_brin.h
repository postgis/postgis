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
#include "access/genam.h"
#include "access/stratnum.h"
#include "access/brin_tuple.h"
#include "utils/datum.h"
#include "gserialized_gist.h"

#define	INCLUSION_MAX_PROCNUMS	        4	/* maximum support procs we need */
#define	PROCNUM_BASE	               11

#define INCLUSION_UNION			0
#define INCLUSION_UNMERGEABLE		1
#define INCLUSION_CONTAINS_EMPTY	2
#define PROCNUM_EMPTY                  14      /* optional support function to manage empty elements */

typedef struct InclusionOpaque
{
	FmgrInfo	extra_procinfos[INCLUSION_MAX_PROCNUMS];
	bool		extra_proc_missing[INCLUSION_MAX_PROCNUMS];
	Oid		cached_subtype;
	FmgrInfo	strategy_procinfos[RTMaxStrategyNumber];
} InclusionOpaque;
