#!/bin/sh

#
# USAGE:
#
# -- postgis-x.y.0dev-r#####.tar.gz
# sh make_dist.sh
#
# -- postgis-1.1.0.tar.gz 
# sh make_dist.sh 1.1.0
#
# NOTE: will not work prior to 1.1.0
#
#

tag=trunk
version=dev

if [ -n "$1" ]; then
	version="$1"
	tag="tags/$version"
fi

outdir="postgis-$version"

if [ -d "$outdir" ]; then
	echo "Output directory $outdir already exists."
	exit 1
fi

echo "Exporting tag $tag"
svnurl="http://svn.osgeo.org/postgis/$tag"
svn export $svnurl "$outdir"
if [ $? -gt 0 ]; then
	exit 1
fi

echo "Removing ignore files, make_dist.sh and HOWTO_RELEASE"
find "$outdir" -name .\*ignore -exec rm -v {} \;
rm -fv "$outdir"/make_dist.sh "$outdir"/HOWTO_RELEASE

# generating configure script and configuring
echo "Running autogen.sh; ./configure"
owd="$PWD"
cd "$outdir"
./autogen.sh
./configure
# generating postgis_svn_revision.h for >= 2.0.0 tags 
if test -f utils/svn_repo_revision.pl; then 
	echo "Generating postgis_svn_revision.h"
	perl utils/svn_repo_revision.pl $svnurl
fi
#make
cd "$owd"

# generating comments
echo "Generating documentation"
owd="$PWD"
cd "$outdir"/doc
make comments
if [ $? -gt 0 ]; then
	exit 1
fi
make clean # won't drop the comment files
cd "$owd"

# Run make distclean
echo "Running make distclean"
owd="$PWD"
cd "$outdir"
make distclean
cd "$owd"

# Find a better version name when fetching
# a development branch
if test "$tag" = "trunk"; then
  echo "Tweaking version for development snapshot"
  VMAJ=`grep POSTGIS_MAJOR_VERSION "$outdir"/Version.config | cut -d= -f2`
  VMIN=`grep POSTGIS_MINOR_VERSION "$outdir"/Version.config | cut -d= -f2`
  VMIC=`grep POSTGIS_MICRO_VERSION "$outdir"/Version.config | cut -d= -f2`
  VREV=`cat "$outdir"/postgis_svn_revision.h | awk '{print $3}'`
  version="${VMAJ}.${VMIN}.${VMIC}-r${VREV}"
  newoutdir=postgis-${version}
  mv -vi "$outdir" "$newoutdir"
  outdir=${newoutdir}
fi

package="postgis-$version.tar.gz"
echo "Generating $package file"
tar czf "$package" "$outdir"

#echo "Cleaning up"
#rm -Rf "$outdir"

