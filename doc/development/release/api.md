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

## Standards Conformance

When a patch changes constructors, parsers, serializers, predicates,
measurements, validity, simplicity, overlay, or dimensional behavior, review
the change against the standards and PostGIS extensions that define the
current public contract.

Check the user-visible behavior against the relevant OGC/SFS, ISO 19125,
SQL/MM, WKT, WKB, EWKT, EWKB, SRID, and SQL/MM type-code expectations. PostGIS
also has deliberate extensions and historical compatibility behavior, so do
not normalize behavior only because one standard spelling exists. Document
the chosen behavior in the manual when it can surprise users.

Specific review points:

* Keep empty-geometry behavior documented and tested for constructors,
  predicates, validity checks, accessors, measurements, and overlay or
  constructive functions. See
  [Empty geometry semantics](../internals/empty-geometry.md) for the current
  implementation map.
* Be explicit when a spatial predicate, overlay, or measurement intentionally
  uses 2D semantics for higher-dimensional input. Add or point to a separate
  3D function when users need true 3D behavior.
* Keep validity and simplicity documentation explicit about geometry-type
  criteria, repeated points, invalid input handling, parser acceptance, and
  repair guidance.
* Treat precision-model changes as API changes. Current ordinary geometries do
  not carry inferred precision metadata; explicit overlay grid size, topology
  precision, MVT quantization, and tolerance arguments are separate API
  surfaces. See
  [Precision and tolerance internals](../internals/precision-tolerance.md).
* When changing WKT/WKB parsing or output, test OGC, ISO, SQL/MM, EWKT, EWKB,
  SRID, empty-geometry, dimensionality, and type-code compatibility together.
