---
title: "Deprecating and Removing API"
date: 2026-06-26
weight: 30
geekdocHidden: false
---

Several removal types affect user upgrades and must be considered carefully:

* SQL API functions
* C API functions
* SQL-visible types, views, and tables

Functions internal to PostGIS that are never exposed and are only used inside
PostGIS libraries can be changed freely.

## C API Functions

Avoid removing C API functions in minor and patch releases of PostGIS.

If there is a C API function that must be removed, stub it so the signature still
exists but throws an error. Remove the function from its original file and add
the stub to the relevant deprecation file. Ideally, do not do this in a micro
release. It is acceptable in a minor release. In a major release such as PostGIS
4, legacy files could theoretically be emptied and the old functions removed
entirely.

A function can be stubbed in `3.0.0`, but not normally in `3.0.1`. There are edge
cases where a micro release can do this if the SQL signature exposing the
function is fixed carefully, but users often do not run `ALTER EXTENSION` or
`SELECT postgis_extensions_upgrade()` during a micro upgrade. Removing a C symbol
can therefore break production code.

For the `postgis` extension, deprecated functions belong in
`postgis/postgis_legacy.c`. A stub looks like:

```c
POSTGIS_DEPRECATE("2.0.0", postgis_uses_stats)
```

The macro records the version where the function was removed and the function
name.

For other extensions:

* `postgis_sfcgal`: `sfcgal/postgis_sfcgal_legacy.c`
* `postgis_raster`: `raster/rt_pg/rtpg_legacy.c`
* `postgis_topology`: no legacy file exists yet; create
  `topology/postgis_topology_legacy.c` if topology ever needs deprecated C API
  stubs.

The point of replacing a removed function with a throwing stub is `pg_upgrade`.
During `pg_upgrade`, PostgreSQL does not use the normal `CREATE EXTENSION`
routine that loads function definitions from a file. It uses a naked
`CREATE EXTENSION` and then reloads functions, types, and other objects from the
old databases as they existed, still pointing at the same shared library. When
loading those objects, PostgreSQL validates that the referenced functions exist
in the library. If the functions are missing, `pg_upgrade` fails. If a function
exists and only throws an error, `pg_upgrade` can continue.

Old signatures are used because database objects reference functions by object
identifier, not only by name. Recreating a function from scratch gives it a new
OID even if the definition text is identical. Views, tables, and other internal
references would then be broken.

Legacy C signatures exist to keep `pg_upgrade` working.
