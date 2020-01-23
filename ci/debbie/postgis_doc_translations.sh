#!/usr/bin/env bash
set -e
## begin variables passed in by jenkins

# export PG_VER=9.2
# export PGPORT=8442
# export OS_BUILD=64
# export POSTGIS_MAJOR_VERSION=2
# export POSTGIS_MINOR_VERSION=2
# export POSTGIS_MICRO_VERSION=0dev
# export JENKINS_HOME=/var/lib/jenkins/workspace
# export GEOS_VER=3.4.3
# export GDAL_VER=2.0
# export MAKE_GARDEN=1
# export MAKE_EXTENSION=1

## end variables passed in by jenkins

export PROJECTS=${JENKINS_HOME}/workspace
export PGPATH=${PROJECTS}/pg/rel/pg${PG_VER}w${OS_BUILD}
export PATH=${PATH}:${PROJECTS}/gdal/rel-${GDAL_VER}w${OS_BUILD}/bin
export POSTGIS_SVN_REVISION=${SVN_REVISION}
echo $PATH
cd ${WORKSPACE}/docs/branches/${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}_translations

sh autogen.sh

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
cd doc
make update-pot
make pull-tx

mv postgis.xml postgis.xml.orig
sed -e "s:</title>:</title><subtitle><subscript>SVN Revision (<emphasis>${POSTGIS_SVN_REVISION}</emphasis>)</subscript></subtitle>:" postgis.xml.orig > postgis.xml

make check-localized

#make pdf
rm -rf images
mkdir images
cp html/images/* images 
#make epub
#make -e chunked-html 2>&1 | tee -a doc-errors.log
#make update-po
make html-localized
# make -C po/es/ local-html
# make -C po/fr/ local-html
# make -C po/it_IT/ local-html
# make -C po/pt_BR/ local-html
# make -C po/pl/ local-html
# make -C po/ko_KR/ local-html

cp -R html/*.*  /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
cp -R html/images/* /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}/images
chmod -R 755 /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}

#add back make pdf but after html copy so will work even if pdf generation fails
make pdf-localized
cp -R po/*/*.pdf /var/www/postgis_stuff/
chmod -R 755 /var/www/postgis_docs/manual-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}
chmod -R 755 /var/www/postgis_stuff/postgis-${POSTGIS_MAJOR_VERSION}.${POSTGIS_MINOR_VERSION}*.pdf
