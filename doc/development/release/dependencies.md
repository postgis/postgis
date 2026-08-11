---
title: "Dependency and Support Guards"
date: 2026-06-26
weight: 50
geekdocHidden: false
---

This page explains how to guard functionality that depends on newer libraries
and how to remove support for older PostgreSQL versions.

## Dependency Library Guarding

When functionality can only be used with a dependency library newer than a
specific version, guard both the implementation and the tests. PostgreSQL
version-dependent behavior can also need SQL-file guards.

PostgreSQL guards appear in SQL files and C files:

```c
#if POSTGIS_PGSQL_VERSION >= 150
/* code that requires PostgreSQL 15+ */
#endif
```

Add matching test guards in `regress/**/tests.mk.in`.

GEOS guards appear in C files:

```c
#if POSTGIS_GEOS_VERSION < 31300
/* GEOS < 3.13 code goes here */
#endif
```

Add matching test guards in `regress/**/tests.mk.in` or
`raster/test/regress/tests.mk.in`:

```make
ifeq ($(shell expr "$(POSTGIS_GEOS_VERSION)" ">=" 31300),1)
TESTS += \
  my_geos_313_test
endif
```

SFCGAL guards appear in C files:

```c
#if POSTGIS_SFCGAL_VERSION >= 20300
/* SFCGAL 2.3+ required */
#endif
```

Add matching test guards in `sfcgal/regress/tests.mk.in`:

```make
ifeq ($(shell expr "$(POSTGIS_SFCGAL_VERSION)" ">=" 20300),1)
TESTS += \
  my_sfcgal_203_test
endif
```

PROJ guards appear in C files:

```c
#if POSTGIS_PROJ_VERSION > 60000
/* PROJ 6.0+ code */
#endif
```

GDAL guards appear in C files:

```c
#if POSTGIS_GDAL_VERSION < 30700
/* GDAL < 3.7 logic */
#endif
```

Even if a user cannot use a function with their compiled dependency set, the
function still needs to be exposed. It should report an error explaining which
library version is required. The function must still exist in the C library, so
guards are almost always on the C side and only rarely in SQL files.

## Dependency Version Reporting

`postgis_full_version()` should report dependency details that help reproduce
bugs from package, source-build, and CI environments. When an upstream
dependency exposes revision-level build information, prefer wiring that into
the PostGIS version-reporting path instead of inventing a PostGIS-side parser
for dependency source trees.

For GEOS, revision-level runtime and header information is tracked upstream in
<https://github.com/libgeos/geos/issues/1446>. Once GEOS exposes that data,
PostGIS can decide how much of it belongs in `postgis_full_version()` and the
GEOS version helper functions.

## Vendored Dependency Refreshes

PostGIS vendors a small number of source dependencies under `deps/`. Treat
these directories as maintained source snapshots, not as package-manager
inputs. Before refreshing one, read its local README, compare the PostGIS-local
bridge patches, and record the exact upstream release tag or commit used.

| Dependency | Local version or snapshot | Upstream source | Refresh notes |
|------------|---------------------------|-----------------|---------------|
| Wagyu | 0.5.0 | <https://github.com/mapbox/wagyu> | Used only when protobuf-c support enables MVT. The vendored copy includes PostGIS bridge code in `deps/wagyu/lwgeom_wagyu.*`. |
| Ryu | v2.0-derived snapshot | <https://github.com/ulfjack/ryu> | `deps/ryu/README.md` documents substantial PostGIS precision and formatting changes. Do not overwrite it from upstream without preserving those changes and validating `lwprint_double` output. This copy includes the unreleased upstream `pow5Factor()` optimization from <https://github.com/ulfjack/ryu/pull/188>. |
| FlatGeobuf | 3.26.2 with FlatBuffers 23.3.3 | <https://github.com/flatgeobuf/flatgeobuf> | Refreshed from FlatGeobuf 3.26.2; its C++ sources and schemas are unchanged from the 3.25.0 import at https://github.com/postgis/postgis/pull/726. The version is tracked here because the upstream source files do not embed it. Regenerate the FlatGeobuf headers, preserve the unique FlatBuffers namespace, and keep PostGIS big-endian fixes when refreshing. The portable `packedrtree.cpp` big-endian fix is proposed upstream at https://github.com/flatgeobuf/flatgeobuf/pull/512. |
| uthash | 2.4.0 | <https://github.com/troydhanson/uthash> | `deps/uthash/include/uthash.h` comes from upstream `v2.4.0/src/uthash.h` with PostGIS' `HASH_FUNCTION` collision fix retained as `UTHASH_FUNCTION`. |

## Removing Support for PostgreSQL Versions

When dropping support for an older PostgreSQL major version:

1. Edit `configure.ac`, starting near the minimum-version check such as
   `dnl Ensure that we are using PostgreSQL >= 14`.
2. Remove PostgreSQL guards for versions lower than the new minimum. Search for
   `POSTGIS_PGSQL_VERSION` and older `PG_VERSION_NUM` syntax.
3. Edit `doc/postgis.xml` and update the `min_postgres_version` entity.
4. Update CI scripts that hard-code a PostgreSQL major for packaging or docs
   jobs, notably `ci/debbie/postgis_make_dist.sh` and
   `ci/debbie/postgis_release_docs.sh`.
5. Add a `NEWS` entry under `Breaking Changes`.

## Support Matrix Maintenance

The compatibility data, updater, validator, fallback cache, and generated JSON
payload belong to this repository. The website consumes that payload for its
own compatibility page and should not maintain a second compatibility table.
See [Compatibility data maintenance](../compatibility/_index.md) for the full
data and publication contract. The support-policy summary remains at
<https://postgis.net/development/versions_eol/>.

When updating release or dependency support:

1. Run `python3 utils/docs/support_matrix.py update`; a changed upstream format
   retains the corresponding last-known-good fields in `cache.json` and
   records a visible warning without rewriting `matrix.json`.
2. Run `python3 utils/docs/support_matrix.py check` before publishing. It validates
   the source data, resolved compatibility cells, patch overlays, dependency
   inventory, and generated payload schema.
3. Check the website support policy before changing branch, release, or
   announcement wording.
4. Treat all rows marked EOL as historical context only. EOL PostGIS versions
   do not receive micro updates or security fixes.
5. Distinguish "supported", "builds but not recommended", and "assumed to work
   but not tested" when writing release notes or compatibility text.
6. Remember the historical project rule of thumb: support at least two
   PostgreSQL major versions for each PostGIS release line, usually more when
   dependency requirements allow it, but rarely more than five.

Old Trac compatibility tables are useful for archaeology, but do not copy them
into current guidance without checking the release branch, dependency guards,
CI matrix, and website support policy.
