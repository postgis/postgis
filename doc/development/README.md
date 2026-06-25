# PostGIS Developer Documentation

This directory is the repository home for PostGIS developer documentation.
Trac remains the project issue tracker, but development workflow notes should
live here so they are reviewed with code and branch changes.

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
* [Coding style](style.md) covers C formatting, comments, Doxygen comments, and
  source naming conventions.
* [Developer tools](tools.md) covers `logbt`, formatting, coverage, and profiling
  entry points.
* [PostGIS internals](internals.md) covers allocator boundaries, PostgreSQL
  function macros, detoasting, geometry serialization structures, empty
  geometry semantics, spatial collections, raster affine georeferencing, and
  raster physical georeferencing parameters.
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
* [Governance notes](governance.md) records PSC process, core-contributor
  governance, project/service inventory, and governance-documentation
  consolidation notes.
* [Docker setup](environment/docker.md) summarizes the development container
  options historically documented on the Trac developer wiki.
* [Documentation WIP](doc-wip.md) records open migration questions while this
  pull request remains draft.

The generated Doxygen manuals are published at
<https://postgis.net/docs/doxygen/>. User-facing manuals remain at
<https://postgis.net/documentation/>.

## Draft-Only Migration Workspace

The [wiki migration map](wiki.md) and [Trac wiki corpus](../trac-wiki/) are
review aids for this draft pull request. They are not intended to become
published developer documentation. Before this branch leaves draft, each useful
wiki page should either be folded into the maintained topic pages above, left
as historical Trac context, or retired through the cleanup ledger.
