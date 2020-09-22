#include "postgres.h"
#include "utils/builtins.h"
#include "../postgis_config.h"
#include "lwgeom_pg.h"

#include <protobuf-c/protobuf-c.h>

#ifdef HAVE_WAGYU
#include "lwgeom_wagyu.h"
#endif

PG_FUNCTION_INFO_V1(postgis_libprotobuf_version);
Datum postgis_libprotobuf_version(PG_FUNCTION_ARGS)
{
	static char str[50] = {0};
	text *result;
	snprintf(str, sizeof(str), "%s (Internal)", protobuf_c_version());
	result = cstring_to_text(str);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_wagyu_version);
Datum postgis_wagyu_version(PG_FUNCTION_ARGS)
{
#ifndef HAVE_WAGYU
	PG_RETURN_NULL();
#else /* HAVE_WAGYU  */
	const char *ver = libwagyu_version();
	text *result = cstring_to_text(ver);
	PG_RETURN_POINTER(result);
#endif
}
