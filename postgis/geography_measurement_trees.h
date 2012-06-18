#include "liblwgeom_internal.h"
#include "lwgeodetic_tree.h"
#include "lwgeom_cache.h"

/*
* Specific tree types include all the generic slots and 
* their own slots for their trees. We put the implementation
* for the CircTreeGeomCache here because we can't shove
* the PgSQL specific bits of the code (fcinfo) back into
* liblwgeom, where most of the circtree logic lives.
*/
typedef struct {
	int                         type;       // <GeomCache>
	GSERIALIZED*                geom1;      // 
	GSERIALIZED*                geom2;      // 
	size_t                      geom1_size; // 
	size_t                      geom2_size; // 
	int32                       argnum;     // </GeomCache>
	CIRC_NODE*                  index;
} CircTreeGeomCache;



CircTreeGeomCache* GetCircTreeGeomCache(FunctionCallInfoData* fcinfo, const GSERIALIZED* g1, const GSERIALIZED* g2);
