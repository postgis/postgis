#!/usr/bin/env bash

set -Eeuo pipefail

TARGET=${TARGET:-x86_64-w64-mingw32}
PREFIX=${PREFIX:-/opt/postgis-mingw}
BUILD_DIR=${BUILD_DIR:-build-mingw-wine}
REPO_ROOT=$(pwd)
TMP_ROOT=${TMP_ROOT:-${REPO_ROOT}/.tmp/mingw-wine}
WORKDIR=${WORKDIR:-${TMP_ROOT}/work}
LOG_DIR=${LOG_DIR:-${TMP_ROOT}/logs}

export DEBIAN_FRONTEND=noninteractive
export PATH="${PREFIX}/bin:/usr/${TARGET}/bin:${PATH}"
export TMPDIR="${TMP_ROOT}/tmp"
export CC=${CC:-${TARGET}-gcc-posix}
export CXX=${CXX:-${TARGET}-g++-posix}
export AR=${AR:-${TARGET}-ar}
export RANLIB=${RANLIB:-${TARGET}-ranlib}
export WINDRES=${WINDRES:-${TARGET}-windres}
export PKG_CONFIG_LIBDIR="${PREFIX}/lib/pkgconfig"
export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig"
export WINEDEBUG=${WINEDEBUG:--all}
export WINEPREFIX=${WINEPREFIX:-/tmp/postgis-wine}
export WINEARCH=${WINEARCH:-win64}
WINE_TMPDIR=${WINE_TMPDIR:-/tmp}
WINE=${WINE:-wine}

log_tail()
{
	local status=$1
	local file=$2
	echo "----- ${file} tail -----" >&2
	tail -160 "${file}" >&2 || true
	exit "${status}"
}

run_logged()
{
	local log=$1
	shift
	"$@" >"${log}" 2>&1 || log_tail "$?" "${log}"
}

install_packages()
{
	apt-get update >/dev/null
	apt-get install -y --no-install-recommends \
		ca-certificates \
		curl \
		dpkg-dev \
		build-essential \
		autoconf \
		automake \
		libtool \
		pkg-config \
		cmake \
		ninja-build \
		bison \
		flex \
		gettext \
		perl \
		sqlite3 \
		mingw-w64 \
		gcc-mingw-w64-x86-64-posix \
		g++-mingw-w64-x86-64-posix \
		wine \
		wine64 \
		xauth \
		xvfb \
		diffutils >/dev/null

	cat >/etc/apt/sources.list.d/postgis-mingw-wine-src.list <<'EOF'
deb-src http://deb.debian.org/debian trixie main
deb-src http://deb.debian.org/debian-security trixie-security main
deb-src http://deb.debian.org/debian trixie-updates main
EOF
	apt-get update >/dev/null
}

fetch_sources()
{
	rm -rf "${WORKDIR}"
	mkdir -p "${WORKDIR}/src" "${PREFIX}/include" "${PREFIX}/lib/pkgconfig"
	cd "${WORKDIR}/src"
	apt-get source cunit geos proj libxml2 postgresql-17 >/dev/null
	curl -fsSLO https://www.sqlite.org/2024/sqlite-autoconf-3460100.tar.gz
	curl -fsSLO https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.18.tar.gz
	curl -fsSLO https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz
	tar xf sqlite-autoconf-3460100.tar.gz
	tar xf libiconv-1.18.tar.gz
	tar xf gmp-6.3.0.tar.xz
}

build_sqlite()
{
	cd "${WORKDIR}/src/sqlite-autoconf-3460100"
	${CC} -O2 -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o sqlite3.o
	cp sqlite3.h sqlite3ext.h "${PREFIX}/include/"
	${AR} rcs "${PREFIX}/lib/libsqlite3.a" sqlite3.o
	cat >"${PREFIX}/lib/pkgconfig/sqlite3.pc" <<EOF
prefix=${PREFIX}
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include
Name: SQLite
Description: SQL database engine
Version: 3.46.1
Libs: -L\${libdir} -lsqlite3
Cflags: -I\${includedir}
EOF
}

