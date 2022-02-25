#!/bin/sh

TMPDIR="/tmp/postgis_check_distclean_$$"

cleanup()
{
  #echo "${TMPDIR} has things"
  rm -rf ${TMPDIR}
}

trap 'cleanup' 0

mkdir -p $TMPDIR


find . -type f | sort > ${TMPDIR}/leftover_files_after_distclean
cat <<EOF > ${TMPDIR}/leftover_files_after_distclean.expected
./doc/postgis_comments.sql
./doc/raster_comments.sql
./doc/sfcgal_comments.sql
./doc/tiger_geocoder_comments.sql
./doc/topology_comments.sql
./liblwgeom/lwin_wkt_lex.c
./liblwgeom/lwin_wkt_parse.c
./liblwgeom/lwin_wkt_parse.h
./postgis_revision.h
EOF

fgrep -vf \
  ${TMPDIR}/leftover_files_after_distclean.expected \
  ${TMPDIR}/leftover_files_after_distclean > \
  ${TMPDIR}/unexpected_leftovers

if test $(cat ${TMPDIR}/unexpected_leftovers | wc -l) != 0; then
  echo "Unexpected left over files after distclean:" >&2
  cat ${TMPDIR}/unexpected_leftovers >&2
  false
fi
