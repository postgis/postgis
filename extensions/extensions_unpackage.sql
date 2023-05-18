\set QUIET
BEGIN;
set client_min_messages to WARNING;
SELECT 'ALTER EXTENSION ' || extname || ' DROP ' ||
  regexp_replace(
    regexp_replace(
      pg_catalog.pg_describe_object(d.classid, d.objid, 0),
      'cast from (.*) to (.*)',
      'cast(\1 as \2)'
    ),
    '(.*) for access method (.*)',
    '\1 using \2'
  ) || ';'
FROM pg_catalog.pg_depend AS d
INNER JOIN pg_extension AS e ON (d.refobjid = e.oid)
WHERE d.refclassid = 'pg_catalog.pg_extension'::pg_catalog.regclass
AND deptype = 'e' AND e.extname in (
	'postgis_sfcgal',
	'postgis_raster',
	'postgis_topology',
	'postgis'
)
ORDER BY length(e.extname) DESC
\gexec

DROP EXTENSION IF EXISTS postgis_sfcgal;
DROP EXTENSION IF EXISTS postgis_raster;
DROP EXTENSION IF EXISTS postgis_topology;
DROP EXTENSION IF EXISTS postgis;
SELECT 'PostGIS objects unpackaged';
COMMIT;
