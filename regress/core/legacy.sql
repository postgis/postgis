--
-- These tests serve the purpose of ensuring compatibility with
-- old versions of postgis users.
--
-- Their use rely on loading the legacy.sql script.
-- This file also serves as a testcase for uninstall_legacy.sql
--

SET client_min_messages TO WARNING;
CREATE SCHEMA legacy_caller_path;
SET search_path TO public, legacy_caller_path;

WITH legacy_aliases(legacy_definition, modern_regprocedure, legacy_symbol) AS (
	VALUES
		('public.AsBinary(geometry)', 'st_asbinary(geometry)'::regprocedure, 'LWGEOM_asBinary'),
		('public.AsBinary(geometry, text)', 'st_asbinary(geometry,text)'::regprocedure, 'LWGEOM_asBinary'),
		('public.AsText(geometry)', 'st_astext(geometry)'::regprocedure, 'LWGEOM_asText'),
		('public.ndims(geometry)', 'st_ndims(geometry)'::regprocedure, 'LWGEOM_ndims'),
		('public.SetSRID(geometry, integer)', 'st_setsrid(geometry,integer)'::regprocedure, 'LWGEOM_set_srid'),
		('public.SRID(geometry)', 'st_srid(geometry)'::regprocedure, 'LWGEOM_get_srid')
)
SELECT pg_catalog.format(
	'CREATE OR REPLACE FUNCTION %s RETURNS %s AS %L, %L LANGUAGE c IMMUTABLE STRICT',
	legacy_definition,
	pg_catalog.format_type(pg_proc.prorettype, NULL),
	pg_proc.probin,
	legacy_symbol
)
FROM legacy_aliases
JOIN pg_catalog.pg_proc
	ON pg_proc.oid = legacy_aliases.modern_regprocedure
JOIN pg_catalog.pg_language
	ON pg_language.oid = pg_proc.prolang
WHERE pg_language.lanname = 'c'
\gexec

SELECT 'legacy_old_catalog_aliases', count(*)
FROM pg_catalog.pg_proc
JOIN pg_catalog.pg_language ON pg_language.oid = pg_proc.prolang
WHERE pg_proc.oid IN (
	'asbinary(geometry)'::regprocedure,
	'asbinary(geometry,text)'::regprocedure,
	'astext(geometry)'::regprocedure,
	'ndims(geometry)'::regprocedure,
	'setsrid(geometry,integer)'::regprocedure,
	'srid(geometry)'::regprocedure
)
AND pg_language.lanname = 'c';

SELECT 'legacy_old_catalog_calls',
	public.AsText(public.ST_Point(1, 2)) = 'POINT(1 2)'
	AND octet_length(public.AsBinary(public.ST_Point(1, 2))) > 0
	AND octet_length(public.AsBinary(public.ST_Point(1, 2), 'NDR')) > 0
	AND public.ndims(public.ST_Point(1, 2)) = 2
	AND public.SRID(public.SetSRID(public.ST_Point(1, 2), 4326)) = 4326;

\cd :scriptdir
\i legacy.sql

SELECT 'legacy_load_search_path', pg_catalog.current_setting('search_path');

SELECT 'estimated_extent_properties', count(*)
FROM pg_catalog.pg_proc
WHERE oid IN (
	'estimated_extent(text,text,text)'::regprocedure,
	'estimated_extent(text,text)'::regprocedure
)
AND prosecdef
AND provolatile = 'i';

SELECT 'legacy_wrapper_search_path', count(*)
FROM pg_catalog.pg_proc
WHERE oid IN (
	'asbinary(geometry)'::regprocedure,
	'asbinary(geometry,text)'::regprocedure,
	'astext(geometry)'::regprocedure,
	'geomfromtext(text,integer)'::regprocedure,
	'geomfromtext(text)'::regprocedure,
	'ndims(geometry)'::regprocedure,
	'setsrid(geometry,integer)'::regprocedure,
	'srid(geometry)'::regprocedure,
	'st_asbinary(text)'::regprocedure,
	'st_astext(bytea)'::regprocedure
)
AND proconfig = ARRAY[
	'search_path=' || pg_catalog.quote_ident(pg_catalog.current_schema()) || ', pg_catalog'
];

