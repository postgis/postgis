# PostGIS Agent Notes

For build, test, and contributor workflow instructions, use
[`doc/development/`](doc/development/). In particular:

* [`doc/development/building/`](doc/development/building/) covers local
  dependencies, PostgreSQL setup, build commands, install steps, and cleanup.
* [`doc/development/testing/`](doc/development/testing/) covers regression
  commands, CUnit tests, coverage, dependency guards, and backtraces.
* [`doc/development/workflow/`](doc/development/workflow/) covers Trac,
  code mirrors, mailing lists, and patch submission.
* [`doc/development/style/`](doc/development/style/) covers source formatting,
  comments, Doxygen comments, and naming conventions.
* [`doc/development/internals/`](doc/development/internals/) covers allocator
  boundaries, PostgreSQL C API macros, detoasting, and geometry serialization
  structures.
* [`doc/development/website/`](doc/development/website/) covers the `postgis.net`
  Hugo repository, website release pointers, and website validation.
* [`doc/development/maintenance/`](doc/development/maintenance/) covers pull
  request landing, tracker trailers, `NEWS`, branch readback, and external
  service hygiene.
* [`doc/development/governance/`](doc/development/governance/) covers the
  current RFC-5 status, umbrella project list, and governance-documentation
  consolidation notes.
* [`doc/development/release/`](doc/development/release/) covers SQL/C API
  compatibility, upgrade scripting, dependency guards, and support-window
  changes.
* [`doc/development/release-process/`](doc/development/release-process/) covers
  release preparation, publishing, announcements, and opening the next
  development cycle.

When changing this repository, preserve unrelated local worktree state. Use the
Gitea repository as the canonical upstream and treat GitHub as a mirror for pull
request mechanics. Prefer focused validation for the touched subsystem, and run
`git clang-format` on changed C or C++ hunks before final validation.
