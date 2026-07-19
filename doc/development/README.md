---
title: "PostGIS Development Guide"
date: 2026-06-26
weight: 10
geekdocHidden: true
---

This directory is the repository home for PostGIS developer documentation.
Trac remains the project issue tracker, but development workflow notes should
live here so they are reviewed with code and branch changes.

This subtree is organized for the developer and maintainer role. User-facing
documentation remains in the DocBook manual and on the public website; pages
such as [Manual documentation workflow](manual.md) describe how contributors
maintain those user docs rather than forming a separate user-doc chapter here.

Modules:

* [Contributing workflow](contributing.md) explains Trac tickets, code mirrors,
  mailing lists, and patch submission.
* [Development environments](environment/_index.md) links the supported local,
  CI, and container setup paths.
* [Development TODO](todo.md) records high-level future work ideas that are not
  accepted designs or release blockers.
* [Testing and debugging](testing/_index.md) explains SQL regression tests, CUnit,
  dependency guards, and backtraces.
* [Coding style](style.md) covers C formatting, comments, Doxygen comments, and
  source naming conventions.
* [Developer tools](tools.md) covers `logbt`, formatting, coverage, and profiling
  entry points.
* [PostGIS internals](internals/_index.md) covers allocator boundaries, PostgreSQL
  function macros, detoasting, geometry serialization structures, empty
  geometry semantics, spatial collections, raster affine georeferencing,
  raster physical georeferencing parameters, raster storage/WKB, and
  raster/GDAL driver boundaries.
* [Release and upgrade rules](release/_index.md) indexes SQL/C API compatibility,
  upgrade scripting, dependency guards, regression roles, and support-window
  changes.
* [Release process](release-process.md) covers version numbering, release
  preparation, publishing, announcements, and opening the next development
  cycle.
* [Manual documentation workflow](manual.md) covers DocBook, generated comments,
  translated manuals, images, and Doxygen output.
* [Website maintenance](website.md) covers the `postgis.net` Hugo repository,
  public development pages, release pointer updates, and website validation.
* [Pull request and maintainer workflow](maintenance/_index.md) covers source-of-truth
  branches, mirror PR handling, tracker trailers, `NEWS`, and public readback.
* [Governance notes](governance/_index.md) records PSC process, core-contributor
  governance, project/service inventory, and governance-documentation
  consolidation notes.

The generated Doxygen manuals are published at
<https://postgis.net/docs/doxygen/>. User-facing manuals remain at
<https://postgis.net/documentation/>.
