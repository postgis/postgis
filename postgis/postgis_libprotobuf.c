#include "postgres.h"
#include "utils/builtins.h"
#include "../postgis_config.h"
#include "lwgeom_pg.h"

#ifdef HAVE_LIBPROTOBUF
#include <protobuf-c/protobuf-c.h>
#endif

PG_FUNCTION_INFO_V1(postgis_libprotobuf_version);
Datum postgis_libprotobuf_version(PG_FUNCTION_ARGS)
{
#ifndef HAVE_LIBPROTOBUF
	PG_RETURN_NULL();
#else /* HAVE_LIBPROTOBUF  */
	const char *ver = protobuf_c_version();
	text *result = cstring_to_text(ver);
	PG_RETURN_POINTER(result);
#endif
}