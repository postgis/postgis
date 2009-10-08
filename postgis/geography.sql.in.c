---------------------------------------------------------------------------
-- $Id$
--
-- PostGIS - Spatial Types for PostgreSQL
-- Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
---------------------------------------------------------------------------

BEGIN;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_typmod_in(cstring[])
	RETURNS integer
	AS 'MODULE_PATHNAME','geography_typmod_in'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_typmod_out(integer)
	RETURNS cstring
	AS 'MODULE_PATHNAME','geography_typmod_out'
	LANGUAGE 'C' IMMUTABLE STRICT; 
	
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_in(cstring, oid, integer)
	RETURNS geography
	AS 'MODULE_PATHNAME','geography_in'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_out(geography)
	RETURNS cstring
	AS 'MODULE_PATHNAME','geography_out'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_analyze(internal)
	RETURNS bool
	AS 'MODULE_PATHNAME','geography_analyze'
	LANGUAGE 'C' VOLATILE STRICT; 

-- Availability: 1.5.0
CREATE TYPE geography (
	internallength = variable,
	input = geography_in,
	output = geography_out,
	typmod_in = geography_typmod_in,
	typmod_out = geography_typmod_out,
	analyze = geography_analyze,
	storage = main,
	alignment = double
);

--
-- GIDX type is used by the GiST index bindings. 
-- In/out functions are stubs, as all access should be internal.
---
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION gidx_in(cstring)
	RETURNS gidx
	AS 'MODULE_PATHNAME','gidx_in'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION gidx_out(gidx)
	RETURNS cstring
	AS 'MODULE_PATHNAME','gidx_out'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- Availability: 1.5.0
CREATE TYPE gidx (
	internallength = variable,
	input = gidx_in,
	output = gidx_out,
	storage = plain,
	alignment = double
);


-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography(geography, integer, boolean)
	RETURNS geography
	AS 'MODULE_PATHNAME','geography_enforce_typmod'
	LANGUAGE 'C' IMMUTABLE STRICT; 

-- Availability: 1.5.0
CREATE CAST (geography AS geography) WITH FUNCTION geography(geography, integer, boolean) AS IMPLICIT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_AsText(geography)
	RETURNS text
	AS 'MODULE_PATHNAME','geography_as_text'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Availability: 1.5.0 - this is just a hack to prevent unknown from causing ambiguous name because of geography
CREATE OR REPLACE FUNCTION ST_AsText(text)
	RETURNS text AS
	$$ SELECT ST_AsText($1::geometry);  $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_GeographyFromText(text)
	RETURNS geography
	AS 'MODULE_PATHNAME','geography_from_text'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_AsBinary(geography)
	RETURNS bytea
	AS 'MODULE_PATHNAME','geography_as_binary'
	LANGUAGE 'C' IMMUTABLE STRICT;
	
-- Availability: 1.5.0 - this is just a hack to prevent unknown from causing ambiguous name because of geography
CREATE OR REPLACE FUNCTION ST_AsBinary(text)
	RETURNS bytea AS
	$$ SELECT ST_AsBinary($1::geometry);  $$
	LANGUAGE 'sql' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_GeographyFromBinary(bytea)
	RETURNS geography
	AS 'MODULE_PATHNAME','geography_from_binary'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_typmod_dims(integer)
	RETURNS integer
	AS 'MODULE_PATHNAME','geography_typmod_dims'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_typmod_srid(integer)
	RETURNS integer
	AS 'MODULE_PATHNAME','geography_typmod_srid'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_typmod_type(integer)
	RETURNS text
	AS 'MODULE_PATHNAME','geography_typmod_type'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE VIEW geography_columns AS
	SELECT
		current_database() AS f_table_catalog, 
		n.nspname AS f_table_schema, 
		c.relname AS f_table_name, 
		a.attname AS f_geography_column,
		geography_typmod_dims(a.atttypmod) AS coord_dimension,
		geography_typmod_srid(a.atttypmod) AS srid,
		geography_typmod_type(a.atttypmod) AS type
	FROM 
		pg_class c, 
		pg_attribute a, 
		pg_type t, 
		pg_namespace n
	WHERE c.relkind IN('r','v')
	AND t.typname = 'geography'
	AND a.attisdropped = false
	AND a.atttypid = t.oid
	AND a.attrelid = c.oid
	AND c.relnamespace = n.oid;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography(geometry)
	RETURNS geography
	AS 'MODULE_PATHNAME','geography_from_geometry'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE CAST (geometry AS geography) WITH FUNCTION geography(geometry) AS IMPLICIT;

