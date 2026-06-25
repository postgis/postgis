# API Compatibility

Use these pages when a patch changes user-visible SQL objects, C symbols,
extension library names, or upgrade-sensitive compatibility rules.

* [Versioning policy](versioning.md) explains version terminology, library
  naming, and release-line compatibility constraints.
* [Deprecating and removing API](deprecation.md) explains SQL-visible object
  removals and C API stubs for `pg_upgrade`.
* [SQL upgrade and deprecation](sql-scripting.md) explains SQL metadata
  comments, `Replaces`, and before/after upgrade hooks.

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