SELECT 'legacy_rewritten_c_aliases', count(*)
FROM pg_catalog.pg_proc
JOIN pg_catalog.pg_language ON pg_language.oid = pg_proc.prolang
WHERE pg_proc.oid IN (
	'asbinary(geometry)'::regprocedure,
	'asbinary(geometry,text)'::regprocedure,
	'astext(geometry)'::regprocedure,
	'ndims(geometry)'::regprocedure,
	'setsrid(geometry,integer)'::regprocedure,
	'srid(geometry)'::regprocedure
)
AND pg_language.lanname = 'sql'
AND probin IS NULL
AND prosrc LIKE '%_postgis_deprecate%'
AND proconfig = ARRAY[
	'search_path=' || pg_catalog.quote_ident(pg_catalog.current_schema()) || ', pg_catalog'
];

SELECT 'Starting up MapServer/Geoserver tests...';
-- Set up the data table
SELECT 'Setting up the data table...';
CREATE TABLE wmstest ( id INTEGER );
SELECT AddGeometryColumn( 'wmstest', 'pt', 4326, 'POLYGON', 2 );
INSERT INTO wmstest SELECT lon * 100 + lat AS id, st_setsrid(st_buffer(st_makepoint(lon, lat),1.0),4326) AS pt
FROM (select lon, generate_series(-80,80, 5) AS lat FROM (SELECT generate_series(-175, 175, 5) AS lon) AS sq1) AS sq2;
ALTER TABLE wmstest add PRIMARY KEY ( id );
CREATE INDEX wmstest_geomidx ON wmstest using gist ( pt );

-- Geoserver 2.0 NG tests
SELECT 'Running Geoserver 2.0 NG tests...';
-- Run a Geoserver 2.0 NG metadata query
SELECT 'Geoserver1', TYPE FROM GEOMETRY_COLUMNS WHERE F_TABLE_SCHEMA = 'public' AND F_TABLE_NAME = 'wmstest' AND F_GEOMETRY_COLUMN = 'pt';
SELECT 'Geoserver2', SRID FROM GEOMETRY_COLUMNS WHERE F_TABLE_SCHEMA = 'public' AND F_TABLE_NAME = 'wmstest' AND F_GEOMETRY_COLUMN = 'pt';
-- Run a Geoserver 2.0 NG WMS query
SELECT 'Geoserver3', "id",substr(encode(asBinary(force_2d("pt"),'XDR'),'base64'),0,16) as "pt" FROM "public"."wmstest" WHERE "pt" && GeomFromText('POLYGON ((-6.58216065979069 -0.7685569763184591, -6.58216065979069 0.911225433349509, -3.050569931030911 0.911225433349509, -3.050569931030911 -0.7685569763184591, -6.58216065979069 -0.7685569763184591))', 4326);
-- Run a Geoserver 2.0 NG KML query
SELECT 'Geoserver4', count(*) FROM "public"."wmstest" WHERE "pt" && GeomFromText('POLYGON ((-1.504017942347938 24.0332272532341, -1.504017942347938 25.99364254836741, 1.736833353559741 25.99364254836741, 1.736833353559741 24.0332272532341, -1.504017942347938 24.0332272532341))', 4326);
SELECT 'Geoserver5', "id",substr(encode(asBinary(force_2d("pt"),'XDR'),'base64'),0,16) as "pt" FROM "public"."wmstest" WHERE "pt" && GeomFromText('POLYGON ((-1.504017942347938 24.0332272532341, -1.504017942347938 25.99364254836741, 1.736833353559741 25.99364254836741, 1.736833353559741 24.0332272532341, -1.504017942347938 24.0332272532341))', 4326);
SELECT 'Geoserver6', "id",substr(encode(asBinary(force_2d("pt"),'XDR'),'base64'),0,16) as "pt" FROM "public"."wmstest" WHERE "pt" && GeomFromText('POLYGON ((-1.507182836191598 24.031312785172446, -1.507182836191598 25.995557016429064, 1.7399982474034008 25.995557016429064, 1.7399982474034008 24.031312785172446, -1.507182836191598 24.031312785172446))', 4326);

