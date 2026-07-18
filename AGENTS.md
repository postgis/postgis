# PostGIS Agent Notes

For build, test, and contributor workflow instructions, use
[`doc/development/`](doc/development/). In particular:

* [`doc/development/environment/ubuntu.md`](doc/development/environment/ubuntu.md) covers local
  dependencies, PostgreSQL setup, build commands, install steps, and cleanup.
* [`doc/development/testing/_index.md`](doc/development/testing/_index.md) covers regression
  commands, CUnit tests, coverage, dependency guards, and backtraces.
* [`doc/development/contributing.md`](doc/development/contributing.md) covers Trac,
  code mirrors, mailing lists, and patch submission.
* [`doc/development/style.md`](doc/development/style.md) covers source formatting,
  comments, Doxygen comments, and naming conventions.
* [`doc/development/internals/_index.md`](doc/development/internals/_index.md) covers allocator
  boundaries, PostgreSQL C API macros, detoasting, and geometry serialization
  structures.
* [`doc/development/website.md`](doc/development/website.md) covers the `postgis.net`
  Hugo repository, website release pointers, and website validation.
* [`doc/development/maintenance/_index.md`](doc/development/maintenance/_index.md) covers pull
  request landing, tracker trailers, `NEWS`, branch readback, and external
  service hygiene.
* [`doc/development/governance/_index.md`](doc/development/governance/_index.md) covers the
  current RFC-5 status, umbrella project list, and governance-documentation
  consolidation notes.
* [`doc/development/release/_index.md`](doc/development/release/_index.md) covers SQL/C API
  compatibility, upgrade scripting, dependency guards, and support-window
  changes.
* [`doc/development/release-process.md`](doc/development/release-process.md) covers
  release preparation, publishing, announcements, and opening the next
  development cycle.

When changing this repository, preserve unrelated local worktree state. Use the
Gitea repository as the canonical upstream and treat GitHub as a mirror for pull
request mechanics. Prefer focused validation for the touched subsystem, and run
`git clang-format` on changed C or C++ hunks before final validation.
