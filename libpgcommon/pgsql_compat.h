#ifndef _PGSQL_COMPAT_H
#define _PGSQL_COMPAT_H 1

/* Make sure PG_NARGS is defined for older PostgreSQL versions */
#ifndef PG_NARGS
#define PG_NARGS() (fcinfo->nargs)
#endif

/* Define ARR_OVERHEAD_NONULLS for PostgreSQL < 8.2 */
#if POSTGIS_PGSQL_VERSION < 82
#define ARR_OVERHEAD_NONULLS(x) ARR_OVERHEAD((x))
#endif

/* PostgreSQL < 8.3 uses VARATT_SIZEP rather than SET_VARSIZE for varlena types */
#if POSTGIS_PGSQL_VERSION < 83
#define SET_VARSIZE(var, size)   VARATT_SIZEP(var) = size
#endif

#endif /* _PGSQL_COMPAT_H */
