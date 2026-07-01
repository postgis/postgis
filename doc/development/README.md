---
title: "PostGIS Developer Documentation"
date: 2026-06-26
weight: 10
layout: toplevel
geekdocHidden: false
---

# PostGIS Developer Documentation

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
* [Development environments](environment.md) links the Ubuntu, Windows, and Docker
  setup paths.
* [Ubuntu setup](environment/ubuntu.md) gives a practical package-based setup,
  build commands, installation into a test cluster, and cleanup.
* [Windows setup](environment/windows.md) records the current MSYS2 and Winnie
  MinGW build surfaces.
* [Testing and debugging](testing.md) explains SQL regression tests, CUnit,
  dependency guards, and backtraces.
* [Development TODO](todo.md) records high-level future work ideas that are not
  accepted designs or release blockers.
* [Coding style](style.md) covers C formatting, comments, Doxygen comments, and
  source naming conventions.
* [Developer tools](tools.md) covers `logbt`, formatting, coverage, and profiling
  entry points.
* [PostGIS internals](internals.md) covers allocator boundaries, PostgreSQL
  function macros, detoasting, geometry serialization structures, empty
  geometry semantics, spatial collections, raster affine georeferencing,
  raster physical georeferencing parameters, raster storage/WKB, and
  raster/GDAL driver boundaries.
* [Release and upgrade rules](release.md) indexes SQL/C API compatibility,
  upgrade scripting, dependency guards, regression roles, and support-window
  changes.
* [Release process](release-process.md) covers version numbering, release
  preparation, publishing, announcements, and opening the next development
  cycle.
* [Manual documentation workflow](manual.md) covers DocBook, generated comments,
  translated manuals, images, and Doxygen output.
* [Website maintenance](website.md) covers the `postgis.net` Hugo repository,
  public development pages, release pointer updates, and website validation.
* [Pull request and maintainer workflow](maintenance.md) covers source-of-truth
  branches, mirror PR handling, tracker trailers, `NEWS`, and public readback.
* [Pull request metadata](maintenance/pull-request-metadata.md) covers labels,
  milestones, draft state, and mirror metadata for PostGIS pull requests.
* [Backpatching fixes](maintenance/backpatching.md) covers supported-branch
  fix propagation, commit provenance, `NEWS`, Trac follow-up, and CI readback.
* [Governance notes](governance.md) records PSC process, core-contributor
  governance, project/service inventory, and governance-documentation
  consolidation notes.
* [Docker setup](environment/docker.md) summarizes the development container
  options historically documented on the Trac developer wiki.

The generated Doxygen manuals are published at
<https://postgis.net/docs/doxygen/>. User-facing manuals remain at
<https://postgis.net/documentation/>.

## Draft-Only Migration Records

The [Trac cleanup ledger](wiki/trac-cleanup.md) and
[Trac wiki corpus record](../trac-wiki/) are review aids for this draft pull
request. They are not intended to become published developer documentation.
The raw `.tracwiki` page import has been fully audited; each page was folded
into maintained topic pages above, left as historical Trac context, or retired
through the cleanup ledger.
