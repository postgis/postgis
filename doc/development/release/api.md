---
title: "API Compatibility"
date: 2026-06-26
weight: 10
geekdocHidden: false
---

Use these pages when a patch changes user-visible SQL objects, C symbols,
extension library names, or upgrade-sensitive compatibility rules.

* [Versioning policy](versioning.md) explains version terminology, library
  naming, and release-line compatibility constraints.
* [Deprecating and removing API](deprecation.md) explains SQL-visible object
  removals and C API stubs for `pg_upgrade`.
* [SQL upgrade and deprecation](sql-scripting.md) explains SQL metadata
  comments, `Replaces`, and before/after upgrade hooks.

## Standards Compatibility

Treat standards compatibility as part of the public API. When a patch changes
geometry construction, parsing, output, predicates, or measurements, check its
behavior against the Simple Features and SQL/MM contracts cited by the affected
manual entry. Record intentional PostGIS extensions or deviations instead of
silently presenting them as standard behavior.

Review at least the following surfaces when they apply:

* empty geometry results and distinctions between empty and `NULL`;
* dimensional semantics, including Z and M preservation or loss;
* validity and simplicity requirements, and behavior for invalid inputs;
* precision, rounding, and robustness at the documented boundary; and
* WKT/WKB compatibility, including EWKT/EWKB extensions and round trips.

Add regression coverage for the standard case and for any intentional PostGIS
extension. Update the function reference with the relevant OGC/SFS or SQL/MM
annotation when compatibility changes.

## Configuration Settings

Treat SQL-visible behavior controlled by PostgreSQL configuration settings as
part of the public API. A GUC that changes query semantics, output standards,
precision, indexing behavior, or upgrade-visible behavior can make the same SQL
return different results in different sessions, so it needs the same review as
an API change.

Prefer explicit SQL function arguments, new functions, or documented extension
objects when users need a durable behavior choice. Reserve GUCs for diagnostics,
logging, cache sizes, external data search paths, or operational settings where
session-local variation is expected and does not change the meaning of a
documented SQL expression.

When a setting is still the right interface, document the default, scope,
supported values, and upgrade impact in the manual, and add regression coverage
for the default behavior.