build_cunit()
{
	cd "${WORKDIR}/src/cunit-2.1-3-dfsg"
	autoreconf -fi >"${LOG_DIR}/cunit.autoreconf.log" 2>&1 || true
	run_logged "${LOG_DIR}/cunit.configure.log" ./configure \
		--host="${TARGET}" \
		--prefix="${PREFIX}" \
		--disable-shared \
		--enable-static
	run_logged "${LOG_DIR}/cunit.make.log" make -j"$(nproc)"
	run_logged "${LOG_DIR}/cunit.install.log" make install
}

build_libiconv()
{
	cd "${WORKDIR}/src/libiconv-1.18"
	run_logged "${LOG_DIR}/libiconv.configure.log" ./configure \
		--host="${TARGET}" \
		--prefix="${PREFIX}" \
		--disable-shared \
		--enable-static
	run_logged "${LOG_DIR}/libiconv.make.log" make -j"$(nproc)"
	run_logged "${LOG_DIR}/libiconv.install.log" make install
}

build_gmp()
{
	cd "${WORKDIR}/src/gmp-6.3.0"
	run_logged "${LOG_DIR}/gmp.configure.log" env CC_FOR_BUILD="/usr/bin/gcc -B/usr/bin/" CPP_FOR_BUILD="/usr/bin/gcc -E" ./configure \
		--host="${TARGET}" \
		--prefix="${PREFIX}" \
		--disable-shared \
		--enable-static
	run_logged "${LOG_DIR}/gmp.make.log" make -j"$(nproc)"
	run_logged "${LOG_DIR}/gmp.install.log" make install
}

build_geos()
{
	run_logged "${LOG_DIR}/geos.configure.log" cmake \
		-S "${WORKDIR}/src/geos-3.13.1" \
		-B "${WORKDIR}/build-geos" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_C_COMPILER="${CC}" \
		-DCMAKE_CXX_COMPILER="${CXX}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_TESTING=OFF \
		-DGEOS_BUILD_DEVELOPER=OFF
	run_logged "${LOG_DIR}/geos.make.log" cmake --build "${WORKDIR}/build-geos" -j"$(nproc)"
	run_logged "${LOG_DIR}/geos.install.log" cmake --install "${WORKDIR}/build-geos"
	cat >"${PREFIX}/bin/${TARGET}-geos-config" <<EOF
#!/bin/sh
case "\$1" in
  --version) echo "3.13.1" ;;
  --cflags) echo "-I${PREFIX}/include" ;;
  --clibs|--libs) echo "-L${PREFIX}/lib -lgeos_c -lgeos -lstdc++" ;;
  *) echo "unsupported geos-config option: \$1" >&2; exit 1 ;;
esac
EOF
	chmod +x "${PREFIX}/bin/${TARGET}-geos-config"
}

build_libxml2()
{
	cd "${WORKDIR}/src/libxml2-2.12.7+dfsg+really2.9.14"
	run_logged "${LOG_DIR}/libxml2.autoreconf.log" autoreconf -fi
	run_logged "${LOG_DIR}/libxml2.configure.log" ./configure \
		--host="${TARGET}" \
		--prefix="${PREFIX}" \
		--disable-shared \
		--enable-static \
		--without-python \
		--without-threads \
		--without-zlib \
		--without-lzma \
		--without-iconv \
		--without-modules \
		--without-ftp \
		--without-http
	run_logged "${LOG_DIR}/libxml2.make.log" make -j"$(nproc)"
	run_logged "${LOG_DIR}/libxml2.install.log" make install
}

