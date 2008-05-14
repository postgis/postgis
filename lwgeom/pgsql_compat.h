#ifndef _PGSQL_COMPAT_H

#if POSTGIS_PGSQL_VERSION < 73
#define WARNING NOTICE
#endif

/* PostgreSQL < 8.3 uses VARATT_SIZEP rather than SET_VARSIZE for varlena types */
#if POSTGIS_PGSQL_VERSION < 83
#define SET_VARSIZE(var, size)   VARATT_SIZEP(var) = size
#endif

#endif /* _PGSQL_COMPAT_H */
