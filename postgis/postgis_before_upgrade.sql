-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.net
--
-- Copyright (C) 2011-2012 Sandro Santilli <strk@kbt.io>
-- Copyright (C) 2010-2013 Regina Obe <lr@pcorp.us>
-- Copyright (C) 2009      Paul Ramsey <pramsey@cleverelephant.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- This file contains drop commands for obsoleted items that need
-- to be dropped _before_ upgrade of old functions.
-- Changes to this file affect postgis_upgrade*.sql script.
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


-- Helper function to drop functions when they match the full signature
-- Requires schema, name and __exact arguments__ as extracted from pg_catalog.pg_get_function_arguments
-- You can extract the old function arguments using a query like:
-- SELECT  p.oid as oid,
--                 n.nspname as schema,
--                 p.proname as name,
--                 pg_catalog.pg_get_function_arguments(p.oid) as arguments
--         FROM pg_catalog.pg_proc p
--         LEFT JOIN pg_catalog.pg_namespace n ON n.oid = p.pronamespace
--         WHERE
--                 pg_catalog.LOWER(n.nspname) = pg_catalog.LOWER('public') AND
--                 pg_catalog.LOWER(p.proname) = pg_catalog.LOWER('ST_AsGeoJson')
--         ORDER BY 1, 2, 3, 4;
CREATE OR REPLACE FUNCTION _postgis_drop_function_if_needed(
	function_name text,
	function_arguments text) RETURNS void AS $$
DECLARE
	sql_drop text;
	postgis_namespace OID;
	matching_function REGPROCEDURE;
BEGIN

	-- Fetch install namespace for PostGIS
	SELECT n.oid
	FROM pg_catalog.pg_proc p
	JOIN pg_catalog.pg_namespace n ON p.pronamespace = n.oid
	WHERE proname = 'postgis_full_version'
	INTO postgis_namespace;

	-- Find a function matching the given signature
	SELECT oid
	FROM pg_catalog.pg_proc p
	WHERE pronamespace = postgis_namespace
	AND pg_catalog.LOWER(p.proname) = pg_catalog.LOWER(function_name)
	AND pg_catalog.pg_function_is_visible(p.oid)
	AND pg_catalog.LOWER(pg_catalog.pg_get_function_identity_arguments(p.oid)) ~ pg_catalog.LOWER(function_arguments)
	INTO matching_function;

	IF matching_function IS NOT NULL THEN
		sql_drop := 'DROP FUNCTION ' || matching_function;
		RAISE DEBUG 'SQL query: %', sql_drop;
		BEGIN
			EXECUTE sql_drop;
		EXCEPTION
			WHEN OTHERS THEN
				RAISE EXCEPTION 'Could not drop function %. You might need to drop dependant objects. Postgres error: %', function_name, SQLERRM;
		END;
	END IF;

END;
$$ LANGUAGE plpgsql;


-- FUNCTION AddGeometryColumn signature dropped
-- (catalog_name character varying, schema_name character varying, table_name character varying, column_name character varying, new_srid integer, new_type character varying, new_dim integer, use_typmod boolean)
SELECT _postgis_drop_function_if_needed
	(
	'AddGeometryColumn',
	'catalog_name character varying, schema_name character varying, table_name character varying, column_name character varying, new_srid integer, new_type character varying, new_dim integer, use_typmod boolean'
	);

-- FUNCTION ST_AsX3D was changed to add versioning for 2.0
-- (geom geometry, prec integer, options integer)
SELECT _postgis_drop_function_if_needed
	(
	'ST_AsX3D',
	'geom geometry, prec integer, options integer'
	);