build_libpq()
{
	cd "${WORKDIR}/src/postgresql-17-17.10"
	run_logged "${LOG_DIR}/postgresql.configure.log" ./configure \
		--host="${TARGET}" \
		--prefix="${PREFIX}" \
		--without-readline \
		--without-zlib \
		--without-icu
	run_logged "${LOG_DIR}/postgresql.libpq.make.log" make -C src/interfaces/libpq -j"$(nproc)"
	run_logged "${LOG_DIR}/postgresql.include.install.log" make -C src/include install
	run_logged "${LOG_DIR}/postgresql.libpq.install.log" make -C src/interfaces/libpq install

	mkdir -p "${PREFIX}/lib/pgxs/src/makefiles" "${PREFIX}/bin"
	touch "${PREFIX}/lib/pgxs/src/makefiles/pgxs.mk"
	cat >"${PREFIX}/bin/${TARGET}-pg_config" <<EOF
#!/bin/sh
case "\$1" in
  --version) echo "PostgreSQL 17.10" ;;
  --pgxs) echo "${PREFIX}/lib/pgxs/src/makefiles/pgxs.mk" ;;
  --pkglibdir|--libdir) echo "${PREFIX}/lib" ;;
  --sharedir) echo "${PREFIX}/share" ;;
  --includedir) echo "${PREFIX}/include" ;;
  --includedir-server) echo "${PREFIX}/include/server" ;;
  --docdir) echo "${PREFIX}/doc" ;;
  --mandir) echo "${PREFIX}/man" ;;
  --localedir) echo "${PREFIX}/share/locale" ;;
  --bindir) echo "${PREFIX}/bin" ;;
  --cc) echo "${TARGET}-gcc" ;;
  --cflags) echo "-I${PREFIX}/include" ;;
  *) echo "unsupported pg_config option: \$1" >&2; exit 1 ;;
esac
EOF
	chmod +x "${PREFIX}/bin/${TARGET}-pg_config"
}

build_proj()
{
	run_logged "${LOG_DIR}/proj.configure.log" cmake \
		-S "${WORKDIR}/src/proj-9.6.0" \
		-B "${WORKDIR}/build-proj" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_C_COMPILER="${CC}" \
		-DCMAKE_CXX_COMPILER="${CXX}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DSQLite3_INCLUDE_DIR="${PREFIX}/include" \
		-DSQLite3_LIBRARY="${PREFIX}/lib/libsqlite3.a" \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_TESTING=OFF \
		-DBUILD_APPS=OFF \
		-DENABLE_CURL=OFF \
		-DENABLE_TIFF=OFF \
		-DENABLE_IPO=OFF
	run_logged "${LOG_DIR}/proj.make.log" cmake --build "${WORKDIR}/build-proj" -j"$(nproc)"
	run_logged "${LOG_DIR}/proj.install.log" cmake --install "${WORKDIR}/build-proj"
}

configure_postgis()
{
	rm -rf "${BUILD_DIR}"
	mkdir "${BUILD_DIR}"
	cd "${BUILD_DIR}"

	export GEOS_CFLAGS="-I${PREFIX}/include"
	export GEOS_LIBS="-L${PREFIX}/lib -lgeos_c -lgeos -lstdc++"
	export PROJ_CFLAGS="-I${PREFIX}/include"
	export PROJ_LIBS="-L${PREFIX}/lib -lproj -lsqlite3 -lstdc++ -lws2_32 -lbcrypt -lole32 -lshell32"

	run_logged "${LOG_DIR}/postgis.configure.log" ../configure \
		--host="${TARGET}" \
		--with-pgconfig="${PREFIX}/bin/${TARGET}-pg_config" \
		--with-geosconfig="${PREFIX}/bin/${TARGET}-geos-config" \
		--with-xml2config="${PREFIX}/bin/xml2-config" \
		--with-libiconv="${PREFIX}" \
		--without-protobuf \
		--without-raster \
		--disable-spellcheck-tests \
		CFLAGS="-O2 -Wall" \
		LDFLAGS="-L${PREFIX}/lib -static -static-libgcc -static-libstdc++"

	echo "CONFIGURE_SUMMARY_BEGIN"
	sed -n '/^  PostGIS is now configured/,/^$/p' "${LOG_DIR}/postgis.configure.log"
	echo "CONFIGURE_SUMMARY_END"
}

build_postgis_targets()
{
	run_logged "${LOG_DIR}/postgis.liblwgeom.make.log" make -j"$(nproc)" -C liblwgeom
	run_logged "${LOG_DIR}/postgis.cunit.make.log" make -j"$(nproc)" -C liblwgeom/cunit cu_tester
	run_logged "${LOG_DIR}/postgis.loader.make.log" make -j"$(nproc)" -C loader shp2pgsql.exe pgsql2shp.exe

	echo "BUILD_TAIL_BEGIN"
	tail -40 "${LOG_DIR}/postgis.liblwgeom.make.log"
	tail -40 "${LOG_DIR}/postgis.cunit.make.log"
	tail -60 "${LOG_DIR}/postgis.loader.make.log"
	echo "BUILD_TAIL_END"
}

