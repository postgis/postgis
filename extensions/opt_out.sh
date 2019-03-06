#!/bin/sh

test -n "$1" || {
  echo "Usage: $0 <dbname>"
  exit 1
}

db="$1"

psql -XtA ${db} <<'EOF' | psql -XtA ${db}

-----------------
-- for sfcgal
-----------------

SELECT 'ALTER EXTENSION ' || extname || ' DROP ' || regexp_replace(
    regexp_replace(pg_catalog.pg_describe_object(d.classid, d.objid, 0), E'cast from (.*) to (.*)', E'cast\(\\1 as \\2\)'),
    E'(.*) for access method (.*)', E'\\1 using \\2') || ';' AS sqladd
FROM pg_catalog.pg_depend AS d
INNER JOIN pg_extension AS e ON (d.refobjid = e.oid)
WHERE d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass
AND deptype = 'e' AND e.extname = 'postgis_sfcgal'
ORDER BY sqladd;

SELECT 'DROP EXTENSION postgis_sfcgal;';

-----------------
-- for raster
-----------------

SELECT 'ALTER EXTENSION ' || extname || ' DROP ' || regexp_replace(
    regexp_replace(pg_catalog.pg_describe_object(d.classid, d.objid, 0), E'cast from (.*) to (.*)', E'cast\(\\1 as \\2\)'),
    E'(.*) for access method (.*)', E'\\1 using \\2') || ';' AS sqladd
FROM pg_catalog.pg_depend AS d
INNER JOIN pg_extension AS e ON (d.refobjid = e.oid)
WHERE d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass
AND deptype = 'e' AND e.extname = 'postgis_raster'
ORDER BY sqladd;

SELECT 'DROP EXTENSION postgis_raster;';

-----------------
-- for topology
-----------------

SELECT 'ALTER EXTENSION ' || extname || ' DROP ' || regexp_replace(
    regexp_replace(pg_catalog.pg_describe_object(d.classid, d.objid, 0), E'cast from (.*) to (.*)', E'cast\(\\1 as \\2\)'),
    E'(.*) for access method (.*)', E'\\1 using \\2') || ';' AS sqladd
FROM pg_catalog.pg_depend AS d
INNER JOIN pg_extension AS e ON (d.refobjid = e.oid)
WHERE d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass
AND deptype = 'e' AND e.extname = 'postgis_topology'
ORDER BY sqladd;

SELECT 'DROP EXTENSION postgis_topology;';

--------------------
-- for postgis core
--------------------

SELECT 'ALTER EXTENSION ' || extname || ' DROP ' || regexp_replace(
    regexp_replace(pg_catalog.pg_describe_object(d.classid, d.objid, 0), E'cast from (.*) to (.*)', E'cast\(\\1 as \\2\)'),
    E'(.*) for access method (.*)', E'\\1 using \\2') || ';' AS sqladd
FROM pg_catalog.pg_depend AS d
INNER JOIN pg_extension AS e ON (d.refobjid = e.oid)
WHERE d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass
AND deptype = 'e' AND e.extname = 'postgis'
ORDER BY sqladd;

SELECT 'DROP EXTENSION postgis;';

EOF
