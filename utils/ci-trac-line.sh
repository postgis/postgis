#!/bin/sh

set -eu

usage()
{
	cat <<EOF
Usage:
  $0 [VERSION]
  $0 --check FILE VERSION
  $0 --replace FILE VERSION

Print, check, or replace a Trac ContinuousIntegration badge row for a stable
release branch. VERSION is the minor release number without the "stable-"
prefix, for example "3.7".
EOF
}

generate_row()
{
	VER="$1"
	BRANCH="stable-${VER}"

	cat <<EOF
||= '''${VER}''' =|| \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Make_Dist%2Flabel%3Ddebbie&build=last:\${params.reference=refs/heads/${BRANCH}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Make_Dist/label=debbie/)]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_${VER}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_${VER}/, alt=status, height="20", title='PG 12-17(branch), GDAL 3.5, GEOS 3.13')]] || \\
|| [[Image(https://winnie.postgis.net/buildStatus/icon?job=PostGIS_${VER}, link=https://winnie.postgis.net/view/PostGIS/job/PostGIS_${VER}/, alt=status, height="20", title='PG 12-17, GDAL 3.7.1, GEOS 3.13 64-bit')]] || \\
|| [[Image(https://github.com/postgis/postgis/actions/workflows/ci.yml/badge.svg?branch=${BRANCH}, link=https://github.com/postgis/postgis/actions?query=branch%3A${BRANCH}, alt=status, height="20")]] || \\
|| [[Image(https://woodie.osgeo.org/api/badges/postgis/postgis/status.svg?branch=${BRANCH}, link=https://woodie.osgeo.org/postgis/postgis/branches/${BRANCH}, alt=status)]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Worker_Run/label=bessie&build=last:\${params.reference=refs/heads/${BRANCH}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Worker_Run/label=bessie, alt=status, height="20", title='PG 18.4, GDAL 3.11.5, GEOS 3.14.1')]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Worker_Run/label=berrie&build=last:\${params.reference=refs/heads/${BRANCH}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Worker_Run/label=berrie, alt=status, height="20", title='PG 12(branch), GDAL 2.4.0 branch, GEOS 3.7.1')]] || \\
|| [[Image(https://debbie.postgis.net/buildStatus/icon?job=PostGIS_Worker_Run/label=berrie64&build=last:\${params.reference=refs/heads/${BRANCH}}, link=https://debbie.postgis.net/view/PostGIS/job/PostGIS_Worker_Run/label=berrie64, alt=status, height="20", title='PG 12(branch), GDAL 2.4.0 branch, GEOS 3.7.1')]] || \\
|| [[Image(https://github.com/postgis/postgis/actions/workflows/ci-freebsd.yml/badge.svg?branch=${BRANCH}, link=https://github.com/postgis/postgis/actions/workflows/ci-freebsd.yml?query=branch%3A${BRANCH}, alt=status, height="20", title='FreeBSD')]] || \\
|| [[Image(https://github.com/postgis/postgis/actions/workflows/ci-macos.yml/badge.svg?branch=${BRANCH}, link=https://github.com/postgis/postgis/actions/workflows/ci-macos.yml?query=branch%3A${BRANCH}, alt=status, height="20", title='macOS')]] || \\
|| [[Image(https://gitlab.com/postgis/postgis/badges/${BRANCH}/pipeline.svg, link=https://gitlab.com/postgis/postgis/-/commits/${BRANCH}, alt=status, height="20", title='32/64 bit trixie, PG 15, GDAL 2.4.0, GEOS 3.7.1, PROJ 5.2.0')]] || \\
|| [[Image(https://www.ict.inaf.it/gitlab/postgis/postgis/badges/master/pipeline.svg?ref=refs/heads/${BRANCH}, link=https://www.ict.inaf.it/gitlab/postgis/postgis/-/commits/${BRANCH}, alt=inaf/${BRANCH})]] ||
EOF
}

check_row()
{
	file="$1"
	ver="$2"
	block="$(generate_row "$ver")"
	BLOCK="$block" perl -0e '
		local $/;
		my $content = <>;
		exit(defined($content) && length($content) && index($content, $ENV{BLOCK}) >= 0 ? 0 : 1);
	' "$file"
}

replace_row()
{
	file="$1"
	ver="$2"
	tmp="${file}.tmp.$$"
	block="$(generate_row "$ver")"

	BLOCK="$block" VER="$ver" perl -0ne '
		my $ver = quotemeta($ENV{VER});
		my $block = $ENV{BLOCK};
		my $changed = s/\|\|= \x27\x27\x27$ver\x27\x27\x27 =\|\| \\\n.*?(?=\|\|= \x27\x27\x27\d+\.\d+\x27\x27\x27 =\|\| \\\n|\}\}\}|\z)/$block\n/s;
		print;
		exit($changed ? 0 : 2);
	' "$file" > "$tmp" || {
		status=$?
		rm -f "$tmp"
		exit "$status"
	}
	mv "$tmp" "$file"
}

case "${1:-}" in
	-h|--help)
		usage
		exit 0
		;;
	--check)
		test "$#" -eq 3 || { usage >&2; exit 2; }
		check_row "$2" "$3"
		;;
	--replace)
		test "$#" -eq 3 || { usage >&2; exit 2; }
		replace_row "$2" "$3"
		;;
	"")
		usage >&2
		exit 2
		;;
	*)
		test "$#" -eq 1 || { usage >&2; exit 2; }
		generate_row "$1"
		;;
esac
