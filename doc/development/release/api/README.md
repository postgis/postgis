# API Compatibility

This page covers version terminology, library naming, and the compatibility
rules for SQL API and C API changes.

## Terminology

PostGIS version names use three digits separated by periods:

```text
Major.Minor.Patch
```

| Version | Meaning | Example |
|---------|---------|---------|
| `3.0.0` | Major release (new API surface) | First release of a major version |
| `3.6.0` | Minor release (new features, backwards-compatible) | Added new functions |
| `3.0.1` | Patch release (bug fixes, data changes) | Small fixes to `3.0.0` |

SQL API functions are functions exposed to users in the database. They are
SQL, PL/pgSQL, or C-backed functions.

C API functions are library functions that back SQL API functions.

## Library File Naming

Since PostGIS 3.0 library files are major-versioned. Before PostGIS 3 they were
minor-versioned. In the wild, extension shared-library names vary by operating
system, but common examples are:

* `postgis-2.4.so`, `postgis_topology-2.4.so`, `rtpostgis-2.4.so`
* `postgis-2.5.so`, `postgis_topology-2.5.so`, `rtpostgis-2.5.so`
* `postgis-3.so`, `postgis_raster-3.so`, `postgis_sfcgal-3.so`,
  `postgis_topology-3.so`

PostGIS 3.0 missed some separation work that was fixed in 3.1. Before 3.1,
`postgis_sfcgal` was embedded in `postgis-*.so`, which meant not all PostGIS
libraries had the same set of functions. In 3.1 it was separated into
`postgis-3.so` and `postgis_sfcgal-3.so`.

Also in 3.0.0, raster functionality moved from the `postgis` extension to the
`postgis_raster` extension. The library was always separate, but the old
`rtpostgis` library name did not match the extension name. For upgrade
purposes, old `rtpostgis` is equivalent to current `postgis_raster`.

These changes were made to reduce `pg_upgrade` pain. Developers can still use
minor-versioned libraries for easier testing in the same cluster by configuring
with:

```sh
./configure --with-library-minor-version
```

## Do and Do Not

Do not introduce new SQL API functions in a patch release.

Do not change structure definitions, such as `geometry_columns`,
`geography_columns`, geometry types, or raster types, in a patch release.

Do introduce new SQL API functions in a minor release.

Functions that are not exposed through the SQL API can be introduced any time.

Only major versions can remove SQL API functions or C API functions without
stubbing.

Only the first release of a major version can introduce functionality that
requires `pg_dump` and `pg_restore`.

Do not require newer dependency-library versions in a micro release. You can
require newer dependency-library versions in the first release of a minor
series. For example, PostGIS often drops support for older GEOS and PostgreSQL
versions in a new minor release.

## When Removing Objects Impacts Upgrade

Several removal types affect user upgrades and must be considered carefully:

* SQL API functions
* C API functions
* SQL-visible types, views, and tables

Functions internal to PostGIS that are never exposed and are only used inside
PostGIS libraries can be changed freely.

## Upgrading C API Functions

Avoid removing C API functions in minor and patch releases of PostGIS.

If there is a C API function that must be removed, stub it so the signature
still exists but throws an error. Remove the function from its original file and
add the stub to the relevant deprecation file. Ideally, do not do this in a
micro release. It is acceptable in a minor release. In a major release such as
PostGIS 4, legacy files could theoretically be emptied and the old functions
removed entirely.

A function can be stubbed in `3.0.0`, but not normally in `3.0.1`. There are
edge cases where a micro release can do this if the SQL signature exposing the
function is fixed carefully, but users often do not run `ALTER EXTENSION` or
`SELECT postgis_extensions_upgrade()` during a micro upgrade. Removing a C
symbol can therefore break production code.

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
`CREATE EXTENSION` and then reloads functions, types, and other objects from
the old databases as they existed, still pointing at the same shared library.
When loading those objects, PostgreSQL validates that the referenced functions
exist in the library. If the functions are missing, `pg_upgrade` fails. If a
function exists and only throws an error, `pg_upgrade` can continue.

Old signatures are used because database objects reference functions by object
identifier, not only by name. Recreating a function from scratch gives it a new
OID even if the definition text is identical. Views, tables, and other internal
references would then be broken.

Legacy C signatures exist to keep `pg_upgrade` working.
