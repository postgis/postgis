#!/bin/sh

major=`grep ^SO_MAJOR_VERSION lwgeom/Makefile | cut -d= -f2`
minor=`grep ^SO_MINOR_VERSION lwgeom/Makefile | cut -d= -f2`
micro=`grep ^SO_MICRO_VERSION lwgeom/Makefile | cut -d= -f2`

tag="pgis_"$major"_"$minor"_"$micro
version="$major.$minor.$micro"
package="postgis-$version.tgz"
outdir="postgis-$version"

if [ -d "$outdir" ]; then
	echo "Output directory $outdir already exist"
	exit 1
fi

echo "Exporting tag $tag"
cvs export -r "$tag" -d "$outdir" postgis 
if [ $? -gt 0 ]; then
	exit 1
fi

# remove .cvsignore
echo "Removing .cvsignore and make_dist files"
find "$outdir" -name .cvsignore -exec rm {} \;
rm -f "$outdir"/make_dist

# remove regress tests
echo "Removing regress tests"
rm -Rf $outdir/regress

# generating documentation
echo "Generating documentation"
owd="$PWD"
cd "$outdir"/doc
make html
if [ $? -gt 0 ]; then
	exit 1
fi
cd "$owd"

echo "Generating tar file"
tar czf "$package" "$outdir"

#echo "Cleaning up"
#rm -Rf "$outdir"

echo "Package file is $package"
echo "Package dir is $outdir"