-- ---------- ---------- ---------- ---------- ---------- ---------- ----------
-- GiST Support Functions
-- ---------- ---------- ---------- ---------- ---------- ---------- ----------

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_consistent(internal,geometry,int4) 
	RETURNS bool 
	AS 'MODULE_PATHNAME' ,'geography_gist_consistent'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_compress(internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME','geography_gist_compress'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_penalty(internal,internal,internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'geography_gist_penalty'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_picksplit(internal, internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'geography_gist_picksplit'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_union(bytea, internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'geography_gist_union'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_same(box2d, box2d, internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'geography_gist_same'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_decompress(internal) 
	RETURNS internal 
	AS 'MODULE_PATHNAME' ,'geography_gist_decompress'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_selectivity (internal, oid, internal, int4)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'geography_gist_selectivity'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_gist_join_selectivity(internal, oid, internal, smallint)
	RETURNS float8
	AS 'MODULE_PATHNAME', 'geography_gist_join_selectivity'
	LANGUAGE 'C';

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION geography_overlaps(geography, geography) 
	RETURNS boolean 
	AS 'MODULE_PATHNAME' ,'geography_overlaps'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OPERATOR && (
	LEFTARG = geography, RIGHTARG = geography, PROCEDURE = geography_overlaps,
	COMMUTATOR = '&&',
	RESTRICT = geography_gist_selectivity, 
	JOIN = geography_gist_join_selectivity
);


-- Availability: 1.5.0
CREATE OPERATOR CLASS gist_geography_ops
	DEFAULT FOR TYPE geography USING GIST AS
	STORAGE 	gidx,
	OPERATOR        3        &&	,
--	OPERATOR        6        ~=	,
--	OPERATOR        7        ~	,
--	OPERATOR        8        @	,
	FUNCTION        1        geography_gist_consistent (internal, geometry, int4),
	FUNCTION        2        geography_gist_union (bytea, internal),
	FUNCTION        3        geography_gist_compress (internal),
	FUNCTION        4        geography_gist_decompress (internal),
	FUNCTION        5        geography_gist_penalty (internal, internal, internal),
	FUNCTION        6        geography_gist_picksplit (internal, internal),
	FUNCTION        7        geography_gist_same (box2d, box2d, internal);

-- ---------- ---------- ---------- ---------- ---------- ---------- ----------
-- Export Functions
-- Availability: 1.5.0
-- ---------- ---------- ---------- ---------- ---------- ---------- ----------

--
-- SVG OUTPUT
--

-- ST_AsSVG(geography, precision, rel)
CREATE OR REPLACE FUNCTION ST_AsSVG(geography,int4,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','geography_as_svg'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- ST_AsSVG(geography, precision) / rel=0
CREATE OR REPLACE FUNCTION ST_AsSVG(geography,int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','geography_as_svg'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- ST_AsSVG(geography) / precision=15, rel=0
CREATE OR REPLACE FUNCTION ST_AsSVG(geography)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','geography_as_svg'
	LANGUAGE 'C' IMMUTABLE STRICT;
	

--
-- GML OUTPUT
--

-- _ST_AsGML(version, geography, precision, option)
CREATE OR REPLACE FUNCTION _ST_AsGML(int4, geography, int4, int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','geography_as_gml'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- ST_AsGML(geography, precision) / version=2 options=0
CREATE OR REPLACE FUNCTION ST_AsGML(geography, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, $2, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML(geography) / precision=15 version=2 options=0
CREATE OR REPLACE FUNCTION ST_AsGML(geography)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, 15, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML(version, geography) / precision=15 version=2 options=0
CREATE OR REPLACE FUNCTION ST_AsGML(int4, geography)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, 15, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML(version, geography, precision) / options = 0
CREATE OR REPLACE FUNCTION ST_AsGML(int4, geography, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, $3, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML (geography, precision, option) / version=2
CREATE OR REPLACE FUNCTION ST_AsGML(geography, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML(2, $1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGML(version, geography, precision, option)
CREATE OR REPLACE FUNCTION ST_AsGML(int4, geography, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGML($1, $2, $3, $4)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;



--
-- KML OUTPUT
--

-- _ST_AsKML(version, geography, precision)
CREATE OR REPLACE FUNCTION _ST_AsKML(int4, geography, int4)
	RETURNS TEXT
	AS 'MODULE_PATHNAME','geography_as_kml'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- AsKML(geography,precision) / version=2
CREATE OR REPLACE FUNCTION ST_AsKML(geography, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, $1, $2)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- AsKML(geography) / precision=15 version=2
CREATE OR REPLACE FUNCTION ST_AsKML(geography)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML(2, $1, 15)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsKML(version, geography) / precision=15 
CREATE OR REPLACE FUNCTION ST_AsKML(int4, geography)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, $2, 15)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsKML(version, geography, precision)
CREATE OR REPLACE FUNCTION ST_AsKML(int4, geography, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsKML($1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;



--
-- GeoJson Output
--

CREATE OR REPLACE FUNCTION _ST_AsGeoJson(int4, geography, int4, int4)
	RETURNS text
	AS 'MODULE_PATHNAME','geography_as_geojson'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- ST_AsGeoJson(geography, precision) / version=1 options=0
CREATE OR REPLACE FUNCTION ST_AsGeoJson(geography, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson(1, $1, $2, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(geography) / precision=15 version=1 options=0
CREATE OR REPLACE FUNCTION ST_AsGeoJson(geography)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson(1, $1, 15, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(version, geography) / precision=15 options=0
CREATE OR REPLACE FUNCTION ST_AsGeoJson(int4, geography)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson($1, $2, 15, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(version, geography, precision) / options=0
CREATE OR REPLACE FUNCTION ST_AsGeoJson(int4, geography, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson($1, $2, $3, 0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(geography, precision, options) / version=1
CREATE OR REPLACE FUNCTION ST_AsGeoJson(geography, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson(1, $1, $2, $3)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ST_AsGeoJson(version, geography, precision,options)
CREATE OR REPLACE FUNCTION ST_AsGeoJson(int4, geography, int4, int4)
	RETURNS TEXT
	AS 'SELECT _ST_AsGeoJson($1, $2, $3, $4)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- ---------- ---------- ---------- ---------- ---------- ---------- ----------
-- Measurement Functions
-- Availability: 1.5.0
-- ---------- ---------- ---------- ---------- ---------- ---------- ----------

-- Stop calculation and return immediately once distance is less than tolerance
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION _ST_Distance(geography, geography, float8)
	RETURNS float8
	AS 'MODULE_PATHNAME','geography_distance_sphere'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_Distance(geography, geography)
	RETURNS float8
	AS 'SELECT _ST_Distance($1, $2, 0.0)'
	LANGUAGE 'SQL' IMMUTABLE STRICT;

-- Only expands the bounding box, the actual geometry will remain unchanged, use with care.
-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION _ST_Expand(geography, float8)
	RETURNS geography
	AS 'MODULE_PATHNAME','geography_expand'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_DWithin(geography, geography, float8)
	RETURNS boolean
	AS 'SELECT $1 && _ST_Expand($2,$3) AND $2 && _ST_Expand($1,$3) AND _ST_Distance($1, $2, $3) < $3'
	LANGUAGE 'SQL' IMMUTABLE;

-- Availability: 1.5.0
CREATE OR REPLACE FUNCTION ST_Area(geography)
	RETURNS float8
	AS 'MODULE_PATHNAME','geography_area_sphere'
	LANGUAGE 'C' IMMUTABLE STRICT;

-- ---------- ---------- ---------- ---------- ---------- ---------- ----------

COMMIT;
