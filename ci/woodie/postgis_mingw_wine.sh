#!/usr/bin/env bash

set -Eeuo pipefail

TARGET=${TARGET:-x86_64-w64-mingw32}
BUILD_DIR=${BUILD_DIR:-.tmp/build-mingw-wine}
REPO_ROOT=$(pwd)
TMP_ROOT=${TMP_ROOT:-${REPO_ROOT}/.tmp/mingw-wine}
PREFIX=${PREFIX:-${TMP_ROOT}/prefix}
WORKDIR=${WORKDIR:-${TMP_ROOT}/work}
LOG_DIR=${LOG_DIR:-${TMP_ROOT}/logs}
DOWNLOAD_DIR=${DOWNLOAD_DIR:-${TMP_ROOT}/downloads}
PGWIN_URL=${PGWIN_URL:-https://get.enterprisedb.com/postgresql/postgresql-17.10-1-windows-x64-binaries.zip}
VCREDIST_URL=${VCREDIST_URL:-https://aka.ms/vs/17/release/vc_redist.x64.exe}
INSTALL_VC_RUNTIME=${INSTALL_VC_RUNTIME:-0}
PGWIN_ROOT=${PGWIN_ROOT:-${TMP_ROOT}/postgresql-windows/pgsql}
PGDATA=${PGDATA:-${TMP_ROOT}/pgdata}
DEFAULT_WINEPREFIX=${WINEPREFIX:-${TMP_ROOT}/wine-prefix}
PGWIN_RUN_ROOT=${PGWIN_RUN_ROOT:-${DEFAULT_WINEPREFIX}/drive_c/pgsql}
PGDATA_RUN=${PGDATA_RUN:-${DEFAULT_WINEPREFIX}/drive_c/pgdata}
PGHOST=${PGHOST:-127.0.0.1}
PGPORT=${PGPORT:-55432}
PGUSER=${PGUSER:-postgres}
RUN_FROM=${RUN_FROM:-}
RUN_FROM_ACTIVE=

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
export WINEDLLOVERRIDES=${WINEDLLOVERRIDES:-mscoree,mshtml=}
export WINEPREFIX=${DEFAULT_WINEPREFIX}
export WINEARCH=${WINEARCH:-win64}
WINE_TMPDIR=${WINE_TMPDIR:-${TMP_ROOT}/wine-tmp}
WINE=${WINE:-wine}
XVFB_WINE=(env -u XDG_RUNTIME_DIR TMPDIR="${WINE_TMPDIR}" xvfb-run -a "${WINE}")
CURL=(curl --retry 5 --retry-delay 5 --retry-all-errors --connect-timeout 30 -fL)

phase_log()
{
	local message=$1
	echo "${message}" | tee -a "${LOG_DIR}/phase-times.log"
}

phase()
{
	local name=$1
	local started
	shift
	if test -n "${RUN_FROM}" && test -z "${RUN_FROM_ACTIVE}"; then
		if test "${name}" = "${RUN_FROM}"; then
			RUN_FROM_ACTIVE=1
		else
			phase_log "PHASE_SKIP ${name}"
			return 0
		fi
	fi
	started=$(date +%s)
	phase_log "PHASE_BEGIN ${name}"
	"$@"
	phase_log "PHASE_SECONDS ${name} $(( $(date +%s) - started ))"
}

src_dir()
{
	local pattern=$1
	local found
	found=$(find "${WORKDIR}/src" -maxdepth 1 -type d -name "${pattern}" -print | sort | head -1)
	if test -z "${found}"; then
		echo "source directory matching ${pattern} not found under ${WORKDIR}/src" >&2
		exit 1
	fi
	printf '%s\n' "${found}"
}

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

run_autogen()
{
	cd "${REPO_ROOT}"
	./autogen.sh >"${LOG_DIR}/postgis.autogen.log" 2>&1 || log_tail "$?" "${LOG_DIR}/postgis.autogen.log"
}

install_packages()
{
	dpkg --add-architecture i386
	apt-get update >/dev/null
	apt-get install -y --no-install-recommends \
		ca-certificates \
		curl \
		gpg \
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
		postgresql-client \
		unzip \
		mingw-w64 \
		gcc-mingw-w64-x86-64-posix \
		g++-mingw-w64-x86-64-posix \
		protobuf-c-compiler \
		protobuf-compiler \
		xauth \
		xvfb \
		diffutils >/dev/null

	install -d -m 755 /etc/apt/keyrings
	rm -f /etc/apt/keyrings/winehq-archive.key
	"${CURL[@]}" https://dl.winehq.org/wine-builds/winehq.key \
		| gpg --dearmor -o /etc/apt/keyrings/winehq-archive.key
	cat >/etc/apt/sources.list.d/winehq.list <<'EOF'
deb [signed-by=/etc/apt/keyrings/winehq-archive.key] https://dl.winehq.org/wine-builds/debian/ bookworm main
EOF
	cat >/etc/apt/sources.list.d/postgis-mingw-wine-src.list <<'EOF'
deb-src http://deb.debian.org/debian trixie main
deb-src http://deb.debian.org/debian-security trixie-security main
deb-src http://deb.debian.org/debian trixie-updates main
EOF
	apt-get update >/dev/null
	apt-get install -y --install-recommends winehq-stable >/dev/null
}

fetch_sources()
{
	rm -rf "${WORKDIR}"
	mkdir -p "${WORKDIR}/src" "${PREFIX}/include" "${PREFIX}/lib/pkgconfig" "${DOWNLOAD_DIR}"
	cd "${WORKDIR}/src"
	apt-get source cunit geos proj libxml2 postgresql-17 zlib libjpeg-turbo libpng1.6 tiff libgeotiff gdal json-c protobuf-c >/dev/null
	"${CURL[@]}" -O https://www.sqlite.org/2024/sqlite-autoconf-3460100.tar.gz
	"${CURL[@]}" -O https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.18.tar.gz
	"${CURL[@]}" -O https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz
	tar xf sqlite-autoconf-3460100.tar.gz
	tar xf libiconv-1.18.tar.gz
	tar xf gmp-6.3.0.tar.xz
}

build_sqlite()
{
	cd "${WORKDIR}/src/sqlite-autoconf-3460100"
	${CC} -O2 -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o sqlite3.o
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

build_zlib()
{
	cd "$(src_dir 'zlib-*')"
	run_logged "${LOG_DIR}/zlib.configure.log" env CHOST="${TARGET}" CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" ./configure \
		--static \
		--prefix="${PREFIX}"
	run_logged "${LOG_DIR}/zlib.make.log" make -j"$(nproc)"
	run_logged "${LOG_DIR}/zlib.install.log" make install
}

build_libjpeg()
{
	rm -rf "${WORKDIR}/build-libjpeg"
	run_logged "${LOG_DIR}/libjpeg.configure.log" cmake \
		-S "$(src_dir 'libjpeg-turbo-*')" \
		-B "${WORKDIR}/build-libjpeg" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DCMAKE_C_COMPILER="${CC}" \
		-DCMAKE_RC_COMPILER="${WINDRES}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_BUILD_TYPE=Release \
		-DENABLE_SHARED=OFF \
		-DENABLE_STATIC=ON \
		-DWITH_TURBOJPEG=OFF \
		-DWITH_JAVA=OFF
	run_logged "${LOG_DIR}/libjpeg.make.log" cmake --build "${WORKDIR}/build-libjpeg" -j"$(nproc)"
	run_logged "${LOG_DIR}/libjpeg.install.log" cmake --install "${WORKDIR}/build-libjpeg"
}

build_libpng()
{
	rm -rf "${WORKDIR}/build-libpng"
	run_logged "${LOG_DIR}/libpng.configure.log" cmake \
		-S "$(src_dir 'libpng1.6-*')" \
		-B "${WORKDIR}/build-libpng" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DCMAKE_C_COMPILER="${CC}" \
		-DCMAKE_RC_COMPILER="${WINDRES}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DPNG_SHARED=OFF \
		-DPNG_STATIC=ON \
		-DPNG_TESTS=OFF \
		-DZLIB_INCLUDE_DIR="${PREFIX}/include" \
		-DZLIB_LIBRARY="${PREFIX}/lib/libz.a"
	run_logged "${LOG_DIR}/libpng.make.log" cmake --build "${WORKDIR}/build-libpng" -j"$(nproc)"
	run_logged "${LOG_DIR}/libpng.install.log" cmake --install "${WORKDIR}/build-libpng"
}

build_libtiff()
{
	rm -rf "${WORKDIR}/build-libtiff"
	run_logged "${LOG_DIR}/libtiff.configure.log" cmake \
		-S "$(src_dir 'tiff-*')" \
		-B "${WORKDIR}/build-libtiff" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DCMAKE_C_COMPILER="${CC}" \
		-DCMAKE_CXX_COMPILER="${CXX}" \
		-DCMAKE_RC_COMPILER="${WINDRES}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DCMAKE_EXE_LINKER_FLAGS="-L${PREFIX}/lib -ljpeg -lz" \
		-DBUILD_SHARED_LIBS=OFF \
		-Dtiff-tools=OFF \
		-Dtiff-tests=OFF \
		-Dtiff-contrib=OFF \
		-Dtiff-docs=OFF \
		-Dzlib=ON \
		-Djpeg=ON \
		-Dlibdeflate=OFF \
		-Dlzma=OFF \
		-Dzstd=OFF \
		-Dwebp=OFF \
		-Djbig=OFF \
		-DZLIB_INCLUDE_DIR="${PREFIX}/include" \
		-DZLIB_LIBRARY="${PREFIX}/lib/libz.a" \
		-DJPEG_INCLUDE_DIR="${PREFIX}/include" \
		-DJPEG_LIBRARY="${PREFIX}/lib/libjpeg.a"
	run_logged "${LOG_DIR}/libtiff.make.log" cmake --build "${WORKDIR}/build-libtiff" -j"$(nproc)"
	run_logged "${LOG_DIR}/libtiff.install.log" cmake --install "${WORKDIR}/build-libtiff"
	rm -rf "${PREFIX}/lib/cmake/tiff"
}

build_libgeotiff()
{
	cd "$(src_dir 'libgeotiff-*')"
	run_logged "${LOG_DIR}/libgeotiff.configure.log" env \
		CPPFLAGS="-I${PREFIX}/include" \
		LDFLAGS="-L${PREFIX}/lib" \
		LIBS="-ltiff -ljpeg -lz -lproj -lsqlite3 -lstdc++ -lws2_32 -lbcrypt -lole32 -lshell32" \
		./configure \
			--host="${TARGET}" \
			--prefix="${PREFIX}" \
			--disable-shared \
			--enable-static \
			--with-libtiff="${PREFIX}" \
			--with-jpeg="${PREFIX}" \
			--with-zip="${PREFIX}" \
			--with-proj="${PREFIX}"
	run_logged "${LOG_DIR}/libgeotiff.make.log" make -j"$(nproc)"
	run_logged "${LOG_DIR}/libgeotiff.install.log" make install
}

build_json_c()
{
	rm -rf "${WORKDIR}/build-json-c"
	run_logged "${LOG_DIR}/json-c.configure.log" cmake \
		-S "$(src_dir 'json-c-*')" \
		-B "${WORKDIR}/build-json-c" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DCMAKE_C_COMPILER="${CC}" \
		-DCMAKE_RC_COMPILER="${WINDRES}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=OFF \
		-DDISABLE_WERROR=ON \
		-DDISABLE_THREAD_LOCAL_STORAGE=ON \
		-DBUILD_TESTING=OFF
	run_logged "${LOG_DIR}/json-c.make.log" cmake --build "${WORKDIR}/build-json-c" -j"$(nproc)"
	run_logged "${LOG_DIR}/json-c.install.log" cmake --install "${WORKDIR}/build-json-c"
}

build_protobuf_c()
{
	cd "$(src_dir 'protobuf-c-*')"
	run_logged "${LOG_DIR}/protobuf-c.configure.log" ./configure \
		--host="${TARGET}" \
		--prefix="${PREFIX}" \
		--disable-shared \
		--enable-static \
		--disable-protoc
	run_logged "${LOG_DIR}/protobuf-c.make.log" make -j"$(nproc)"
	run_logged "${LOG_DIR}/protobuf-c.install.log" make install
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
	rm -rf "${WORKDIR}/build-geos"
	run_logged "${LOG_DIR}/geos.configure.log" cmake \
		-S "${WORKDIR}/src/geos-3.13.1" \
		-B "${WORKDIR}/build-geos" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
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
	run_logged "${LOG_DIR}/postgresql.configure.log" env CPPFLAGS="-I${PREFIX}/include" LDFLAGS="-L${PREFIX}/lib" ./configure \
		--host="${TARGET}" \
		--prefix="${PREFIX}" \
		--without-readline \
		--with-zlib \
		--without-icu
	run_logged "${LOG_DIR}/postgresql.libpq.make.log" make -C src/interfaces/libpq -j"$(nproc)"
	run_logged "${LOG_DIR}/postgresql.backend.make.log" make -C src/backend libpostgres.a -j"$(nproc)"
	run_logged "${LOG_DIR}/postgresql.include.install.log" make -C src/include install
	run_logged "${LOG_DIR}/postgresql.libpq.install.log" make -C src/interfaces/libpq install
	cp src/backend/libpostgres.a "${PREFIX}/lib/"
	cp src/common/libpgcommon.a src/port/libpgport.a "${PREFIX}/lib/"
	mkdir -p "${PREFIX}/lib/pgxs/src/makefiles"
	cp src/Makefile.global src/Makefile.shlib src/nls-global.mk "${PREFIX}/lib/pgxs/src/"
	cp src/makefiles/Makefile.win32 "${PREFIX}/lib/pgxs/src/Makefile.port"
	cp src/makefiles/pgxs.mk "${PREFIX}/lib/pgxs/src/makefiles/"

	mkdir -p "${PREFIX}/bin"
	cat >"${PREFIX}/bin/${TARGET}-pg_config" <<EOF
#!/bin/sh
case "\$1" in
  --version) echo "PostgreSQL 17.10" ;;
  --pgxs) echo "${PREFIX}/lib/pgxs/src/makefiles/pgxs.mk" ;;
  --pkglibdir) echo "${PGWIN_ROOT}/lib" ;;
  --libdir) echo "${PREFIX}/lib" ;;
  --sharedir) echo "${PGWIN_ROOT}/share" ;;
  --includedir) echo "${PREFIX}/include" ;;
  --pkgincludedir) echo "${PREFIX}/include/postgresql" ;;
  --includedir-server) echo "${PREFIX}/include/postgresql/17/server" ;;
  --sysconfdir) echo "${PGWIN_ROOT}/etc" ;;
  --docdir) echo "${PREFIX}/doc" ;;
  --mandir) echo "${PREFIX}/man" ;;
  --localedir) echo "${PREFIX}/share/locale" ;;
  --bindir) echo "${PREFIX}/bin" ;;
  --cc) echo "${CC}" ;;
  --cflags) echo "-I${PREFIX}/include" ;;
  *) echo "unsupported pg_config option: \$1" >&2; exit 1 ;;
