# Windows Development Environments

PostGIS has two maintained Windows build surfaces:

* The MSYS2 GitHub Actions job in `.github/workflows/msys.yml`.
* The Winnie Jenkins scripts under `ci/winnie/`.

Use those files as the current source of truth when checking Windows-specific
build behavior. The retired Trac pages record older MinGW, MinGW-w64, MSYS, and
MSVC recipes from the PostGIS 1.x and 2.x packaging era. They are useful for
understanding historical packaging constraints, but they should not be used as
current setup instructions without checking the live build scripts first.

## MSYS2 GitHub Actions

The GitHub Actions job uses `msys2/setup-msys2` and installs dependencies with
the MSYS2 package manager instead of hand-building GEOS, PROJ, GDAL, libxml2,
CUnit, PostgreSQL, and gettext from source. At the time of this note the active
matrix entry is `clang64`; older `mingw64` and `ucrt64` entries are kept in the
workflow as comments.

The job initializes a temporary PostgreSQL cluster, runs `./autogen.sh`,
configures PostGIS with the MSYS2 `pg_config`, builds, and runs `make check`.
When changing Windows-sensitive build code, compare the local change against
that workflow before reviving an old Trac command line.

## Winnie

The Winnie scripts are the long-running Windows packaging and regression-test
surface. Shared configuration lives in `ci/winnie/winnie_common.sh`, including
dependency versions, `PATH`, `PKG_CONFIG_PATH`, `PROJ_LIB`, `PROJ_PATH`, and the
`MSYS2_ARG_CONV_EXCL` path-conversion guard.

Use Winnie when the question is about release packaging, dependency bundles, or
the Windows jobs represented on the project CI inventory. Use the MSYS2 GitHub
Actions workflow when the question is about the lightweight pull-request build.

## Historical MinGW Packaging Notes

`DevWikiMingW64_Setup` described MinGW-w64 and MSYS 1.x setup by downloading
old SourceForge toolchain zips, selecting GCC 4.5-era packages, copying DLL
import libraries by hand, and compiling build tools and CUnit manually.

`DevWikiWinMingW64` and `DevWikiWinMingW64_21` are the matching full PostGIS
2.0 and 2.1 packaging recipes. They assume prepared `ming64.zip`, `ming32.zip`,
or `ming64gcc48.zip` build environments, PostgreSQL 9.x, GCC 4.x, GDAL 1.x,
GEOS 3.3/3.4, PROJ 4.x, DocBook XSL 1.76, and source-built dependency prefixes
under `PROJECTS`. Their durable value is the separation between a release
packaging environment and a lightweight pull-request build, not the exact
commands.

The older `DevWikiWinMingWSys_20`, `DevWikiWinMingWSys_20_MSVC`,
`DevWikiWinVC_15`, and `DevWikiWinNSIS` notes were even more tightly bound to
PostGIS 1.5 and 2.0, PostgreSQL 9.0/9.1, Windows XP, Visual Studio 2008/2010,
SourceForge-era MinGW/MSYS installers, hand-edited dependency headers, and
Stack Builder/NSIS packaging. Their surviving lesson is that Windows release
packaging needs an explicit dependency bundle, installer payload list, and
regression-test environment. In current repository terms, those responsibilities
belong to Winnie and the MSYS2 workflow, not to standalone wiki recipes.

Do not publish those steps as current instructions. The current analogue for
prepared dependency bundles, `OS_BUILD`, `MINGHOST`, `GCC_TYPE`, and versioned
dependency prefixes is Winnie, especially `ci/winnie/winnie_common.sh`. The
current pull-request smoke build is the MSYS2 workflow, which installs
dependencies from MSYS2 packages and should not copy source-build bootstrap
steps from the old wiki pages. Current Windows fixes should update the relevant
MSYS2 workflow, Winnie build/package/regression script, or shared build logic,
then document any durable rule here.
