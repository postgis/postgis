# Testing and Debugging

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
   psql formatting.
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

## Standard Test Runs

Tests expect PostgreSQL to be running and the current user to be able to create
databases. The setup in [Building PostGIS](../environment/ubuntu/) configures a packaged
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

## Garden Checks

Run garden checks with:

```sh
make garden
```

Garden tests exercise documentation examples and broader SQL behavior. They are
useful before release work, when changing visible SQL behavior, and when manual
examples are added or reorganized.

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
as described in [Building PostGIS](../environment/ubuntu/) and replace
`/usr/local/pgsql/bin/pg_ctl` in those scripts with the packaged location or
create a symlink at `/usr/local/pgsql/bin` pointing to `${PGHOME}/bin`.

## CUnit Tests

CUnit tests live in `liblwgeom/cunit`. Prefer them for low-level geometry
library behavior because they are faster and easier to debug than SQL tests.

To add a test:

1. Pick an existing module that matches the code under test, or copy an
   existing module when adding a large new area.
2. Name the test function `test_*`.
3. Declare the test in the module header.
4. Register it in that module's suite.
5. Include the Trac ticket number in a short comment when the test captures a
   ticket fix.

Run CUnit tests:

```sh
make -C liblwgeom/cunit cu_tester
./liblwgeom/cunit/cu_tester geodetic
```

## Backtraces

For crashes, a backtrace from the PostgreSQL backend is usually the most useful
diagnostic artifact.

Build PostgreSQL and PostGIS with debug symbols when possible. In one terminal,
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
