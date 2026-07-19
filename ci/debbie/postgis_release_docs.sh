#!/usr/bin/env bash

set -euo pipefail

export PG_VER=18
# export PGPORT=8442
export OS_BUILD=64
#this is passed in via postgis_make_dist.sh via jenkins
#export reference=
export PROJECTS=/var/lib/jenkins/workspace
export GEOS_VER=3.14
export GDAL_VER=3.13
export PROJ_VER=9.8.1
export SFCGAL_VER=2.3.0
export CGAL_VER=6.2
export WEB_DIR=${WEB_DIR:-/var/www/postgis_stuff}
export PGPATH=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}
export GEOSPATH=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}
export GDALPATH=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}
export PROJPATH=${PROJECTS}/proj/rel-${PROJ_VER}w${OS_BUILD}
export SFCGALPATH=${PROJECTS}/sfcgal/rel-sfcgal-${SFCGAL_VER}w${OS_BUILD}
export CGALPATH=${PROJECTS}/sfcgal/rel-cgal-${CGAL_VER}w${OS_BUILD}
export PATH="${PGPATH}/bin:${GEOSPATH}/bin:${GDALPATH}/bin:${PROJPATH}/bin:${SFCGALPATH}/bin:$PATH"
export LD_LIBRARY_PATH="${SFCGALPATH}/lib:${PROJPATH}/lib:${GDALPATH}/lib:${GEOSPATH}/lib:${PGPATH}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export PKG_CONFIG_PATH="${SFCGALPATH}/lib/pkgconfig:${PROJPATH}/lib/pkgconfig:${GDALPATH}/lib/pkgconfig:${GEOSPATH}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"
export PROJ_DATA=${PROJPATH}/share/proj
export GDAL_DATA=${GDALPATH}/share/gdal

PG_CONFIG=${PGPATH}/bin/pg_config
GEOS_CONFIG=${GEOSPATH}/bin/geos-config
GDAL_CONFIG=${GDALPATH}/bin/gdal-config
SFCGAL_CONFIG=${SFCGALPATH}/bin/sfcgal-config

require_executable()
{
  if ! test -x "$1"; then
    echo "Required release-docs dependency is missing: $1" >&2
    exit 1
  fi
}

require_minimum_version()
{
  dependency=$1
  actual=$2
  minimum=$3
  numeric_actual=$(printf '%s\n' "${actual}" | sed 's/[^0-9.].*$//')
  if test -z "${numeric_actual}"; then
    echo "Could not parse ${dependency} version from: ${actual}" >&2
    exit 1
  fi
  if ! dpkg --compare-versions "${numeric_actual}" ge "${minimum}"; then
    echo "${dependency} ${actual} is older than required ${minimum}" >&2
    exit 1
  fi
  printf '%-12s %s (minimum %s)\n' "${dependency}:" "${actual}" "${minimum}"
}

for dependency_tool in \
  "${PG_CONFIG}" \
  "${GEOS_CONFIG}" \
  "${GDAL_CONFIG}" \
  "${PROJPATH}/bin/projinfo" \
  "${SFCGAL_CONFIG}"
do
  require_executable "${dependency_tool}"
done
test -r "${CGALPATH}/include/CGAL/version.h" || {
  echo "Required CGAL version header is missing under ${CGALPATH}" >&2
  exit 1
}

pg_version=$("${PG_CONFIG}" --version | awk '{print $2}')
geos_version=$("${GEOS_CONFIG}" --version)
gdal_version=$("${GDAL_CONFIG}" --version)
proj_version=$(PKG_CONFIG_PATH="${PROJPATH}/lib/pkgconfig" pkg-config --modversion proj)
sfcgal_version=$("${SFCGAL_CONFIG}" --version)
cgal_version=$(awk '/^#define CGAL_VERSION / { print $3; exit }' "${CGALPATH}/include/CGAL/version.h")

echo "Release documentation dependency contract:"
require_minimum_version PostgreSQL "${pg_version}" 18.4
require_minimum_version GEOS "${geos_version}" 3.14.1
require_minimum_version GDAL "${gdal_version}" 3.13.1
require_minimum_version PROJ "${proj_version}" 9.8.1
require_minimum_version SFCGAL "${sfcgal_version}" 2.3.0
require_minimum_version CGAL "${cgal_version}" 6.2

