#!/bin/sh
export TESTSITE="http://postgis.net/extra/test-data"
if [ -z "$DOWNLOAD_DATADIR" ]; then
	DOWNLOAD_DATADIR=$(dirname "$0")/download_data
fi

mkdir -p $DOWNLOAD_DATADIR
cd $DOWNLOAD_DATADIR
echo $DOWNLOAD_DATADIR

wget -nc ${TESTSITE}/tiger_national.sql.bz2 -O tiger_national.sql.bz2
wget -nc ${TESTSITE}/tiger_dc.sql.bz2 -O tiger_dc.sql.bz2
wget -nc ${TESTSITE}/osm_china.sql.bz2 -O osm_china.sql.bz2
bzip2 -dk tiger_national.sql.bz2
bzip2 -dk tiger_dc.sql.bz2
bzip2 -dk osm_china.sql.bz2
echo "Done downloading data"
export DOWNLOAD_DATADIR
