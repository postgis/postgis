#!/bin/sh

if [ -z "$1" ]; then
	echo "Usage: `basename $0` <geosdir>" >&2
	exit 1
fi

geos_dir=$1
version=`${geos_dir}/bin/geos-config --version`
if [ $? -ne 0 ]; then
	exit 1
fi

if [ "$version" = "@GEOS_VERSION@" ]; then
	geos_version=100
	version="1.0.0"
else
	major=`echo $version | sed 's/\..*//'`
	minor=`echo $version | sed 's/[^\.]*\.\([^.]*\)\.*/\1/'`
	geos_version=`printf %d%2.2d $major $minor`
fi
cat <<EOF
#define POSTGIS_GEOS_VERSION $geos_version
#ifndef GEOS_VERSION
#define GEOS_VERSION "$version"
#endif
EOF
