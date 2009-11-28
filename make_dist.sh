#!/bin/sh

#
# USAGE:
#
# -- postgis-cvs.tar.gz 
# sh make_dist.sh
#
# -- postgis-1.1.0.tar.gz 
# sh make_dist.sh 1.1.0
#
# NOTE: will not work prior to 1.1.0
#
#

tag=trunk
version=cvs

if [ -n "$1" ]; then
	version="$1"
	#version=`echo $version | sed 's/RC/-rc/'`
	#tag=pgis_`echo "$version" | sed 's/\./_/g'`
	tag="tags/$version"
fi

outdir="postgis-$version"
package="postgis-$version.tar.gz"

if [ -d "$outdir" ]; then
	echo "Output directory $outdir already exist"
	exit 1
fi

echo "Exporting tag $tag"
#cvs export -r "$tag" -d "$outdir" postgis 
svn export "http://svn.osgeo.org/postgis/$tag" "$outdir"
if [ $? -gt 0 ]; then
	exit 1
fi

# remove .cvsignore, make_dist.sh and HOWTO_RELEASE
echo "Removing .cvsignore and make_dist.sh files"
find "$outdir" -name .cvsignore -exec rm {} \;
rm -f "$outdir"/make_dist.sh "$outdir"/HOWTO_RELEASE

# generating configure script
echo "Running autogen.sh; ./configure"
owd="$PWD"
cd "$outdir"
./autogen.sh
./configure
make
cd "$owd"

# generating documentation
echo "Generating documentation images"
owd="$PWD"
cd "$outdir"/doc/html/image_src
sleep 1 # wait some time 
make images 
if [ $? -gt 0 ]; then
	exit 1
fi
cd "$owd"

# generating documentation
echo "Generating documentation"
owd="$PWD"
cd "$outdir"/doc
sleep 2 # wait some time to have 'make' recognize it needs to build html
make 
if [ $? -gt 0 ]; then
	exit 1
fi
# move the comments down a level for safe keeping
cp postgis_comments.sql ..
make clean # won't drop the html dir
# move them back up afterwards
cp ../postgis_comments.sql .
cd "$owd"

# Run make distclean
echo "Running make distclean"
owd="$PWD"
cd "$outdir"
make distclean
cd "$owd"

echo "Generating $package file"
tar czf "$package" "$outdir"

#echo "Cleaning up"
#rm -Rf "$outdir"