-- MapServer 5.4 tests
select 'MapServer1', attname from pg_attribute, pg_constraint, pg_class where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = 'wmstest' and pg_table_is_visible(pg_class.oid) and pg_constraint.conkey[2] is null;
SET client_min_messages TO ERROR;
select 'MapServer2', "id",substr(encode(AsBinary(force_collection(force_2d("pt")),'NDR'),'base64'),0,16) as geom,"id" from wmstest where pt && GeomFromText('POLYGON((-98.5 32,-98.5 39,-91.5 39,-91.5 32,-98.5 32))',find_srid('','wmstest','pt')) order by "id";
SET client_min_messages TO WARNING;

-- MapServer 5.6 tests
select * from wmstest where false limit 0;
select 'MapServer3', attname from pg_attribute, pg_constraint, pg_class where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = 'wmstest' and pg_table_is_visible(pg_class.oid) and pg_constraint.conkey[2] is null;
SET client_min_messages TO ERROR;
select 'MapServer4', "id",substr(encode(AsBinary(force_collection(force_2d("pt")),'NDR'),'hex'),0,16) as geom,"id" from wmstest where pt && GeomFromText('POLYGON((-98.5 32,-98.5 39,-91.5 39,-91.5 32,-98.5 32))',find_srid('','wmstest','pt')) order by "id";
SET client_min_messages TO WARNING;

-- Drop the data table
SELECT 'Removing the data table...';
DROP TABLE wmstest;
DELETE FROM geometry_columns WHERE f_table_name = 'wmstest' AND f_table_schema = 'public';
SELECT 'Done.';

-- test #1869 ST_AsBinary is not unique --
SELECT 1869 As ticket_id, ST_AsText(ST_AsBinary('POINT(1 2)'));

CREATE SCHEMA legacy_shadow;

CREATE FUNCTION legacy_shadow._postgis_deprecate(text, text, text)
RETURNS void
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow _postgis_deprecate called';
END;
$$;

CREATE FUNCTION legacy_shadow.st_asbinary(public.geometry)
RETURNS bytea
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow ST_AsBinary called';
END;
$$;

CREATE FUNCTION legacy_shadow.st_astext(public.geometry)
RETURNS text
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow ST_AsText(geometry) called';
END;
$$;

CREATE FUNCTION legacy_shadow.st_astext(bytea)
RETURNS text
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow ST_AsText(bytea) called';
END;
$$;

CREATE FUNCTION legacy_shadow.st_geomfromtext(text)
RETURNS public.geometry
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow ST_GeomFromText(text) called';
END;
$$;

CREATE FUNCTION legacy_shadow.st_geomfromtext(text, integer)
RETURNS public.geometry
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow ST_GeomFromText(text, integer) called';
END;
$$;

CREATE FUNCTION legacy_shadow.st_ndims(public.geometry)
RETURNS smallint
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow ST_NDims called';
END;
$$;

CREATE FUNCTION legacy_shadow.st_setsrid(public.geometry, integer)
RETURNS public.geometry
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow ST_SetSRID called';
END;
$$;

CREATE FUNCTION legacy_shadow.st_srid(public.geometry)
RETURNS integer
LANGUAGE plpgsql AS $$
BEGIN
	RAISE EXCEPTION 'legacy shadow ST_SRID called';
END;
$$;

SET client_min_messages TO ERROR;
SET search_path TO legacy_shadow, public;

SELECT 'legacy_search_path', public.AsText(public.ST_Point(1, 2)) = 'POINT(1 2)'
	AND octet_length(public.AsBinary(public.ST_Point(1, 2))) > 0
	AND public.SRID(public.SetSRID(public.GeomFromText('POINT(1 2)'), 4326)) = 4326
	AND public.SRID(public.GeomFromText('POINT(1 2)', 4326)) = 4326
	AND public.ndims(public.ST_Point(1, 2)) = 2
	AND octet_length(public.ST_AsBinary('POINT(1 2)')) > 0
	AND public.ST_AsText(public.ST_AsBinary(public.ST_Point(1, 2))) = 'POINT(1 2)';

SET search_path TO public;
SET client_min_messages TO WARNING;
DROP SCHEMA legacy_shadow CASCADE;

\i uninstall_legacy.sql

DROP SCHEMA legacy_caller_path;
