---
title: "FreeBSD Development Environment"
date: 2026-06-27
weight: 30
geekdocHidden: false
---

The current FreeBSD pull-request build is defined in
`.github/workflows/ci-freebsd.yml`. Use that workflow as the source of truth
when checking platform-specific dependencies or flags.

At the time of this note, the job runs FreeBSD 14.3 through
`vmactions/freebsd-vm`, switches the package repository from quarterly to
latest, installs PostgreSQL 17 packages, and builds with clang through ccache.
It builds the topology, SFCGAL, and protobuf paths, but disables raster and GUI:

```sh
sed -i.bak \
  -e 's,pkg+http://pkg.FreeBSD.org/${ABI}/quarterly,pkg+http://pkg.FreeBSD.org/${ABI}/latest,' \
  /etc/pkg/FreeBSD.conf
ASSUME_ALWAYS_YES=yes pkg bootstrap -f
env IGNORE_OSVERSION=yes pkg update -f
env IGNORE_OSVERSION=yes pkg install -y \
  autoconf automake bison ccache-static cunit docbook \
  gdal geos gmake iconv json-c libtool libxml2 libxslt \
  pkgconf postgresql17-contrib postgresql17-server \
  proj protobuf-c sfcgal
projsync --system-directory --source-id us_noaa
projsync --system-directory --source-id ch_swisstopo
```

The workflow sets the compiler and ccache environment before configuring:

```sh
export CC="ccache clang"
export CXX="ccache clang++"
export CCACHE_STATIC_PREFIX="/usr/local"
export MAKEJOBS="-j2"
```

Then it runs:

```sh
find . -name "*.pl" | xargs sed -i '' 's|/usr/bin/perl|/usr/bin/env perl|'

./autogen.sh
./configure \
  PKG_CONFIG=/usr/local/bin/pkgconf \
  CFLAGS="-isystem /usr/local/include -Wall -fno-omit-frame-pointer -Werror" \
  LDFLAGS="-L/usr/local/lib" \
  --with-libiconv-prefix=/usr/local \
  --without-gui \
  --with-topology \
  --without-raster \
  --with-sfcgal=/usr/local/bin/sfcgal-config \
  --with-protobuf

service postgresql oneinitdb
service postgresql onestart
su postgres -c "createuser -s $(whoami)"

gmake ${MAKEJOBS}
gmake ${MAKEJOBS} install
gmake ${MAKEJOBS} check RUNTESTFLAGS="-v --extension --dumprestore"
service postgresql onestop
```

If local behavior differs from CI, first compare the FreeBSD release, package
repository, PostgreSQL package major version, `pkgconf` path, and configure
flags against `.github/workflows/ci-freebsd.yml`.
