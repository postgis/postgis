#!/bin/sh

major=`grep ^SO_MAJOR_VERSION Version.config | cut -d= -f2`
minor=`grep ^SO_MINOR_VERSION Version.config | cut -d= -f2`
micro=`grep ^SO_MICRO_VERSION Version.config | cut -d= -f2`

if [ -n "$3" ]; then
	major=$1
	minor=$2
	micro=$3
fi

tag="pgis_"$major"_"$minor"_"$micro
version="$major.$minor.$micro"
version=`echo $version | sed 's/RC/-rc/'`
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

## remove regress tests
#echo "Removing regress tests"
#rm -Rf $outdir/regress

# generating configure script
echo "Running autoconf"
owd="$PWD"
cd "$outdir"
autoconf
# remove the autom4te.cache dir, created by autoconf
rm -Rf autom4te.cache
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
make clean # won't drop the html dir
cd "$owd"

## generating parser
#echo "Generating parser"
#owd="$PWD"
#cd "$outdir"/lwgeom
#make lex.yy.c
#if [ $? -gt 0 ]; then
#	exit 1
#fi
#cd "$owd"

echo "Generating tar file"
tar czf "$package" "$outdir"

#echo "Cleaning up"
#rm -Rf "$outdir"

echo "Package file is $package"
echo "Package dir is $outdir"
