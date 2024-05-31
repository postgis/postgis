#!/bin/sh

WORKDIR=$PWD/releases-check
API=https://git.osgeo.org/gitea/api/v1
DOWNLOAD_BASE=https://download.osgeo.org/postgis/source
MD5_BASE=https://postgis.net/stuff
OLDEST_MAJ=3
OLDEST_MIN=0

mkdir -p ${WORKDIR}
cd ${WORKDIR}

echo "Fetching list of supported releases"
curl -s ${API}/repos/postgis/postgis/tags > tags.json
jq -r .[].name tags.json |
  grep "^${OLDEST_MAJ}\.${OLDEST_MIN}" |
  grep -v [a-z] > checked_releases.txt

while read REL; do
  relname=postgis-${REL}.tar.gz
  md5name=${relname}.md5

  echo -n "Checking ${relname} ... "
  curl -s ${DOWNLOAD_BASE}/${relname} -o ${relname} || {
    echo "unable to download ${relname} from ${DOWNLOAD_BASE}"
    continue
  }
  curl -s ${MD5_BASE}/${md5name} -o ${md5name} || {
    echo "unable to download ${md5name} from ${MD5_BASE}"
    continue
  }
  md5sum ${relname} | diff - ${md5name} > /dev/null 2>&1 || {
    echo "MD5 mismatch"
    continue
  }
  echo "OK"
done < checked_releases.txt
