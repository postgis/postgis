---
title: "Testing and Debugging"
date: 2026-06-26
weight: 40
geekdocHidden: false
geekdocCollapseSection: true
---

PostGIS uses both SQL regression tests and CUnit tests. Choose the lowest layer
that exercises the behavior you changed.

Use CUnit for `liblwgeom` behavior that does not require PostgreSQL internals.
Use SQL regression tests for SQL API behavior, extension install and upgrade
paths, planner behavior, or behavior that depends on PostgreSQL types.

## SQL Regression Tests

Regression tests live under `regress/`, `raster/test/regress/`,
`topology/test/`, and related subsystem directories. A typical test has a
`.sql` file and a matching `_expected` file.

To add a new SQL regression:

1. Add a focused `.sql` file or append to the ticket-oriented test file when
   that matches local convention.
2. Include stable labels in query output when a file covers several cases.
3. Generate expected output through the regression harness, not by guessing
   psql formatting. A direct `psql -tXA` run can seed an `_expected` file, but
   the committed result should match the output produced by `make check` after
   the harness applies its substitutions.
4. Add the test to the relevant `tests.mk.in` or subsystem makefile.
5. Guard tests that require a dependency version newer than the release
   minimum.

Example dependency guard:

```make
ifeq ($(shell expr "$(POSTGIS_GEOS_VERSION)" ">=" 31300),1)
TESTS += \
  my_geos_313_test
endif
```

Run one SQL regression from the repository root:

```sh
make -C regress check RUNTESTFLAGS="--extension" TESTS="$(pwd)/regress/core/affine"
```

If a failed run leaves `postgis_reg` behind, drop it before rerunning:

```sh
dropdb postgis_reg
```

When testing against several PostgreSQL installs, put the target PostgreSQL
client binaries first in `PATH` and set `PGPORT`, `PGUSER`, and `PGHOST`
explicitly.

### Sandboxed Regression Roles

Continuous integration workers are frequently configured with sandboxed
PostgreSQL roles that cannot create databases or install extensions. The
regression harness accepts two environment variables to test the complete
installation flow without granting superuser rights to the calling account.

`POSTGIS_REGRESS_DB_OWNER` tells `regress/run_test.pl` to hand ownership of the
temporary regression database to a privileged role while the less privileged
caller maintains the session. This mirrors production setups where database
creation is delegated to controlled roles.

`POSTGIS_REGRESS_ROLE_EXT_CREATOR` optionally overrides the role used for
extension creation when it is distinct from the database owner.

When both roles are configured in PostgreSQL to create the PostGIS extensions,
the test suite exercises the same upgrade and `CREATE EXTENSION` pathways as a
superuser-driven run. This avoids the traps described in
<https://trac.osgeo.org/postgis/ticket/5212> while keeping the CI account
unprivileged.

## Standard Test Runs

Tests expect PostgreSQL to be running and the current user to be able to create
databases. The setup in [Building PostGIS](../environment/ubuntu.md) configures a packaged
cluster for local testing.

Export connection parameters so every helper uses the same cluster:

```sh
export PGHOST=127.0.0.1
export PGPORT=5432        # adjust if your cluster uses another port
export PGUSER=$(whoami)
export PGDATABASE=postgres
```

Install the freshly built artifacts before running extension tests:

```sh
sudo make install
```

Run the standard compile and regression suite:

```sh
make check RUNTESTFLAGS="--verbose --extension --raster --topology --sfcgal"
```

Run Undefined Behavior Sanitizer validation:

```sh
CC=clang ./configure CFLAGS="-g3 -O0 -fno-omit-frame-pointer -fsanitize=undefined" LDFLAGS="-fsanitize=undefined"
make -j"$(nproc)"
make check RUNTESTFLAGS="--verbose --extension --raster --topology --sfcgal"
```

Other useful targets:

* `make check RUNTESTFLAGS="--upgrade --extension --verbose"` exercises
  extension upgrade paths after `sudo make install`.
* `make installcheck` runs tests against an installed copy after
  `sudo make install`.
* `make check-contributor-credits` runs an optional maintainer audit that checks
  that every non-merge Git author, `Co-authored-by` trailer, and contributor
  named in an unreleased `NEWS` section is present in `doc/credits.xml`. It is
  not part of `make check` because it needs Python and a Git worktree with
  full, non-shallow history. GitHub Actions and Woodpecker each run a dedicated
  contributor-credit job with full history, keeping the audit in CI without
  making ordinary build or packaging checks depend on it. Release trees without
  `.git` skip the check. Add aliases to `.mailmap` instead of duplicating a
  person under multiple names in the manual. If only a public handle can be
  verified, use a linked `@handle (GitHub user)` credit. If only a partial name
  is public, use `@handle (GitHub user: partial name)` to credit both forms and
  keep all handle-first entries together at the start of the list. Do not infer
  a person's name from an email address.