-- FUNCTION UpdateGeometrySRID changed the name of the args (http://trac.osgeo.org/postgis/ticket/1606) for 2.0
-- It changed the paramenter `new_srid` to `new_srid_in`
-- (catalogn_name character varying, schema_name character varying, table_name character varying, column_name character varying, new_srid integer)
-- Dropping it conditionally since the same signature still exists.
SELECT _postgis_drop_function_if_needed
	(
	'UpdateGeometrySRID',
	'catalogn_name character varying, schema_name character varying, table_name character varying, column_name character varying, new_srid integer'
	);


--deprecated and removed in 2.1
-- Hack to fix 2.0 naming
-- We can't just drop it since its bound to opclass
-- See ticket 2279 for why we need to do this
-- We can get rid of this DO code when 3.0 comes along
DO  language 'plpgsql' $$
BEGIN
	-- fix geometry ops --
	IF EXISTS(SELECT oprname from pg_operator where oprname = '&&' AND oprrest::text = 'geometry_gist_sel_2d') THEN
	--it is bound to old name, drop new, rename old to new, install will fix body of code
		DROP FUNCTION IF EXISTS gserialized_gist_sel_2d(internal, oid, internal, int4) ;
		ALTER FUNCTION geometry_gist_sel_2d(internal, oid, internal, int4) RENAME TO gserialized_gist_sel_2d;
	END IF;
	IF EXISTS(SELECT oprname from pg_operator where oprname = '&&' AND oprjoin::text = 'geometry_gist_joinsel_2d') THEN
	--it is bound to old name, drop new, rename old to new,  install will fix body of code
		DROP FUNCTION IF EXISTS gserialized_gist_joinsel_2d(internal, oid, internal, smallint) ;
		ALTER FUNCTION geometry_gist_joinsel_2d(internal, oid, internal, smallint) RENAME TO gserialized_gist_joinsel_2d;
	END IF;
	-- fix geography ops --
	IF EXISTS(SELECT oprname from pg_operator where oprname = '&&' AND oprrest::text = 'geography_gist_selectivity') THEN
	--it is bound to old name, drop new, rename old to new, install will fix body of code
		DROP FUNCTION IF EXISTS gserialized_gist_sel_nd(internal, oid, internal, int4) ;
		ALTER FUNCTION geography_gist_selectivity(internal, oid, internal, int4) RENAME TO gserialized_gist_sel_nd;
	END IF;

	IF EXISTS(SELECT oprname from pg_operator where oprname = '&&' AND oprjoin::text = 'geography_gist_join_selectivity') THEN
	--it is bound to old name, drop new, rename old to new, install will fix body of code
		DROP FUNCTION IF EXISTS gserialized_gist_joinsel_nd(internal, oid, internal, smallint) ;
		ALTER FUNCTION geography_gist_join_selectivity(internal, oid, internal, smallint) RENAME TO gserialized_gist_joinsel_nd;
	END IF;
END;
$$ ;


-- FUNCTION ST_AsLatLonText went from multiple signatures to a single one with defaults for 2.2.0
DROP FUNCTION IF EXISTS ST_AsLatLonText(geometry); -- Does not conflict

SELECT _postgis_drop_function_if_needed
	(
	'ST_AsLatLonText',
	'geometry, text'
	);

-- FUNCTION ST_LineCrossingDirection changed argument names in 3.0
-- Was (geom1 geometry, geom2 geometry) and now (line1 geometry, line2 geometry)
SELECT _postgis_drop_function_if_needed
	(
	'ST_LineCrossingDirection',
	'geom1 geometry, geom2 geometry'
	);

-- FUNCTION _st_linecrossingdirection changed argument names in 3.0
-- Was (geom1 geometry, geom2 geometry) and now (line1 geometry, line2 geometry)
SELECT _postgis_drop_function_if_needed
	(
	'_ST_LineCrossingDirection',
	'geom1 geometry, geom2 geometry'
	);

-- FUNCTION ST_AsGeoJson changed argument names
-- (pretty_print => pretty_bool) in 3.0alpha4
SELECT _postgis_drop_function_if_needed
	(
	'ST_AsGeoJson',
	$args$r record, geom_column text, maxdecimaldigits integer, pretty_print boolean$args$
	);

-- FUNCTION _st_orderingequals changed argument names in 3.0
-- Was (GeometryA geometry, GeometryB geometry) and now (geom1 geometry, geom2 geometry)
SELECT _postgis_drop_function_if_needed
	(
	'_st_orderingequals',
	'GeometryA geometry, GeometryB geometry'
	);

-- FUNCTION st_orderingequals changed argument names in 3.0
-- Was (GeometryA geometry, GeometryB geometry) and now (geom1 geometry, geom2 geometry)
SELECT _postgis_drop_function_if_needed
	(
	'st_orderingequals',
	'GeometryA geometry, GeometryB geometry'
	);

-- FUNCTION st_tileenvelope added a new default argument in 3.1
SELECT _postgis_drop_function_if_needed
    (
    'st_tileenvelope',
    'zoom integer, x integer, y integer, bounds geometry'
    );


-- FUNCTION st_buffer changed to add defaults in 3.0
-- This signature was superseeded
DROP FUNCTION IF EXISTS st_buffer(geometry, double precision); -- Does not conflict

-- FUNCTION ST_CurveToLine changed to add defaults in 2.5
-- These signatures were superseeded
DROP FUNCTION IF EXISTS ST_CurveToLine(geometry, integer); -- Does not conflict
DROP FUNCTION IF EXISTS ST_CurveToLine(geometry); -- Does not conflict

-- geometry_columns changed parameter types so we verify if it needs to be dropped
-- We check the catalog to see if the view (geometry_columns) has a column
-- with name `f_table_schema` and type `character varying(256)` as it was
-- changed to type `name` in 2.2
DO  language 'plpgsql' $$
BEGIN
	IF EXISTS
		(
			WITH oids AS
			(
				SELECT c.oid as oid,
					n.nspname,
					c.relname
					FROM pg_catalog.pg_class c
					LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
					WHERE c.relname = 'geometry_columns' AND
						n.nspname = 'public'
					AND pg_catalog.pg_table_is_visible(c.oid)
					ORDER BY 2, 3

			),
			name_attribute AS
			(
				SELECT  a.attname as attname,
						pg_catalog.format_type(a.atttypid, a.atttypmod) as format_type
						FROM pg_catalog.pg_attribute a, oids
						WHERE a.attrelid = oids.oid AND a.attnum > 0 AND NOT a.attisdropped
						ORDER BY a.attnum
			)
			SELECT attname, format_type
			FROM name_attribute
			WHERE attname = 'f_table_schema' AND format_type = 'character varying(256)'
		)
		THEN
			DROP VIEW geometry_columns;
		END IF;
END;
$$;


-- DROP auxiliar function (created above)
DROP FUNCTION _postgis_drop_function_if_needed(text, text);

