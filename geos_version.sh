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
	jtsport="1.3"
	first=1
	last=1
else
	major=`echo $version | sed 's/\..*//'`
	minor=`echo $version | sed 's/[^\.]*\.\([^\.]*\)\..*/\1/'`
	first=$major
	last=`expr $major + $minor`
	geos_version=`printf %d%2.2d $major $minor`
	jtsport=`${geos_dir}/bin/geos-config --jtsport`
fi
cat <<EOF
#ifndef GEOS_FIRST_INTERFACE
#define GEOS_FIRST_INTERFACE $first
#endif
#ifndef GEOS_LAST_INTERFACE
#define GEOS_LAST_INTERFACE $last
#endif
#ifndef GEOS_VERSION
#define GEOS_VERSION "$version"
#endif
#ifndef GEOS_JTS_PORT
#define GEOS_JTS_PORT $jtsport
#endif
#define POSTGIS_GEOS_VERSION $geos_version
EOF
