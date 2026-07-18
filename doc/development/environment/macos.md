---
title: "macOS Development Environment"
date: 2026-06-27
weight: 20
geekdocHidden: false
---

The current macOS pull-request build is defined in
`.github/workflows/ci-macos.yml`. Use that workflow as the source of truth when
checking platform-specific dependencies or flags.

At the time of this note, the job uses `macos-latest`, Homebrew, PostgreSQL 17,
and clang through ccache. It builds the topology, SFCGAL, and protobuf paths,
but disables raster, GUI, and interrupt tests:

```sh
brew update
brew install \
  autoconf automake libtool pkg-config \
  ccache \
  gdal geos icu4c json-c libpq libxml2 \
  proj protobuf-c sfcgal cunit \
  docbook docbook-xsl \
  postgresql@17 gettext
brew link --force gettext
```

The workflow exports Homebrew include and library paths before configuring:

```sh
export HOMEBREW_PREFIX=/opt/homebrew
export PATH="${HOMEBREW_PREFIX}/opt/postgresql@17/bin:${HOMEBREW_PREFIX}/bin:${HOMEBREW_PREFIX}/sbin:${HOMEBREW_PREFIX}/opt/ccache/libexec:${PATH}"
export PGCONFIG="${HOMEBREW_PREFIX}/opt/postgresql@17/bin/pg_config"
export CFLAGS="-I${HOMEBREW_PREFIX}/opt/gettext/include -I${HOMEBREW_PREFIX}/opt/postgresql@17/include -I${HOMEBREW_PREFIX}/include -Wno-nullability-completeness"
export LDFLAGS="-L${HOMEBREW_PREFIX}/opt/gettext/lib -L${HOMEBREW_PREFIX}/opt/postgresql@17/lib"
export CXXFLAGS="-std=c++17"
export CC="ccache clang"
export CXX="ccache clang++"
```

Then it runs:

```sh
./autogen.sh
./configure \
  --without-gui \
  --without-interrupt-tests \
  --with-topology \
  --without-raster \
  --with-sfcgal \
  --with-protobuf \
  --with-pgconfig="${PGCONFIG}"

brew services start postgresql@17
make -j"$(sysctl -n hw.logicalcpu)"
sudo make install
make -j"$(sysctl -n hw.logicalcpu)" check RUNTESTFLAGS="-v --extension --dumprestore"
brew services stop postgresql@17
```

If local behavior differs from CI, first compare the local Homebrew package
versions, `HOMEBREW_PREFIX`, PostgreSQL major version, and configure flags
against `.github/workflows/ci-macos.yml`.
