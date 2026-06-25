# PostGIS Internals

These notes cover implementation details that are useful when working inside
PostGIS rather than only using the SQL API.

* [Memory management](memory/) covers allocator boundaries and ownership rules.
* [PostgreSQL internals for PostGIS developers](postgresql/) covers PostgreSQL
  C function macros, varlena detoasting, serialized geometry handling, and a
  typical SQL function flow.
* [2D and 3D distance internals](distance.md) covers how geometry distance
  functions dispatch 2D and 3D candidate checks, including segment-segment and
  surface cases.
