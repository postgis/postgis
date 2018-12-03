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
# -- postgis-2.2.tar.gz  (from a branch)
# sh make_dist.sh -b 2.2
#
# NOTE: will not work prior to 1.1.0
#
# ENVIRONMENT VARIABLES:
#
#   CONFIGURE_ARGS    passed to ./configure call
#   MAKE              useed for builds (defaults to "make")
#
#

v_major=`cat Version.config | perl -ne 'print if s/POSTGIS_MAJOR_VERSION=(\d)/$1/'`
v_minor=`cat Version.config | perl -ne 'print if s/POSTGIS_MINOR_VERSION=(\d)/$1/'`
v_micro=`cat Version.config | perl -ne 'print if s/POSTGIS_MICRO_VERSION=(\d)/$1/'`
v_carto=`cat Version.config | perl -ne 'print if s/CARTODB_MICRO_VERSION=(\d)/$1/'`

version="${v_major}.${v_minor}.${v_micro}.${v_carto}"
tag=trunk
outdir="postgis-$version"
package="postgis-$version.tar.gz"
branch=svn-2.4-cartodb

# Define in environment if necessary to get postgis to configure,
# which is only done to build comments.
#CONFIGURE_ARGS=

# Define in environment if Gnu make is not make (e.g., gmake).
if [ "$MAKE" = "" ]; then
    MAKE=make
fi

outdir="postgis-$version"

if [ -d "$outdir" ]; then
    /bin/rm -rf $outdir
fi
mkdir $outdir
echo "Exporting to $outdir ..."

git archive $branch | tar -x -C $outdir
if [ $? -gt 0 ]; then
  echo "Git archive to $outdir failed!"
	exit 1
fi

echo "Removing make_dist.sh and HOWTO_RELEASE"
rm -fv "$outdir"/make_dist.sh "$outdir"/HOWTO_RELEASE

# generating configure script and configuring
echo "Running autogen.sh; ./configure"
owd="$PWD"
cd "$outdir"
./autogen.sh
./configure ${CONFIGURE_ARGS}
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
${MAKE} comments
if [ $? -gt 0 ]; then
	exit 1
fi
${MAKE} clean # won't drop the comment files
cd "$owd"

# Run make distclean
echo "Running make distclean"
owd="$PWD"
cd "$outdir"
${MAKE} distclean
if [ "$git" = "yes" ]; then
  echo "Removing .git dir"
  rm -rf .git
fi
cd "$owd"

# Find a better version name when fetching
# a development branch
if test "$version" = "dev"; then
  echo "Tweaking version for development snapshot"
  VMAJ=`grep ^POSTGIS_MAJOR_VERSION "$outdir"/Version.config | cut -d= -f2`
  VMIN=`grep ^POSTGIS_MINOR_VERSION "$outdir"/Version.config | cut -d= -f2`
  VMIC=`grep ^POSTGIS_MICRO_VERSION "$outdir"/Version.config | cut -d= -f2`
  VREV=`cat "$outdir"/postgis_svn_revision.h | awk '{print $3}'`
  version="${VMAJ}.${VMIN}.${VMIC}-r${VREV}"
  newoutdir=postgis-${version}
  rm -rf ${newoutdir}
  mv -v "$outdir" "$newoutdir"
  outdir=${newoutdir}
fi

package="postgis-$version.tar.gz"
echo "Generating $package file"
tar czf "$package" "$outdir"

#echo "Cleaning up"
#rm -Rf "$outdir"
