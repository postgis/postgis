#!/usr/bin/env bash
export PG_VER=9.6
# export PGPORT=8442
export OS_BUILD=64
#this is passed in via postgis_make_dist.sh via jenkins
#export reference=
export PROJECTS=/var/lib/jenkins/workspace
export GEOS_VER=3.6
export GDAL_VER=2.2
export WEB_DIR=/var/www/postgis_stuff
export PGPATH=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}
export PATH="${PGPATH}/bin:$PATH"
export LD_LIBRARY_PATH="${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/lib:${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/lib:${PGPATH}/lib"
./autogen.sh



POSTGIS_MAJOR_VERSION=`grep ^POSTGIS_MAJOR_VERSION Version.config | cut -d= -f2`
POSTGIS_MINOR_VERSION=`grep ^POSTGIS_MINOR_VERSION Version.config | cut -d= -f2`
POSTGIS_MICRO_VERSION=`grep ^POSTGIS_MICRO_VERSION Version.config | cut -d= -f2`

chmod -R 755 /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
echo $PATH

#sh autogen.sh

if [ -f GNUMakefile ]; then
  make distclean
fi

#  --with-gdalconfig=${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/bin/gdal-config
#  --without-raster

CPPFLAGS="-I${PGPATH}/include"  \
LDFLAGS="-L${PGPATH}/lib"  ./configure \
  --with-pgconfig=${PGPATH}/bin/pg_config \
  --with-geosconfig=${PROJECTS}/geos/rel-${GEOS_VER}w${OS_BUILD}/bin/geos-config \
  --without-raster --without-wagyu
make clean

# generating postgis_revision.h in case hasn't been generated
if test -f utils/repo_revision.pl; then
	echo "Generating postgis_revision.h"
	perl utils/repo_revision.pl
fi
export VREV="`cat postgis_revision.h | awk '{print $3}'`"
echo "SVN is ${VREV}"
cd doc



#sed -e "s:</title>:</title><subtitle><subscript>SVN Revision (<emphasis>${POSTGIS_SVN_REVISION}</emphasis>)</subscript></subtitle>:" postgis.xml.orig > postgis.xml

echo "Micro: $POSTGIS_MICRO_VERSION"
cp postgis.xml postgis.xml.orig #we for dev will inject stuff into file, so backup original
#inject a development time stamp if we are in development branch
if [[ "$POSTGIS_MICRO_VERSION" == *"dev"* ]]; then
  export GIT_TIMESTAMP=`git log -1 --pretty=format:%ct`
  export GIT_TIMESTAMP="`date -d @$GIT_TIMESTAMP`" #convert to UTC date
  echo "GIT_TIMESTAMP: ${GIT_TIMESTAMP}"
  export part_old="</title>"
  export part_new="</title><subtitle><subscript>DEV (<emphasis>$GIT_TIMESTAMP rev. $VREV </emphasis>)</subscript></subtitle>"
  sed -i 's,'"$part_old"','"$part_new"',' postgis.xml
fi

make pdf
rm -rf images
mkdir images
cp html/images/* images
make epub
make -e chunked-html-web 2>&1 | tee -a doc-errors.log

if [[ "$reference" == *"master"* ]]; then  #only do this for trunk because only trunk follows transifex
  make update-pot
  make pull-tx
  make -C po/it_IT/ local-html
  make -C po/pt_BR/ local-html
  make -C po/ja/ local-html
  make -C po/de/ local-html
  make -C po/ko_KR/ local-html
  #make pdf-localized
fi

package="doc-html-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}.tar.gz"

export outdir=html
tar -czf "$package" --exclude='.svn' --exclude='.git' --exclude='image_src' "$outdir"



cp postgis.xml.orig postgis.xml
mkdir -p /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
mkdir -p /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}/images
cp -R html/*.*  /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
cp -R html/images/* /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}/images
chmod -R 755 /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
cp -R *.pdf /var/www/postgis_stuff/
cp -R *.epub /var/www/postgis_stuff/
cp -R $package /var/www/postgis_stuff/

if [[ "$POSTGIS_MICRO_VERSION" == *"dev"* ]]; then #rename the files without the micro if it's a development branch
  mv /var/www/postgis_stuff/doc-html-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}.tar.gz /var/www/postgis_stuff/doc-html-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.tar.gz
  mv /var/www/postgis_stuff/postgis-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}.pdf /var/www/postgis_stuff/postgis-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.pdf
  mv /var/www/postgis_stuff/postgis-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.${POSTGIS_MICRO_VERSION}.epub /var/www/postgis_stuff/postgis-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}.epub
fi
