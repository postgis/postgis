#!/bin/sh

if [ -z "$1" ]; then
	echo "Usage `basename $0` <geosdir>" >&2
	exit 1
fi

geos_dir=$1
version=`${geos_dir}/bin/geos-config --version`
if [ "$version" = "@GEOS_VERSION@" ]; then
	geos_version=100
else
	major=`echo $version | sed 's/\..*//'`
	minor=`echo $version | sed 's/.*\.\([^.]*\)\.*/\1/'`
	geos_version=`printf %d%2.2d $major $minor`
fi
cat <<EOF
#define POSTGIS_GEOS_VERSION $geos_version
#define GEOS_VERSION $version
EOF
