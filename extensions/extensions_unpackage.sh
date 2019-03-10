#!/bin/sh

test -n "$1" || {
  echo "Usage: $0 <dbname>"
  exit 1
}

db="$1"

EXTENSIONS="postgis_sfcgal postgis_raster postgis_topology postgis"

(
  for ext in $EXTENSIONS; do
    cat <<EOF

SELECT 'ALTER EXTENSION ' || extname || ' DROP ' ||
  regexp_replace(
    regexp_replace(
      pg_catalog.pg_describe_object(d.classid, d.objid, 0),
      E'cast from (.*) to (.*)',
      E'cast\\(\\\\1 as \\\\2\\)'
    ),
    E'(.*) for access method (.*)',
    E'\\\\1 using \\\\2'
  ) || ';' AS sqladd
FROM pg_catalog.pg_depend AS d
INNER JOIN pg_extension AS e ON (d.refobjid = e.oid)
WHERE d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass
AND deptype = 'e' AND e.extname = '${ext}' ORDER BY sqladd;

SELECT 'DROP EXTENSION IF EXISTS ${ext};';

EOF
  done
) | psql -XtA ${db}
