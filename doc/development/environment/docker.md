---
title: "Docker Development Environment"
date: 2026-06-26
weight: 30
geekdocHidden: false
---

PostGIS has two container families that are useful for development and testing.
Production images are documented separately at
<https://github.com/postgis/docker-postgis> and should not be treated as build
environments.

## OSGeo Build-Test Images

The OSGeo development images are hosted at `repo.osgeo.org/postgis/build-test`.
They contain multiple PostgreSQL versions and preinstalled PostGIS versions for
upgrade testing. They are large, but useful for testing dependency combinations
and older upgrade paths.

```sh
docker pull repo.osgeo.org/postgis/build-test:universal
mkdir -p ~/projects
docker run -d \
  --name postgis-dev \
  --mount type=bind,source="$HOME/projects",target=/projects \
  -p 6432:5432/tcp \
  -p 6433:5433/tcp \
  -p 6434:5434/tcp \
  -p 6435:5435/tcp \
  -p 6436:5436/tcp \
  -p 6437:5437/tcp \
  repo.osgeo.org/postgis/build-test:universal tail -f /dev/null
docker exec -it postgis-dev /bin/bash
```

Inside the container:

```sh
cd /projects
git clone https://gitea.osgeo.org/postgis/postgis.git
cd postgis
export PGVER=15
service postgresql start "${PGVER}"
export PGPORT="$(grep ^port /etc/postgresql/${PGVER}/main/postgresql.conf | awk '{print $3}')"
export PATH="/usr/lib/postgresql/${PGVER}/bin:${PATH}"
./autogen.sh
./configure
make -j"$(nproc)"
make check
```

From the host, connect to the PostgreSQL version whose container port you
exposed. For example, if PostgreSQL inside the container listens on `5436` and
the host port mapping is `6436:5436`, use:

```sh
psql -U postgres -h localhost -p 6436
```

Stop and remove the container with:

```sh
docker stop postgis-dev
docker rm postgis-dev
```

## postgis-build-env Images

The `postgis/postgis-build-env` images are used by GitHub Actions. They contain
the dependencies needed to compile PostGIS, but do not install PostGIS as a
ready-to-use database image.

```sh
docker pull postgis/postgis-build-env:latest
mkdir -p ~/projects
# The container expects the PostGIS checkout at /projects/postgis. Reuse an
# existing checkout there, or clone one before starting the container.
test -d "$HOME/projects/postgis/.git" || \
  git clone https://gitea.osgeo.org/postgis/postgis.git "$HOME/projects/postgis"
docker run -d \
  --name postgis-build-env \
  -v "$HOME/projects:/projects" \
  postgis/postgis-build-env:latest tail -f /dev/null
docker exec -it postgis-build-env /bin/bash
```

Inside the container, prefer an out-of-tree build when iterating:

```sh
SRCDIR=/projects/postgis
BUILDDIR=/src/postgis
cd "${SRCDIR}"
./autogen.sh
mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
"${SRCDIR}/configure" --without-interrupt-tests --enable-lto
make -j"$(nproc)"
/usr/local/pgsql/bin/pg_ctl -c -l /tmp/logfile -o '-F' start
make check
```

Use these images for development and CI reproduction, not production service
deployment.