esac
EOF
	chmod +x "${PREFIX}/bin/${TARGET}-pg_config"
	cp "${PREFIX}/bin/${TARGET}-pg_config" "${PREFIX}/bin/pg_config"
}

build_proj()
{
	rm -rf "${WORKDIR}/build-proj"
	run_logged "${LOG_DIR}/proj.configure.log" cmake \
		-S "${WORKDIR}/src/proj-9.6.0" \
		-B "${WORKDIR}/build-proj" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
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

build_gdal()
{
	local libstdcxx
	libstdcxx=$("${CXX}" -print-file-name=libstdc++.a)

	rm -rf "${WORKDIR}/build-gdal"
	run_logged "${LOG_DIR}/gdal.configure.log" cmake \
		-S "$(src_dir 'gdal-*')" \
		-B "${WORKDIR}/build-gdal" \
		-G Ninja \
		-DCMAKE_SYSTEM_NAME=Windows \
		-DCMAKE_SYSTEM_PROCESSOR=x86_64 \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DCMAKE_C_COMPILER="${CC}" \
		-DCMAKE_CXX_COMPILER="${CXX}" \
		-DCMAKE_RC_COMPILER="${WINDRES}" \
		-DCMAKE_INSTALL_PREFIX="${PREFIX}" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_PREFIX_PATH="${PREFIX}" \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_APPS=OFF \
		-DBUILD_TESTING=OFF \
		-DGDAL_USE_EXTERNAL_LIBS=OFF \
		-DGDAL_USE_ZLIB=ON \
		-DGDAL_USE_TIFF=ON \
		-DGDAL_USE_GEOTIFF=ON \
		-DGDAL_USE_PNG=ON \
		-DGDAL_USE_JPEG=ON \
		-DGDAL_USE_SQLITE3=ON \
		-DGDAL_USE_PROJ=ON \
		-DGDAL_BUILD_OPTIONAL_DRIVERS=OFF \
		-DOGR_BUILD_OPTIONAL_DRIVERS=OFF \
		-DGDAL_ENABLE_DRIVER_GTIFF=ON \
		-DGDAL_ENABLE_DRIVER_MEM=ON \
		-DGDAL_ENABLE_DRIVER_VRT=ON \
		-DGDAL_ENABLE_DRIVER_PNG=ON \
		-DGDAL_ENABLE_DRIVER_JPEG=ON \
		-DOGR_ENABLE_DRIVER_MEM=ON \
		-DZLIB_INCLUDE_DIR="${PREFIX}/include" \
		-DZLIB_LIBRARY="${PREFIX}/lib/libz.a" \
		-DTIFF_INCLUDE_DIR="${PREFIX}/include" \
		-DTIFF_LIBRARY="${PREFIX}/lib/libtiff.a" \
		-DGEOTIFF_INCLUDE_DIR="${PREFIX}/include" \
		-DGEOTIFF_LIBRARY="${PREFIX}/lib/libgeotiff.a" \
		-DPNG_PNG_INCLUDE_DIR="${PREFIX}/include" \
		-DPNG_LIBRARY="${PREFIX}/lib/libpng16.a" \
		-DJPEG_INCLUDE_DIR="${PREFIX}/include" \
		-DJPEG_LIBRARY="${PREFIX}/lib/libjpeg.a" \
		-DSQLite3_INCLUDE_DIR="${PREFIX}/include" \
		-DSQLite3_LIBRARY="${PREFIX}/lib/libsqlite3.a" \
		-DPROJ_INCLUDE_DIR="${PREFIX}/include" \
		-DPROJ_LIBRARY="${PREFIX}/lib/libproj.a"
	run_logged "${LOG_DIR}/gdal.make.log" cmake --build "${WORKDIR}/build-gdal" -j"$(nproc)"
	run_logged "${LOG_DIR}/gdal.install.log" cmake --install "${WORKDIR}/build-gdal"
	cat >"${PREFIX}/bin/${TARGET}-gdal-config" <<EOF
#!/bin/sh
case "\$1" in
  --version) "${WORKDIR}/build-gdal/apps/gdal-config" --version 2>/dev/null || echo "3.10.3" ;;
  --ogr-enabled) echo "yes" ;;
  --cflags) echo "-I${PREFIX}/include" ;;
  --libs) echo "-L${PREFIX}/lib -lgdal -lgeotiff -ltiff -lpng -ljpeg -lz -lproj -lsqlite3 ${libstdcxx} -lwinpthread -lws2_32 -lbcrypt -lole32 -lshell32 -lcrypt32 -lversion -lshlwapi" ;;
  *) echo "unsupported gdal-config option: \$1" >&2; exit 1 ;;
