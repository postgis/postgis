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
#   MAKE              used for builds (defaults to "make")
#   newoutdir  this variable can be overridden to control the outdir and package name.
#              package name will be set to {newoutdir}.tar.gz if this variable is overridden
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
tag=$(git rev-parse HEAD)

if [ -n "$1" ]; then
  if [ "$1" = "-b" ]; then
    shift
    tag=stable-$1
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

git clone . $outdir || exit 1
cd $outdir && git checkout $tag && cd -

echo "Removing make_dist.sh"
rm -fv "$outdir"/make_dist.sh

echo "Removing developer docs, ai skills, and top-level AGENTS.md"
rm -rfv \
  "$outdir"/AGENTS.md \
  "$outdir"/doc/development \
  "$outdir"/doc/skills \
  "$outdir"/doc/rfc

echo "Removing ci files"
rm -rfv \
  "$outdir"/ci \
  "$outdir"/.clang-format \
  "$outdir"/.dron*.yml \
  "$outdir"/.github \
  "$outdir"/.gitlab-ci.yml \
  "$outdir"/utils/check_releases_md5.sh \
  "$outdir"/.woodpecker

# generating configure script and configuring
echo "Running autogen.sh"
owd="$PWD"
cd "$outdir"
./autogen.sh || exit 1

echo "./configure ${CONFIGURE_ARGS}"
./configure ${CONFIGURE_ARGS} || exit 1
# generating postgis_revision.h for >= 2.0.0 tags
if test -f utils/repo_revision.pl; then
	echo "Generating postgis_revision.h"
	perl utils/repo_revision.pl || exit 1
fi
echo "Generating WKT parser sources"
${MAKE} -C liblwgeom lwin_wkt_parse.c lwin_wkt_parse.h lwin_wkt_lex.c || {
	echo "Failed to generate WKT parser sources" >&2
	exit 1
}
for generated_parser_source in \
	liblwgeom/lwin_wkt_parse.c \
	liblwgeom/lwin_wkt_parse.h \
	liblwgeom/lwin_wkt_lex.c; do
	test -f "$generated_parser_source" || {
		echo "Missing generated WKT parser source: $generated_parser_source" >&2
		exit 1
	}
done
# generate ChangeLog
make ChangeLog || exit 1
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
${MAKE} distclean || exit 1

echo "Removing .git dir"
rm -rf .git || exit 1

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
  rm -rf ${newoutdir} || exit 1
  mv -v "$outdir" "$newoutdir" || exit 1
  outdir=${newoutdir}
fi

#if package name is not already set then set it
if test "x$package" = "x"; then
    package="postgis-$version.tar.gz"
fi

echo "Tweaking timestamps"

# Extract last commit date from git
date=$(git log -1 --pretty=format:%cI HEAD)

# Enforce timestamp on files to be included in the package
find "${outdir}" -exec touch -d "${date}" {} \;

echo "Generating $package file"

# Create tar
tar cf - --sort=name --mode=a+rwX --owner=0 --group=0 --numeric-owner "$outdir" > ${package}.tar || exit 1

echo "Checking tar package membership"

if ! tar -tf "${package}.tar" | grep -Fxq "${outdir}/README.md"; then
  echo "Missing required file in source distribution: ${outdir}/README.md"
  exit 1
fi

if ! tar -tf "${package}.tar" | grep -Fxq "${outdir}/CONTRIBUTING.md"; then
  echo "Missing required file in source distribution: ${outdir}/CONTRIBUTING.md"
  exit 1
fi

if ! tar -tf "${package}.tar" | grep -Fxq "${outdir}/doc/postgis.xml"; then
  echo "Missing required file in source distribution: ${outdir}/doc/postgis.xml"
  exit 1
fi

if tar -tf "${package}.tar" | grep -Fq "${outdir}/doc/development/"; then
  echo "Forbidden path in source distribution: ${outdir}/doc/development/"
  exit 1
fi

if tar -tf "${package}.tar" | grep -Fxq "${outdir}/AGENTS.md"; then
  echo "Forbidden file in source distribution: ${outdir}/AGENTS.md"
  exit 1
fi

if tar -tf "${package}.tar" | grep -Fxq "${outdir}/doc/skills/"; then
  echo "Forbidden path in source distribution: ${outdir}/doc/skills/"
  exit 1
fi

# Compress
gzip -n9 < ${package}.tar > ${package} || exit 1
rm ${package}.tar

echo "Generating ${package}.md5 file"
md5sum "${package}" > ${package}.md5

#echo "Cleaning up"
#rm -Rf "$outdir"
