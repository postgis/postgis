---
title: "Versioning Policy"
date: 2026-06-26
weight: 20
geekdocHidden: false
---

PostGIS version names use three digits separated by periods:

```text
Major.Minor.Patch
```

| Version | Meaning | Example |
|---------|---------|---------|
| `3.0.0` | Major release (new API surface) | First release of a major version |
| `3.6.0` | Minor release (new features, backwards-compatible) | Added new functions |
| `3.0.1` | Patch release (bug fixes, data changes) | Small fixes to `3.0.0` |

SQL API functions are functions exposed to users in the database. They are SQL,
PL/pgSQL, or C-backed functions.

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
`rtpostgis` library name did not match the extension name. For upgrade purposes,
old `rtpostgis` is equivalent to current `postgis_raster`.

These changes were made to reduce `pg_upgrade` pain. Developers can still use
minor-versioned libraries for easier testing in the same cluster by configuring
with:

```sh
./configure --with-library-minor-version
```

## Release-Line Rules

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
require newer dependency-library versions in the first release of a minor series.
For example, PostGIS often drops support for older GEOS and PostgreSQL versions
in a new minor release.