esac
EOF
	chmod +x "${PREFIX}/bin/${TARGET}-gdal-config"
	echo "GDAL_CONFIG_SUMMARY_BEGIN"
	sed -n '/-- The following features have been enabled/,/-- The following features have been disabled/p' "${LOG_DIR}/gdal.configure.log" | sed -n '1,160p'
	echo "GDAL_CONFIG_SUMMARY_END"
}

setup_postgresql_windows()
{
	local zip
	zip="${DOWNLOAD_DIR}/$(basename "${PGWIN_URL}")"
	if test ! -f "${zip}"; then
		"${CURL[@]}" "${PGWIN_URL}" -o "${zip}"
	fi
	rm -rf "${TMP_ROOT}/postgresql-windows"
	mkdir -p "${TMP_ROOT}/postgresql-windows"
	unzip -q "${zip}" -d "${TMP_ROOT}/postgresql-windows"
	find "${PGWIN_ROOT}/bin" -maxdepth 1 -type f \( -iname '*.exe' -o -iname '*.dll' \) -exec chmod +x {} +
	test -f "${PGWIN_ROOT}/bin/postgres.exe"
	test -f "${PGWIN_ROOT}/bin/initdb.exe"
}

configure_postgis()
{
	rm -rf "${BUILD_DIR}"
	mkdir "${BUILD_DIR}"
	cd "${BUILD_DIR}"

	export GEOS_CFLAGS="-I${PREFIX}/include"
	export GEOS_LIBS="-L${PREFIX}/lib -lgeos_c -lgeos -lstdc++"
	export PROJ_CFLAGS="-I${PREFIX}/include"
	export PROJ_LIBS="-L${PREFIX}/lib -lproj -lsqlite3 -lstdc++ -lwinpthread -lws2_32 -lbcrypt -lole32 -lshell32"
	export JSONC_CFLAGS="-I${PREFIX}/include/json-c"
	export JSONC_LIBS="-L${PREFIX}/lib -ljson-c"
	export PROTOBUFC_CFLAGS="-I${PREFIX}/include"
	export PROTOBUFC_LIBS="-L${PREFIX}/lib -lprotobuf-c"

	run_logged "${LOG_DIR}/postgis.configure.log" "${REPO_ROOT}/configure" \
		--host="${TARGET}" \
		--prefix="${PREFIX}" \
		--mandir="${PREFIX}/share/man" \
		--with-pgconfig="${PREFIX}/bin/${TARGET}-pg_config" \
		--with-geosconfig="${PREFIX}/bin/${TARGET}-geos-config" \
		--with-gdalconfig="${PREFIX}/bin/${TARGET}-gdal-config" \
		--with-xml2config="${PREFIX}/bin/xml2-config" \
		--with-libiconv="${PREFIX}" \
		--without-sfcgal \
		--disable-spellcheck-tests \
		CFLAGS="-O2 -Wall" \
		LDFLAGS="-L${PREFIX}/lib -static -static-libgcc -static-libstdc++"

	echo "CONFIGURE_SUMMARY_BEGIN"
	sed -n '/^  PostGIS is now configured/,/^$/p' "${LOG_DIR}/postgis.configure.log"
	echo "CONFIGURE_SUMMARY_END"
}