release_docs_lock=${RELEASE_DOCS_LOCK:-/tmp/postgis-release-docs.lock}
exec 9>"${release_docs_lock}"
if ! flock -n 9; then
  echo "Another release documentation build holds ${release_docs_lock}" >&2
  exit 1
fi

LOCALIZED_HTML_JOBS=${LOCALIZED_HTML_JOBS:-$(nproc)}
case "${LOCALIZED_HTML_JOBS}" in
  ''|*[!0-9]*|0)
    echo "LOCALIZED_HTML_JOBS must be a positive integer" >&2
    exit 1
    ;;
esac

release_docs_tmpdir=
release_docs_pgdata=

cleanup_release_docs_pg()
{
  status=$?
  if test "${status}" -ne 0 && test -n "${release_docs_tmpdir}" && test -f "${release_docs_tmpdir}/postgresql.log"; then
    tail -n 100 "${release_docs_tmpdir}/postgresql.log"
  fi
  if test -n "${release_docs_pgdata}" && test -f "${release_docs_pgdata}/postmaster.pid"; then
    "${PGPATH}/bin/pg_ctl" -D "${release_docs_pgdata}" -m fast -w stop || true
  fi
  if test -n "${release_docs_tmpdir}"; then
    rm -rf -- "${release_docs_tmpdir}"
  fi
  return "${status}"
}

start_release_docs_pg()
{
  tmp_root=${TMPDIR:-/tmp}
  mkdir -p "${tmp_root}"
  release_docs_tmpdir=$(mktemp -d "${tmp_root}/postgis-release-docs.XXXXXX")
  release_docs_pgdata="${release_docs_tmpdir}/data"
  release_docs_pghost="${release_docs_tmpdir}/socket"
  mkdir "${release_docs_pghost}"

  release_docs_initdb_options=(-D "${release_docs_pgdata}" -A trust --no-sync)
  if "${PGPATH}/bin/initdb" --help | grep -q -- '--no-data-checksums'; then
    release_docs_initdb_options+=(--no-data-checksums)
  fi
  "${PGPATH}/bin/initdb" "${release_docs_initdb_options[@]}"
  export PGHOST="${release_docs_pghost}"
  export PGPORT="${PGPORT:-8447}"
  PGUSER=$(id -un)
  export PGUSER
  release_docs_pgopts="-F -h '' -k ${PGHOST} -p ${PGPORT}"
  release_docs_pgopts="${release_docs_pgopts} -c fsync=off -c synchronous_commit=off"
  release_docs_pgopts="${release_docs_pgopts} -c full_page_writes=off -c wal_level=minimal"
  release_docs_pgopts="${release_docs_pgopts} -c max_wal_senders=0 -c autovacuum=off"
  "${PGPATH}/bin/pg_ctl" \
    -D "${release_docs_pgdata}" \
    -l "${release_docs_tmpdir}/postgresql.log" \
    -o "${release_docs_pgopts}" \
    -w start
}

trap cleanup_release_docs_pg EXIT

./autogen.sh



POSTGIS_MAJOR_VERSION=$(awk -F= '$1 == "POSTGIS_MAJOR_VERSION" { print $2 }' Version.config)
POSTGIS_MINOR_VERSION=$(awk -F= '$1 == "POSTGIS_MINOR_VERSION" { print $2 }' Version.config)
POSTGIS_MICRO_VERSION=$(awk -F= '$1 == "POSTGIS_MICRO_VERSION" { print $2 }' Version.config)

# TODO: take base as env variable ?
HTML_DIR=${HTML_DIR:-/var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}}
STUFF_DIR=${STUFF_DIR:-/var/www/postgis_stuff}


echo "$PATH"

#sh autogen.sh

if [ -f GNUMakefile ]; then
  make distclean
fi

#  --with-gdalconfig=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/bin/gdal-config
#  --without-raster

CPPFLAGS="-I${PGPATH}/include" \
LDFLAGS="-L${SFCGALPATH}/lib -L${PROJPATH}/lib -L${GDALPATH}/lib -L${GEOSPATH}/lib -L${PGPATH}/lib" ./configure \
  --with-pgconfig=${PG_CONFIG} \
  --with-geosconfig=${GEOS_CONFIG} \
  --with-gdalconfig=${GDAL_CONFIG} \
  --with-projdir=${PROJPATH} \
  --with-sfcgal=${SFCGAL_CONFIG} \
  --with-raster \
  --htmldir "${HTML_DIR}" \
  --docdir "${STUFF_DIR}"
