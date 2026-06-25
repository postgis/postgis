# Release and Upgrade Rules

This directory collects release-policy and upgrade-safety rules consolidated
from the former `doc/developer.md` and the developer wiki. Use it when a patch
changes SQL API, C API, extension upgrade behavior, dependency requirements, or
supported PostgreSQL versions.

## Topics

* [API compatibility](api/) indexes SQL/C API compatibility rules.
* [Versioning policy](versioning/) explains version terminology, library naming,
  and release-line compatibility constraints.
* [Deprecating and removing API](deprecation/) explains SQL-visible object
  removals and C API stubs for `pg_upgrade`.
* [SQL upgrade and deprecation](sql-scripting/) explains `Availability:`,
  `Changed:`, `Replaces`, before/after upgrade hooks, and parser-sensitive SQL
  style.
* [Dependency and support guards](dependencies/) explains dependency-version
  guards and the checklist for dropping PostgreSQL support.
* [Testing and debugging](../testing/) explains sandboxed regression roles used
  by CI and downstream builders.

## Quick Rules

* Do not add SQL API functions in a patch release.
* Do not remove SQL API or C API functions except in a major release unless the
  old signatures are safely stubbed or migrated.
* Do not require newer dependency-library versions in a micro release.
* Treat `pg_upgrade` as the design constraint for legacy SQL and C signatures.
* Keep upgrade metadata comments current and minimal.