build_postgis_targets()
{
	cd "${REPO_ROOT}/${BUILD_DIR}"
	run_logged "${LOG_DIR}/postgis.make.log" make -j1
	run_logged "${LOG_DIR}/postgis.cunit.make.log" make -C liblwgeom/cunit -j1
	run_logged "${LOG_DIR}/postgis.install.log" make install

	echo "BUILD_TAIL_BEGIN"
	tail -120 "${LOG_DIR}/postgis.make.log"
	tail -80 "${LOG_DIR}/postgis.cunit.make.log"
	tail -120 "${LOG_DIR}/postgis.install.log"
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
			if test -d raster/loader; then
				cp "${path}" raster/loader/
			fi
			cp "${path}" "${PGWIN_ROOT}/bin/" || true
		fi
	done
}

prepare_wine_prefix()
{
	local status
	mkdir -p "${WINE_TMPDIR}"
	# Debian bookworm's Wine 8 misreports PostgreSQL Windows binaries as invalid.
	# WineHQ stable carries the fix, but disable GUI bootstrap extras for headless CI.
	timeout 240 "${XVFB_WINE[@]}" reg add 'HKEY_CURRENT_USER\Software\Wine\WineDbg' \
		/v ShowCrashDialog /t REG_DWORD /d 0 /f \
	> "${LOG_DIR}/wine.reg.log" 2>&1 || {
		status=$?
		"${WINE}" wineserver -k >/dev/null 2>&1 || true
		log_tail "${status}" "${LOG_DIR}/wine.reg.log"
	}
}