make clean

# The rendered manual now builds visual examples through a temporary PostGIS
# database.  The distribution job does not otherwise require a running server,
# so keep this database private to the documentation build.
start_release_docs_pg

# generating postgis_revision.h in case hasn't been generated
if test -f utils/repo_revision.pl; then
	echo "Generating postgis_revision.h"
	perl utils/repo_revision.pl
fi
VREV=$(awk '{print $3}' postgis_revision.h)
export VREV
echo "VREV is ${VREV}"
make -C regress visual-examples
cd doc


#sed -e "s:</title>:</title><subtitle><subscript>SVN Revision (<emphasis>${POSTGIS_SVN_REVISION}</emphasis>)</subscript></subtitle>:" postgis.xml.orig > postgis.xml

echo "Micro: $POSTGIS_MICRO_VERSION"
#inject a development time stamp if we are in development branch
# TODO: provide support for an env variable to do this ?
if [[ "$POSTGIS_MICRO_VERSION" == *"dev"* ]]; then
  cp postgis.xml postgis.xml.orig #we for dev will inject stuff into file, so backup original
  GIT_TIMESTAMP=$(git log -1 --pretty=format:%ct)
  GIT_TIMESTAMP=$(date -d @"${GIT_TIMESTAMP}") #convert to UTC date
  export GIT_TIMESTAMP
  echo "GIT_TIMESTAMP: ${GIT_TIMESTAMP}"
  export part_old="</title>"
  export part_new="</title><subtitle>DEV ($GIT_TIMESTAMP rev. $VREV)</subtitle>"
  sed -i 's,'"$part_old"','"$part_new"',' postgis.xml
fi

make cheatsheets
make -e chunked-html # TODO: do we really want this too in the doc-html-*.tar.gz package ?
make -j"${LOCALIZED_HTML_JOBS}" html-localized # TODO: do we really want this too in the doc-html-*.tar.gz package ?

package="doc-html-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}.tar.gz"
tar -czf "$package" --exclude='static' --exclude='wkt' --exclude '*.o' html
cp "${package}" "${STUFF_DIR}"

# Install all html
make html-assets-install
make html-install
make html-install-localized
make chunked-html-install
make -j"${LOCALIZED_HTML_JOBS}" chunked-html-install-localized
make cheatsheet-install
make cheatsheet-install-localized

# Strip out the "postgis-" suffix
# from chunked html directories, backing
# up any previous target directory
while IFS= read -r f; do
  nf=${f/postgis-/}
  if test -e "${nf}"; then
    rm -rf -- "${nf}.old"
    mv -v "${nf}" "${nf}.old"
  fi
  mv -v "${f}" "${nf}"
done < <(
  find "${HTML_DIR}" -maxdepth 1 -mindepth 1 -type d -name 'postgis-*'
)


# Build and install pdf and epub
make pdf-install # || : survive failure
#make epub-install # || : survive failure

# build japanese, french, german, chinese, korean pdf
make -C ../doc/po/ja local-pdf-install
make -C ../doc/po/fr local-pdf-install
make -C ../doc/po/de local-pdf-install
make -C ../doc/po/zh_Hans local-pdf-install
make -C ../doc/po/ko_KR local-pdf-install
make -C ../doc/po/sv local-pdf-install

# build comments files
make -C ../doc/po/ja comments
make -C ../doc/po/fr comments
make -C ../doc/po/de comments
make -C ../doc/po/zh_Hans comments
make -C ../doc/po/ko_KR comments
make -C ../doc/po/sv comments
# Build and install localized pdf and epub
# make pdf-install-localized # || : survive failures
#make epub-install-localized # || : survive failures

if [[ "$POSTGIS_MICRO_VERSION" == *"dev"* ]]; then

  # rename the files without the micro if it's a development branch
  # to avoid proliferation of these packages
  for f in "${STUFF_DIR}"/*-"${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}"-*
  do
    test -e "${f}" || continue
    newname=${f/.${POSTGIS_MICRO_VERSION}/}
    mv -v "${f}" "${newname}"
  done

  # restore the backedup xml file
  cp postgis.xml.orig postgis.xml

fi
