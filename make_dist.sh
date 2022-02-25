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
#   newoutdir  this variable can be overriden to control the outdir and package name.
#              package name will be set to {newoutdir}.tar.gz if this variabe is overridden
#

version=dev

# Define in environment if necessary to get postgis to configure,
# which is only done to build comments.
#CONFIGURE_ARGS=

# Define in environment if Gnu make is not make (e.g., gmake).
if [ "$MAKE" = "" ]; then
    MAKE=make
fi

# Extract tag from git or default to trunk
tag=`git branch | grep \* | awk '{print $2}'`

if [ -n "$1" ]; then
  if [ "$1" = "-b" ]; then
    shift
    tag=svn-$1
    branch=yes
  else
    tag=$1
    version="$1"
  fi
fi

outdir="postgis-$version"

if [ -d "$outdir" ]; then
	echo "Output directory $outdir already exists."
	exit 1
fi

git clone -b $tag . $outdir || exit 1

echo "Removing make_dist.sh and HOWTO_RELEASE"
rm -fv "$outdir"/make_dist.sh "$outdir"/HOWTO_RELEASE

echo "Removing ci files"
rm -rfv "$outdir"/ci "$outdir"/.gitlab-ci.yml "$outdir"/.github "$outdir"/.dron*.yml "$outdir"/.cirrus.yml "$outdir"/.clang-format

# generating configure script and configuring
echo "Running autogen.sh; ./configure"
owd="$PWD"
cd "$outdir"
./autogen.sh
./configure ${CONFIGURE_ARGS}
# generating postgis_revision.h for >= 2.0.0 tags
if test -f utils/repo_revision.pl; then
	echo "Generating postgis_revision.h"
	perl utils/repo_revision.pl
fi
# generate ChangeLog
make ChangeLog
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

echo "Removing .git dir"
rm -rf .git

cd "$owd"

# Find a better version name when fetching
# a development branch
if test "$version" = "dev"; then
  echo "Tweaking version for development snapshot"
  VMAJ=`grep ^POSTGIS_MAJOR_VERSION "$outdir"/Version.config | cut -d= -f2`
  VMIN=`grep ^POSTGIS_MINOR_VERSION "$outdir"/Version.config | cut -d= -f2`
  VMIC=`grep ^POSTGIS_MICRO_VERSION "$outdir"/Version.config | cut -d= -f2`
  VREV=`cat "$outdir"/postgis_revision.h | awk '{print $3}'`
  version="${VMAJ}.${VMIN}.${VMIC}-${VREV}"
  #if newoutdir is not already set, then set it
  if test "x$newoutdir" = "x"; then
      newoutdir=postgis-${version}
  else
      package=${newoutdir}.tar.gz
  fi
  rm -rf ${newoutdir}
  mv -v "$outdir" "$newoutdir"
  outdir=${newoutdir}
fi

#if package name is not already set then set it
if test "x$package" = "x"; then
    package="postgis-$version.tar.gz"
fi
echo "Generating $package file"
tar czf "$package" "$outdir"

#echo "Cleaning up"
#rm -Rf "$outdir"