run_wine_checks()
{
	local cunit_runner
	cd "${REPO_ROOT}/${BUILD_DIR}"
	cunit_runner=liblwgeom/cunit/cu_tester.exe
	if test ! -f "${cunit_runner}"; then
		cunit_runner=liblwgeom/cunit/cu_tester
	fi

	case "${WINEPREFIX}" in
		"${TMP_ROOT}"/*) rm -rf "${WINEPREFIX}" ;;
	esac
	prepare_wine_prefix
	copy_mingw_runtime_dlls
	"${XVFB_WINE[@]}" "${cunit_runner}" > "${LOG_DIR}/postgis.cunit.wine.log" 2>&1 \
		|| log_tail "$?" "${LOG_DIR}/postgis.cunit.wine.log"
	echo "CUNIT_WINE_BEGIN"
	tail -80 "${LOG_DIR}/postgis.cunit.wine.log"
	echo "CUNIT_WINE_END"

	"${XVFB_WINE[@]}" loader/shp2pgsql.exe -s 4326 "${REPO_ROOT}/regress/loader/Point" public.point \
		> "${LOG_DIR}/point.sql" 2>"${LOG_DIR}/shp2pgsql.stderr" \
		|| log_tail "$?" "${LOG_DIR}/shp2pgsql.stderr"
	grep '^INSERT INTO "public"."point"' "${LOG_DIR}/point.sql" | tr -d '\r' > "${LOG_DIR}/point.inserts"
	cat > "${LOG_DIR}/point.inserts.expected" <<'EOF'
INSERT INTO "public"."point" (geom) VALUES ('0101000020E61000000000000000000000000000000000F03F');
INSERT INTO "public"."point" (geom) VALUES ('0101000020E61000000000000000002240000000000000F0BF');
INSERT INTO "public"."point" (geom) VALUES ('0101000020E61000000000000000002240000000000000F0BF');
EOF
	diff -u "${LOG_DIR}/point.inserts.expected" "${LOG_DIR}/point.inserts"

	"${XVFB_WINE[@]}" loader/pgsql2shp.exe -? > "${LOG_DIR}/pgsql2shp.usage" 2>&1 || true
	grep -F "USAGE: pgsql2shp" "${LOG_DIR}/pgsql2shp.usage" >/dev/null

	"${XVFB_WINE[@]}" raster/loader/raster2pgsql.exe -s 4326 "${REPO_ROOT}/raster/test/regress/loader/testraster.tif" public.testraster \
		> "${LOG_DIR}/testraster.sql" 2>"${LOG_DIR}/raster2pgsql.stderr" \
		|| log_tail "$?" "${LOG_DIR}/raster2pgsql.stderr"
	grep -F 'CREATE TABLE "public"."testraster"' "${LOG_DIR}/testraster.sql" >/dev/null

	echo "LOADER_CHECK_BEGIN"
	sed -n '1,16p' "${LOG_DIR}/point.sql"
	diff -u "${LOG_DIR}/point.inserts.expected" "${LOG_DIR}/point.inserts"
	sed -n '1,8p' "${LOG_DIR}/pgsql2shp.usage"
	sed -n '1,12p' "${LOG_DIR}/testraster.sql"
	echo "LOADER_CHECK_END"
}

wine_path()
{
	"${WINE}" winepath -w "$1" 2>/dev/null | tr -d '\r'
}

prepare_postgresql_wine_tree()
{
	rm -rf "${PGWIN_RUN_ROOT}"
	mkdir -p "${PGWIN_RUN_ROOT}"
	cp -a "${PGWIN_ROOT}/." "${PGWIN_RUN_ROOT}/"
	find "${PGWIN_RUN_ROOT}/bin" -maxdepth 1 -type f \( -iname '*.exe' -o -iname '*.dll' \) -exec chmod +x {} +
}

install_vc_runtime()
{
	local redist
	if test "${INSTALL_VC_RUNTIME}" = "1"; then
		redist="${DOWNLOAD_DIR}/vc_redist.x64.exe"
		if test ! -f "${redist}"; then
			"${CURL[@]}" -o "${redist}" "${VCREDIST_URL}"
		fi
		"${XVFB_WINE[@]}" "${redist}" /install /quiet /norestart \
			> "${LOG_DIR}/vcredist.install.log" 2>&1 || true
	fi
	(
		cd "${PGWIN_RUN_ROOT}/bin"
		"${XVFB_WINE[@]}" ./postgres.exe --version
	) > "${LOG_DIR}/postgresql.postgres-version.log" 2>&1 \
		|| log_tail "$?" "${LOG_DIR}/postgresql.postgres-version.log"
	echo "POSTGRES_WINE_VERSION_BEGIN"
	cat "${LOG_DIR}/postgresql.postgres-version.log"
	echo "POSTGRES_WINE_VERSION_END"
}

start_postgresql_wine()
{
	local pgdata_win
	local log_win
	local pid
	prepare_wine_prefix
	prepare_postgresql_wine_tree
	install_vc_runtime
	rm -rf "${PGDATA}" "${PGDATA_RUN}"
	mkdir -p "${PGDATA_RUN}"
	pgdata_win=$(wine_path "${PGDATA_RUN}")
	log_win=$(wine_path "${LOG_DIR}/postgresql-wine.log")
	(
		cd "${PGWIN_RUN_ROOT}/bin"
		"${XVFB_WINE[@]}" ./initdb.exe -D "${pgdata_win}" -U "${PGUSER}" -A trust --encoding=UTF8 --locale=C --no-sync
	) > "${LOG_DIR}/postgresql.initdb.log" 2>&1 || log_tail "$?" "${LOG_DIR}/postgresql.initdb.log"
	cat >>"${PGDATA_RUN}/postgresql.conf" <<EOF
listen_addresses = '127.0.0.1'
port = ${PGPORT}
unix_socket_directories = ''
dynamic_shared_memory_type = windows
shared_buffers = 32MB
max_connections = 20
fsync = off
EOF
	cat >>"${PGDATA_RUN}/pg_hba.conf" <<'EOF'
host all all 127.0.0.1/32 trust
host all all ::1/128 trust
EOF
	(
		cd "${PGWIN_RUN_ROOT}/bin"
		"${XVFB_WINE[@]}" ./pg_ctl.exe -D "${pgdata_win}" -l "${log_win}" -w start
	) > "${LOG_DIR}/postgresql.start.log" 2>&1 || log_tail "$?" "${LOG_DIR}/postgresql.start.log"
	pid=$(
		cd "${PGWIN_RUN_ROOT}/bin"
		"${XVFB_WINE[@]}" ./pg_ctl.exe -D "${pgdata_win}" status 2>/dev/null \
			| sed -n 's/.*PID: *\([0-9][0-9]*\).*/\1/p' | head -1 || true
	)
	echo "${pid}" > "${TMP_ROOT}/postgresql-wine.pid"
	PGHOST="${PGHOST}" PGPORT="${PGPORT}" PGUSER="${PGUSER}" psql -d postgres -c 'select version()' \
		> "${LOG_DIR}/postgresql.psql-version.log" 2>&1 || log_tail "$?" "${LOG_DIR}/postgresql.psql-version.log"
	echo "POSTGRESQL_WINE_BEGIN"
	tail -60 "${LOG_DIR}/postgresql.start.log"
	cat "${LOG_DIR}/postgresql.psql-version.log"
	echo "POSTGRESQL_WINE_END"
}

