# API Compatibility

Use these pages when a patch changes user-visible SQL objects, C symbols,
extension library names, or upgrade-sensitive compatibility rules.

* [Versioning policy](../versioning/) explains version terminology, library
  naming, and release-line compatibility constraints.
* [Deprecating and removing API](../deprecation/) explains SQL-visible object
  removals and C API stubs for `pg_upgrade`.
* [SQL upgrade and deprecation](../sql-scripting/) explains SQL metadata
  comments, `Replaces`, and before/after upgrade hooks.
