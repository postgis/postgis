# PostGIS development environment

This document aims to get your development environment up as fast as possible so
you can jump ahead into improving the PostGIS codebase.
It assumes Ubuntu 24.04 as it is current on Codex space as the time of writing.

This repository does not ship ready-to-run binaries. Contributors are expected to
build PostGIS and its extensions locally and to run the regression suites before
submitting patches. The notes below summarise what the CI images
(https://github.com/postgis/postgis-build-env) provide and adapt them to a plain
Ubuntu host where Docker is unavailable.

## 1. Base system preparation

* Work as a user that is allowed to run `sudo`.
* Keep the system lean: always prefer `apt-get` with `--no-install-recommends`.
* Make sure the packaged PostgreSQL client binaries precede any other build you
  may already have in `PATH` (see Section 2).

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  autoconf automake autotools-dev \
  bison flex \
  build-essential clang clang-tools clang-format llvm \
  ca-certificates curl wget gnupg \
  ccache cmake ninja-build pkg-config \
  cppcheck \
  gdb valgrind lcov \
  git gettext \
  libcunit1 libcunit1-dev \
  libexpat1-dev libfreexl-dev \
  libgdal-dev gdal-bin \
  libgeos-dev \
  libgmp-dev libmpfr-dev \
  libgsl-dev \
  libjson-c-dev \
  libopenjp2-7-dev libpng-dev libtiff-dev libwebp-dev \
  libpcre3-dev \
  libpq-dev \
  libproj-dev proj-bin \
  libprotobuf-c-dev protobuf-c-compiler \
  libprotobuf-dev protobuf-compiler \
  libsfcgal-dev \
  libsqlite3-dev sqlite3 \
  libspatialite-dev \
  libxml2-dev libxslt1-dev libxml2-utils \
  libyaml-dev \
  postgresql postgresql-client postgresql-server-dev-all \
  python3 python3-distutils python3-pip python3-venv python3-psycopg2 \
  xsltproc docbook-xsl docbook-xsl-ns docbook-xml \
  zlib1g-dev
```

This installs software so local workflow can exercise sanitizers, formatters, and coverage reports.

## 2. Configure PostgreSQL for local testing

The CI images keep PostgreSQL entirely under `/usr/local/pgsql`. On Ubuntu we
use the packaged server binaries instead. When running as a privileged user (the
typical situation in this environment) reuse the default cluster owned by the
`postgres` system account:

```bash
export PG_MAJOR=$(pg_config --version | awk '{print $2}' | cut -d. -f1)
export PGHOME=/usr/lib/postgresql/${PG_MAJOR}
export PATH="${PGHOME}/bin:${PATH}"

# Start the packaged "main" cluster if it is not already online.
sudo -u postgres pg_ctlcluster "${PG_MAJOR}" main start

# Allow automated tools to connect without prompting for a password by editing the existing local entries.
sudo perl -0pi -e 's/^(host\s+all\s+all\s+127\.0\.0\.1\/32\s+)scram-sha-256/\1trust/m; s/^(host\s+all\s+all\s+::1\/128\s+)scram-sha-256/\1trust/m' \
  /etc/postgresql/${PG_MAJOR}/main/pg_hba.conf
sudo -u postgres pg_ctlcluster "${PG_MAJOR}" main reload

# Make the current UNIX account a PostgreSQL superuser.
sudo -u postgres createuser -s $(whoami)

# Alternative: keep SCRAM authentication and set a password for automated tooling.
sudo -u postgres psql -c "ALTER ROLE $(whoami) WITH PASSWORD 'postgres';"
export PGPASSWORD=postgres
```

Revert the authentication edits once you finish testing so other users of the system are not left with trust rules in place.

If you prefer a completely isolated cluster, create an unprivileged build user
and run `initdb` as in the upstream documentation. The remainder of this guide
assumes the packaged cluster is used and listens on the default port 5432.

## 3. Enable core dumps and logbt (optional but recommended)

The `ci/github/logbt` helper captures backtraces from crashes. On Ubuntu you
need elevated privileges once to reconfigure the kernel core pattern.

```bash
sudo ./ci/github/logbt --setup
sudo sysctl kernel.core_pattern
# Keep the directory writable:
sudo mkdir -p /tmp/logbt-coredumps
sudo chmod 1777 /tmp/logbt-coredumps
ulimit -c unlimited
```

When running under constrained CI where you cannot change `kernel.core_pattern`,
skip the `logbt` steps; the test suite still runs but you will not get automatic
core backtraces.

## 4. Build prerequisites

The following projects are detected automatically via `pkg-config` or helper
scripts during `./configure`. After installing the Ubuntu packages you should
have:

* GEOS (`geos-config`)
* PROJ (`proj` >= 7)
* GDAL (`gdal-config`)
* JSON-C (`json-c` >= 0.13)
* PROTOBUF-C (for `ST_AsMVT`)
* SFCGAL (for 3D/advanced geometry tests)
* Libxml2, libxslt and docbook stylesheets (for documentation targets)
* Valgrind (for CUnit leak checks)

Verify their presence if `./configure` fails.

## 5. Building PostGIS

```bash
./autogen.sh
./configure \
  --with-jsondir=/usr \
  --with-projdir=/usr \
  --with-raster \
  --with-topology \
  --with-sfcgal
make -j"$(nproc)"
```

When iterating repeatedly, `make clean` or `make distclean` may be necessary if
you switch compilers or change major dependencies.

## 6. Running the regression tests

Tests expect the PostgreSQL cluster started in section 2 to be running and the
current user to be able to create databases. Export the connection parameters so
every helper uses the same cluster:

```bash
export PGHOST=127.0.0.1
export PGPORT=5432        # adjust if your cluster uses another port
export PGUSER=$(whoami)
export PGDATABASE=postgres
```

Before running the tests, install the freshly built artifacts into PostgreSQL so
that the regression harness can locate the control files and shared libraries:

```bash
sudo make install
```

Execute the same test targets used in CI:

```bash
# Standard compile + regression suite
make check RUNTESTFLAGS="--verbose --extension --raster --topology --sfcgal --tiger"

# Undefined behaviour sanitiser runs
CC=clang ./configure CFLAGS="-g3 -O0 -fno-omit-frame-pointer -fsanitize=undefined" LDFLAGS="-fsanitize=undefined"
make -j"$(nproc)"
make check RUNTESTFLAGS="--verbose --extension --raster --topology --sfcgal"

# Coverage build (requires lcov and a clean tree)
make distclean
CFLAGS="-g -O0 --coverage" LDFLAGS="--coverage" ./configure --enable-debug \
  --with-jsondir=/usr --with-projdir=/usr --with-raster --with-topology --with-sfcgal
make -j"$(nproc)"
make check RUNTESTFLAGS="--verbose --extension --raster --topology --sfcgal"
```

Additional targets of interest:

* `make garden` – runs documentation and extra QA checks.
* `make check RUNTESTFLAGS="--upgrade --extension --verbose"` – exercises
  extension upgrade paths; make sure `sudo make install` has been executed into
  the same PostgreSQL instance before invoking this.
* `make installcheck` – runs tests against an installed copy (after `make install`).

### Running a single C unit test

After building the tree (Section 5), you can exercise one CUnit suite in
isolation. From the repository root run:

```bash
cd liblwgeom/cunit
make cu_tester
./cu_tester geodetic    # replace "geodetic" with another suite or test name as needed
```

The `cu_tester` binary also accepts individual test names if you need to drill
down further.

### Running a single SQL regression test

With PostgreSQL running as configured above and the extension installed via
`sudo make install`, target one SQL file by passing it to the regression driver:

```bash
export PGPASSWORD=postgres   # or reuse another password if you kept SCRAM auth
make -C regress check RUNTESTFLAGS="--extension" TESTS="$(pwd)/regress/core/affine"
```

Swap `regress/core/affine` for the test you need.


## 7. Inspecting coverage results

Coverage builds rely on the `lcov` utilities. After executing the coverage `make check` run:

```bash
# Capture data from the build tree
lcov --capture --directory . --output-file coverage.info

# Optionally filter out system libraries and generated files
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.filtered.info

# Produce an HTML report for browsing
genhtml coverage.filtered.info --output-directory coverage-html

# Quickly spot uncovered lines in the terminal
lcov --list coverage.filtered.info | less
```

Open `coverage-html/index.html` in a browser or upload the report to your CI
artifacts to explore missing lines interactively.

If you rely on the `ci/github/run_*.sh` wrappers, export `PGHOME` and `PGDATA` as
shown above and replace `/usr/local/pgsql/bin/pg_ctl` in those scripts with the
packaged location or create a symlink at `/usr/local/pgsql/bin` pointing to
`${PGHOME}/bin`.

## 8. Cleaning up

To reclaim disk space:

```bash
sudo -u postgres pg_ctlcluster "${PG_MAJOR}" main stop
sudo rm -rf /tmp/logbt-coredumps
```

If you created a private cluster instead of using the packaged one, stop it with
`pg_ctl -D "$PGDATA" stop` and remove the data directory manually.

## 9. Maintaining formatting

Before submitting patches that modify C or C++ files, run clang-format only on
the relevant hunks to avoid unrelated churn:

```bash
git clang-format
```

The command formats staged changes by default. Use `git clang-format <commit>` to
reflow a specific range, or pass `--extensions c,cpp,h` when touching files with
non-standard suffixes. Validate that the resulting diff stays focused on the
code you touched.

Review `STYLE` file in root of the repository for other stylistic preferences.
For release policies, upgrade implications, and naming
conventions for new features, skim `doc/developer.md` (PostGIS Developer
How-To) for a concise overview of those workflows.

The Codecov upload commands in `ci/github/run_coverage.sh` and
`ci/github/run_garden.sh` require outbound network access; they are harmless to
ignore locally.