stop_postgresql_wine()
{
	local pgdata_win
	if test -d "${PGDATA_RUN}"; then
		pgdata_win=$(wine_path "${PGDATA_RUN}" || true)
		if test -n "${pgdata_win}"; then
			(
				cd "${PGWIN_RUN_ROOT}/bin"
				"${XVFB_WINE[@]}" ./pg_ctl.exe -D "${pgdata_win}" -m fast -w stop
			) > "${LOG_DIR}/postgresql.stop.log" 2>&1 || true
		fi
	fi
}

install_regression_wrappers()
{
	local wrapdir
	wrapdir="${TMP_ROOT}/wine-wrappers"
	mkdir -p "${wrapdir}"
	for exe in shp2pgsql pgsql2shp raster2pgsql; do
		local exe_path
		if test "${exe}" = raster2pgsql; then
			exe_path="${REPO_ROOT}/${BUILD_DIR}/raster/loader/${exe}.exe"
		else
			exe_path="${REPO_ROOT}/${BUILD_DIR}/loader/${exe}.exe"
		fi
		cat >"${wrapdir}/${exe}" <<EOF
#!/bin/sh
exec env -u XDG_RUNTIME_DIR TMPDIR='${WINE_TMPDIR}' '${WINE}' '${exe_path}' "\$@"
EOF
		chmod +x "${wrapdir}/${exe}"
	done
	export PATH="${wrapdir}:${PATH}"
}