copy_mingw_runtime_dlls()
{
	local dll
	local path
	for dll in libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll LIBPQ.dll libpq.dll; do
		path=$(find "/usr/lib/gcc/${TARGET}" "/usr/${TARGET}" "${PREFIX}" -name "${dll}" -print -quit 2>/dev/null || true)
		if test -n "${path}"; then
			cp "${path}" liblwgeom/cunit/
			cp "${path}" loader/
		fi
	done
}

run_wine_checks()
{
	local cunit_runner
	cunit_runner=liblwgeom/cunit/cu_tester.exe
	if test ! -f "${cunit_runner}"; then
		cunit_runner=liblwgeom/cunit/cu_tester
	fi

	case "${WINEPREFIX}" in
		"${TMP_ROOT}"/*|/tmp/postgis-wine) rm -rf "${WINEPREFIX}" ;;
	esac
	copy_mingw_runtime_dlls
	env -u XDG_RUNTIME_DIR TMPDIR="${WINE_TMPDIR}" xvfb-run -a "${WINE}" "${cunit_runner}" > "${LOG_DIR}/postgis.cunit.wine.log" 2>&1 \
		|| log_tail "$?" "${LOG_DIR}/postgis.cunit.wine.log"
	echo "CUNIT_WINE_BEGIN"
	tail -80 "${LOG_DIR}/postgis.cunit.wine.log"
	echo "CUNIT_WINE_END"

	env -u XDG_RUNTIME_DIR TMPDIR="${WINE_TMPDIR}" xvfb-run -a "${WINE}" loader/shp2pgsql.exe -s 4326 ../regress/loader/Point public.point \
		> "${LOG_DIR}/point.sql" 2>"${LOG_DIR}/shp2pgsql.stderr" \
		|| log_tail "$?" "${LOG_DIR}/shp2pgsql.stderr"
	grep '^INSERT INTO "public"."point"' "${LOG_DIR}/point.sql" | tr -d '\r' > "${LOG_DIR}/point.inserts"
	cat > "${LOG_DIR}/point.inserts.expected" <<'EOF'
INSERT INTO "public"."point" (geom) VALUES ('0101000020E61000000000000000000000000000000000F03F');
INSERT INTO "public"."point" (geom) VALUES ('0101000020E61000000000000000002240000000000000F0BF');
INSERT INTO "public"."point" (geom) VALUES ('0101000020E61000000000000000002240000000000000F0BF');
EOF
	diff -u "${LOG_DIR}/point.inserts.expected" "${LOG_DIR}/point.inserts"

	env -u XDG_RUNTIME_DIR TMPDIR="${WINE_TMPDIR}" xvfb-run -a "${WINE}" loader/pgsql2shp.exe -? > "${LOG_DIR}/pgsql2shp.usage" 2>&1 || true
	grep -F "USAGE: pgsql2shp" "${LOG_DIR}/pgsql2shp.usage" >/dev/null

	echo "LOADER_CHECK_BEGIN"
	sed -n '1,16p' "${LOG_DIR}/point.sql"
	diff -u "${LOG_DIR}/point.inserts.expected" "${LOG_DIR}/point.inserts"
	sed -n '1,8p' "${LOG_DIR}/pgsql2shp.usage"
	echo "LOADER_CHECK_END"
}

main()
{
	local started
	started=$(date +%s)

	mkdir -p "${TMPDIR}" "${LOG_DIR}"
	install_packages
	fetch_sources
	build_sqlite
	build_libiconv
	build_gmp
	build_cunit
	build_geos
	build_libxml2
	build_libpq
	build_proj

	cd "${REPO_ROOT}"
	./autogen.sh >"${LOG_DIR}/postgis.autogen.log" 2>&1 || log_tail "$?" "${LOG_DIR}/postgis.autogen.log"
	configure_postgis
	build_postgis_targets
	run_wine_checks

	echo "WALL_CLOCK_SECONDS=$(( $(date +%s) - started ))"
}

main "$@"
