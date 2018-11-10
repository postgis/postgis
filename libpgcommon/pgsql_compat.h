#ifndef _PGSQL_COMPAT_H
#define _PGSQL_COMPAT_H 1

#include "access/tupdesc.h"

/* TupleDescAttr was backported into 9.6.5 but we support any 9.6.X */
#ifndef TupleDescAttr
#define TupleDescAttr(tupdesc, i) ((tupdesc)->attrs[(i)])
#endif

#endif /* _PGSQL_COMPAT_H */
