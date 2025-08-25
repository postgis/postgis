#!/bin/sh

VER=master

if test -n "$1"; then
  VER=$1
fi

cat <<EOF
||= '''${VER}''' =|| \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Make_Dist%2Flabel%3Ddebbie&build=last:\${params.reference=refs/heads/stable-${VER}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Make_Dist/label=debbie/)]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_${VER}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_${VER}/, alt=status, height="20", title='PG 12-17(branch), GDAL 3.5, GEOS 3.13')]] || \\
|| [[Image(https://winnie.postgis.net/buildStatus/icon?job=PostGIS_${VER}, link=https://winnie.postgis.net/view/PostGIS/job/PostGIS_${VER}/, alt=status, height="20", title='PG 12-17, GDAL 3.7.1, GEOS 3.13 64-bit')]] || \\
|| [[Image(https://dronie.osgeo.org/api/badges/postgis/postgis/status.svg?ref=refs/heads/stable-${VER}, link=https://dronie.osgeo.org/postgis/postgis?branch=stable-${VER}, alt=status)]] || \\
|| [[Image(https://github.com/postgis/postgis/actions/workflows/ci.yml/badge.svg?branch=stable-${VER}, link=https://github.com/postgis/postgis/actions?query=branch%3Astable-${VER}, alt=status, height="20")]] || \\
|| [[Image(https://woodie.osgeo.org/api/badges/postgis/postgis/status.svg?branch=stable-${VER}, link=https://woodie.osgeo.org/postgis/postgis?branch=stable-${VER}, alt=status)]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Worker_Run/label=bessie&build=last:\${params.reference=refs/heads/stable-${VER}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Worker_Run/label=bessie, alt=status, height="20", title='PG 10.1(branch), GDAL 2.1 branch, GEOS 3.6')]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Worker_Run/label=bessie32&build=last:\${params.reference=refs/heads/stable-${VER}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Worker_Run/label=bessie32, alt=status, height="20", title='PG 10.1(branch), GDAL 2.1 branch, GEOS 3.6')]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Worker_Run/label=berrie&build=last:\${params.reference=refs/heads/stable-${VER}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Worker_Run/label=berrie, alt=status, height="20", title='PG 12(branch), GDAL 2.4.0 branch, GEOS 3.7.1')]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Worker_Run/label=berrie64&build=last:\${params.reference=refs/heads/stable-${VER}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Worker_Run/label=berrie64?build=last:\${params.reference=refs/heads/stable-${VER}, alt=status, height="20", title='PG 12(branch), GDAL 2.4.0 branch, GEOS 3.7.1')]] || \\
|| [[Image(https://api.cirrus-ci.com/github/postgis/postgis.svg?branch=stable-${VER}, link=https://cirrus-ci.com/github/postgis/postgis, alt=status, height="20", title='PG 13(branch), GDAL 3.5 branch, GEOS 3.12')]] || \\
|| [[Image(https://gitlab.com/postgis/postgis/badges/stable-${VER}/pipeline.svg, link=https://gitlab.com/postgis/postgis/-/commits/stable-${VER}, alt=status, height="20", title='32/64 bit trixie, PG 15, GDAL 2.4.0, GEOS 3.7.1, PROJ 5.2.0')]] || \\
|| [[Image(https://www.ict.inaf.it/gitlab/postgis/postgis/badges/master/pipeline.svg?ref=refs/heads/${VER}, link=https://www.ict.inaf.it/gitlab/postgis/postgis/-/commits/${VER}, alt=inaf/${VER})]] ||
EOF
