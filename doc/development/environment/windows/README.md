# Windows Development Environments

PostGIS has two maintained Windows build surfaces:

* The MSYS2 GitHub Actions job in `.github/workflows/msys.yml`.
* The Winnie Jenkins scripts under `ci/winnie/`.

Use those files as the current source of truth when checking Windows-specific
build behavior. The imported Trac pages record older MinGW, MinGW-w64, MSYS,
and MSVC recipes from the PostGIS 1.x and 2.x packaging era. They are useful
for understanding historical packaging constraints, but they should not be used
as current setup instructions without checking the live build scripts first.

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

## Historical MinGW Setup Notes

`DevWikiMingW64_Setup` is archived in `doc/trac-wiki/development/` for
provenance. It describes MinGW-w64 and MSYS 1.x setup by downloading old
SourceForge toolchain zips, selecting GCC 4.5-era packages, copying DLL import
libraries by hand, and compiling build tools and CUnit manually.

Do not publish those steps as current instructions. PostgreSQL 9.x, the
referenced GEOS 3.3-era compiler issues, and the MSYS 1.x bootstrap flow are
historical context. Current Windows fixes should instead update the relevant
MSYS2 workflow, Winnie script, or shared build logic, then document any durable
rule here.
