# PostGIS Developer Documentation

This directory is the repository home for PostGIS developer documentation.
Trac remains the project issue tracker, but development workflow notes should
live here so they are reviewed with code and branch changes.

Modules:

* [Contributing workflow](workflow/) explains Trac tickets, code mirrors,
  mailing lists, and patch submission.
* [Development environments](environment/) links the Ubuntu and Docker setup
  paths.
* [Ubuntu setup](environment/ubuntu/) gives a practical package-based setup,
  build commands, installation into a test cluster, and cleanup.
* [Testing and debugging](testing/) explains SQL regression tests, CUnit,
  dependency guards, and backtraces.
* [Coding style](style/) covers C formatting, comments, Doxygen comments, and
  source naming conventions.
* [Developer tools](tools/) covers `logbt`, formatting, coverage, and profiling
  entry points.
* [PostGIS internals](internals/) covers allocator boundaries, PostgreSQL
  function macros, detoasting, and geometry serialization structures.
* [Release and upgrade rules](release/) indexes SQL/C API compatibility,
  upgrade scripting, dependency guards, regression roles, and support-window
  changes.
* [Release process](release-process/) covers version numbering, release
  preparation, publishing, announcements, and opening the next development
  cycle.
* [Manual documentation workflow](manual/) covers DocBook, generated comments,
  translated manuals, images, and Doxygen output.
* [Website maintenance](website/) covers the `postgis.net` Hugo repository,
  public development pages, release pointer updates, and website validation.
* [Pull request and maintainer workflow](maintenance/) covers source-of-truth
  branches, mirror PR handling, tracker trailers, `NEWS`, and public readback.
* [Governance notes](governance/) records the current RFC-5 status, umbrella
  project list, and governance-documentation consolidation notes.
* [Docker setup](environment/docker/) summarizes the development container options
  historically documented on the Trac developer wiki.
* [Wiki migration map](wiki/) records the Trac wiki pages that were inspected
  and the repository location that should now hold maintained content.
* [Documentation WIP](doc-wip.md) records open migration questions while this
  pull request remains draft.
* [Trac wiki corpus](../trac-wiki/) archives the PostGIS-specific Trac wiki
  source pages pulled during the migration.

The generated Doxygen manuals are published at
<https://postgis.net/docs/doxygen/>. User-facing manuals remain at
<https://postgis.net/documentation/>.
