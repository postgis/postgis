#include <stdlib.h>
#include "postgres.h"
#include "executor/spi.h"

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"

#include "geobuf.pb-c.h"

void *encode_to_geobuf(size_t* len, char *geom_name);
