---
title: "Ubuntu Development Environment"
date: 2026-06-26
weight: 10
geekdocHidden: false
---

This document aims to get an Ubuntu development environment ready quickly so you
can build and test the PostGIS codebase. The package list below targets Ubuntu
24.04.

This repository does not ship ready-to-run binaries. Contributors are expected
to build PostGIS and its extensions locally before submitting patches. The
notes below summarise what the CI images
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
  python3 python3-pip python3-venv python3-psycopg2 \
  xsltproc docbook-xsl docbook-xsl-ns docbook-xml \
  zlib1g-dev
```

This installs software so local workflow can exercise builds, sanitizers,
formatters, and regression suites.

## 2. Configure PostgreSQL for local testing

The CI images keep PostgreSQL entirely under `/usr/local/pgsql`. On Ubuntu we
use the packaged server binaries instead. When running as a user with `sudo`
access, reuse the default cluster owned by the `postgres` system account:

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

## 3. Build prerequisites

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

## 4. Building PostGIS

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

## 5. Installing for tests

Most regression tests expect the PostgreSQL cluster configured above to be
running and the freshly built artifacts to be installed into that same
PostgreSQL instance:

```bash
sudo make install
```

See [Testing and debugging](../testing/_index.md) for regression commands, CUnit
tests, coverage, dependency guards, and backtrace capture. Optional tooling such
as `logbt` setup lives in [Developer tools](../tools.md).

## 6. Cleaning up

To reclaim disk space:

```bash
sudo -u postgres pg_ctlcluster "${PG_MAJOR}" main stop
```

If you created a private cluster instead of using the packaged one, stop it with
`pg_ctl -D "$PGDATA" stop` and remove the data directory manually.

## 7. Maintaining formatting

Review [Coding style](../style.md) for source-formatting preferences, including
`git clang-format` usage for C and C++ changes. For release policies, upgrade
implications, and naming conventions for new features, read
[Release and upgrade rules](../release/_index.md).