run_regressions()
{
	local regress_installdir
	cd "${REPO_ROOT}/${BUILD_DIR}"
	install_regression_wrappers
	regress_installdir="${REPO_ROOT}/${BUILD_DIR}/regress/00-regress-install"
	export PGHOST PGPORT PGUSER
	export PGDATABASE=postgres
	export PGIS_REG_TMPDIR="${TMP_ROOT}/pgis_reg"
	export RUNTESTFLAGS="-v --raster --topology"
	export POSTGIS_TOP_BUILD_DIR="${REPO_ROOT}/${BUILD_DIR}"
	PGHOST="${PGHOST}" PGPORT="${PGPORT}" PGUSER="${PGUSER}" psql -d postgres -c 'CREATE EXTENSION postgis; CREATE EXTENSION postgis_raster; CREATE EXTENSION postgis_topology; SELECT postgis_full_version();' \
		> "${LOG_DIR}/postgis.full-version.log" 2>&1 || log_tail "$?" "${LOG_DIR}/postgis.full-version.log"
	echo "POSTGIS_FULL_VERSION_BEGIN"
	cat "${LOG_DIR}/postgis.full-version.log"
	echo "POSTGIS_FULL_VERSION_END"
	run_logged "${LOG_DIR}/postgis.installcheck.log" make installcheck RUNTESTFLAGS="${RUNTESTFLAGS}" REGRESS_INSTALLDIR="${regress_installdir}"
	echo "REGRESSION_SUMMARY_BEGIN"
	grep -E '^(Run tests|Running|PASS|FAIL|SKIP|ERROR|Failed|Summary|Suite)' "${LOG_DIR}/postgis.installcheck.log" | tail -200 || tail -200 "${LOG_DIR}/postgis.installcheck.log"
	echo "REGRESSION_SUMMARY_END"
}