* [CI inventory standards](ci.md) describe how to keep build-bot and badge
  inventories checkable instead of copying stale dashboard markup into
  maintained prose.

## Garden Checks

Run garden checks with:

```sh
make garden
```

Garden tests are generated from the DocBook reference through the XSL files in
`doc/xsl/`. They exercise documented geometry, geography, raster, SFCGAL, and
topology functions with broad input sets, including NULL and empty inputs, to
catch crashes and surprising SQL behavior.

The generated scripts write progress and output tables such as
`postgis_garden_log`, `postgis_garden_log_output`, and raster-specific garden
logs. If a run crashes before completion, inspect the highest `logid` to find
the SQL statement that was running:

```sql
SELECT *
FROM postgis_garden_log
ORDER BY logid DESC
LIMIT 1;
```

Use garden checks before release work, when changing visible SQL behavior, and
when manual examples are added or reorganized. There is no maintained subset
garden generator; for focused debugging, generate the relevant garden SQL and
copy the statement for the target function from the generated script or from
the matching garden log entry.

## Coverage

Coverage builds require `lcov` and a clean tree:

```sh
make distclean
CFLAGS="-g -O0 --coverage" LDFLAGS="--coverage" ./configure --enable-debug \
  --with-jsondir=/usr --with-projdir=/usr --with-raster --with-topology --with-sfcgal
make -j"$(nproc)"
make check RUNTESTFLAGS="--verbose --extension --raster --topology --sfcgal"
```

Capture and inspect coverage data:

```sh
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.filtered.info
genhtml coverage.filtered.info --output-directory coverage-html
lcov --list coverage.filtered.info | less
```

Open `coverage-html/index.html` in a browser or upload the report to CI
artifacts to inspect missing lines.

If you rely on the `ci/github/run_*.sh` wrappers, export `PGHOME` and `PGDATA`
as described in [Building PostGIS](../environment/ubuntu.md) and replace
`/usr/local/pgsql/bin/pg_ctl` in those scripts with the packaged location or
create a symlink at `/usr/local/pgsql/bin` pointing to `${PGHOME}/bin`.

## CUnit Tests

CUnit tests live in `liblwgeom/cunit`. Prefer them for low-level geometry
library behavior because they are faster and easier to debug than SQL tests.
The local `liblwgeom/cunit/README` is the source of truth for the current test
harness details.

To add a test:

1. Pick an existing module that matches the code under test, or copy an
   existing module when adding a large new area.
2. Name the test function `test_*`.
3. Declare the test in the module header.
4. Register it in that module's suite.
5. Include the Trac ticket number in a short comment when the test captures a
   ticket fix.

When a change needs a new CUnit module, copy a nearby module and update
`cu_tester.c` to include the new header and call the new `register_*_suite()`
function. Keep module-level initialization minimal so tests can still be run or
disabled independently while debugging.

Run CUnit tests:

```sh
make -C liblwgeom/cunit cu_tester
./liblwgeom/cunit/cu_tester geodetic
```

## Backtraces

For crashes, a backtrace from the PostgreSQL backend is usually the most useful
diagnostic artifact.

Use `gdb` on Linux, BSD, macOS, Solaris, MSYS2, Cygwin, or similar POSIX-style
environments. On Windows, install the MinGW/MSYS2 debugging tools first. Verify
the debugger is available with:

```sh
gdb -v
```

Build PostgreSQL and PostGIS with debug symbols when possible; a PostgreSQL
source build can use `CFLAGS=-O0 ./configure --enable-debug`. In one terminal,
connect to a PostGIS-enabled database and get the backend PID:

```sql
SELECT postgis_full_version(), pg_backend_pid();
```

In another terminal, attach `gdb`:

```sh
gdb -p <pid>
```

Run the crashing query in the psql terminal. When the backend stops in `gdb`,
capture:

```gdb
bt
```

For regression-test crashes, a safer workflow is:

```sh
make staged-install
regress/run_test.pl --nodrop regress/core/geography.sql
psql postgis_reg
```

Then get `pg_backend_pid()`, attach `gdb`, and rerun the failing SQL file inside
that same database.

Paste the `bt` output unchanged into the bug report or mailing-list thread so
developers can see the original stack frames.