main()
{
	local started
	started=$(date +%s)

	mkdir -p "${TMPDIR}" "${LOG_DIR}" "${WINE_TMPDIR}"
	if test -n "${RUN_FROM}"; then
		phase_log "RUN_FROM ${RUN_FROM}"
	else
		: >"${LOG_DIR}/phase-times.log"
	fi
	phase install_packages install_packages
	phase fetch_sources fetch_sources
	phase build_sqlite build_sqlite
	phase build_zlib build_zlib
	phase build_libjpeg build_libjpeg
	phase build_libpng build_libpng
	phase build_libiconv build_libiconv
	phase build_gmp build_gmp
	phase build_cunit build_cunit
	phase build_geos build_geos
	phase build_libtiff build_libtiff
	phase build_proj build_proj
	phase build_libgeotiff build_libgeotiff
	phase build_gdal build_gdal
	phase build_json_c build_json_c
	phase build_protobuf_c build_protobuf_c
	phase setup_postgresql_windows setup_postgresql_windows
	phase build_libxml2 build_libxml2
	phase build_libpq build_libpq

	phase run_autogen run_autogen
	phase configure_postgis configure_postgis
	phase build_postgis_targets build_postgis_targets
	phase run_wine_checks run_wine_checks
	trap stop_postgresql_wine EXIT
	phase start_postgresql_wine start_postgresql_wine
	phase run_regressions run_regressions

	phase_log "WALL_CLOCK_SECONDS=$(( $(date +%s) - started ))"
}

main "$@"
