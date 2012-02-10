--$Id$
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://www.postgis.org
--
-- Copyright (C) 2010, 2011 Regina Obe and Leo Hsu 
-- Paragon Corporation
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Regina Obe and Leo Hsu <lr@pcorp.us>
--  
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SET search_path TO tiger,public;
CREATE OR REPLACE FUNCTION create_census_base_tables() 
	RETURNS text AS
$$
DECLARE 
BEGIN
IF NOT EXISTS(SELECT table_name FROM information_schema.tables WHERE table_schema = 'tiger' AND table_name = 'tract') THEN
	-- census block group/tracts parent tables not created yet -- create them
	CREATE TABLE tract
	(
	  gid serial NOT NULL PRIMARY KEY,
	  statefp varchar(2),
	  countyfp varchar(3),
	  tractce varchar(6),
	  name varchar(7),
	  namelsad varchar(20),
	  mtfcc varchar(5),
	  funcstat varchar(1),
	  aland double precision,
	  awater double precision,
	  intptlat varchar(11),
	  intptlon varchar(12),
	  geom geometry,
	  CONSTRAINT enforce_dims_geom CHECK (st_ndims(geom) = 2),
	  CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geom) = 'MULTIPOLYGON'::text OR geom IS NULL),
	  CONSTRAINT enforce_srid_geom CHECK (st_srid(geom) = 4269)
	);
	COMMENT ON TABLE tiger.tract IS 'census tracts';
	
	CREATE TABLE tabblock
	(
	  gid serial NOT NULL PRIMARY KEY,
	  statefp varchar(2),
	  countyfp varchar(3),
	  tractce varchar(6),
	  blockce varchar(4),
	  name varchar(10),
	  mtfcc varchar(5),
	  ur varchar(1),
	  uace varchar(5),
	  funcstat varchar(1),
	  aland double precision,
	  awater double precision,
	  intptlat varchar(11),
	  intptlon varchar(12),
	  geom geometry,
	  CONSTRAINT enforce_dims_geom CHECK (st_ndims(geom) = 2),
	  CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geom) = 'MULTIPOLYGON'::text OR geom IS NULL),
	  CONSTRAINT enforce_srid_geom CHECK (st_srid(geom) = 4269)
	);
	COMMENT ON TABLE tiger.tabblock IS 'census blocks';
	
	CREATE TABLE bg
	(
	  gid serial NOT NULL PRIMARY KEY,
	  statefp varchar(2),
	  countyfp varchar(3),
	  tractce varchar(6),
	  blkgrpce varchar(1),
	  namelsad varchar(13),
	  mtfcc varchar(5),
	  funcstat varchar(1),
	  aland double precision,
	  awater double precision,
	  intptlat varchar(11),
	  intptlon varchar(12),
	  geom geometry,
	  CONSTRAINT enforce_dims_geom CHECK (st_ndims(geom) = 2),
	  CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geom) = 'MULTIPOLYGON'::text OR geom IS NULL),
	  CONSTRAINT enforce_srid_geom CHECK (st_srid(geom) = 4269)
	);
	COMMENT ON TABLE tiger.bg IS 'block groups';
	RETURN 'Done creating census tract base tables';
ELSE 
	RETURN 'Tables already present';
END IF;
END
$$
language 'plpgsql';
ALTER FUNCTION tiger.create_census_base_tables() SET search_path=tiger,public;

CREATE OR REPLACE FUNCTION loader_generate_census(param_states text[], os text)
  RETURNS SETOF text AS
$$
SELECT create_census_base_tables();
SELECT 
	loader_macro_replace(
		replace(
			loader_macro_replace(declare_sect
				, ARRAY['staging_fold', 'state_fold','website_root', 'psql', 'state_abbrev', 'data_schema', 'staging_schema', 'state_fips'], 
				ARRAY[variables.staging_fold, s.state_fold, variables.website_root, platform.psql, s.state_abbrev, variables.data_schema, variables.staging_schema, s.state_fips::text]
			), '/', platform.path_sep) || '
	' || platform.wget || ' http://' || variables.website_root  || '/'
	|| state_fold || 
			'/ --no-parent --relative --recursive --level=2 --accept=zip,txt --mirror --reject=html
		' || platform.unzip_command ||
	'	
	' ||
	-- State level files
	array_to_string( ARRAY(SELECT loader_macro_replace(COALESCE(lu.pre_load_process || E'\n', '') || platform.loader || ' -' ||  lu.insert_mode || ' -s 4269 -g the_geom ' 
		|| CASE WHEN lu.single_geom_mode THEN ' -S ' ELSE ' ' END::text || ' -W "latin1" tl_' || variables.tiger_year || '_' || s.state_fips 
	|| '_' || lu.table_name || '.dbf tiger_staging.' || lower(s.state_abbrev) || '_' || lu.table_name || ' | '::text || platform.psql 
		|| COALESCE(E'\n' || 
			lu.post_load_process , '') , ARRAY['loader','table_name', 'lookup_name'], ARRAY[platform.loader, lu.table_name, lu.lookup_name ])
				FROM loader_lookuptables AS lu
				WHERE level_state = true AND load = true AND lookup_name IN('tract','bg','tabblock')
				ORDER BY process_order, lookup_name), E'\n') ::text 
	-- County Level files
	|| E'\n' ||
		array_to_string( ARRAY(SELECT loader_macro_replace(COALESCE(lu.pre_load_process || E'\n', '') || COALESCE(county_process_command || E'\n','')
				|| COALESCE(E'\n' ||lu.post_load_process , '') , ARRAY['loader','table_name','lookup_name'], ARRAY[platform.loader, lu.table_name, lu.lookup_name ]) 
				FROM loader_lookuptables AS lu
				WHERE level_county = true AND load = true AND lookup_name IN('tract','bg','tabblock')
				ORDER BY process_order, lookup_name), E'\n') ::text 
	, ARRAY['psql', 'data_schema','staging_schema', 'staging_fold', 'state_fold', 'website_root', 'state_abbrev','state_fips'], 
	ARRAY[platform.psql,  variables.data_schema, variables.staging_schema, variables.staging_fold, s.state_fold,variables.website_root, s.state_abbrev, s.state_fips::text])
			AS shell_code
FROM loader_variables As variables
		CROSS JOIN (SELECT name As state, abbrev As state_abbrev, lpad(st_code::text,2,'0') As state_fips, 
			 lpad(st_code::text,2,'0') || '_' 
	|| replace(name, ' ', '_') As state_fold
FROM state_lookup) As s CROSS JOIN loader_platform As platform
WHERE $1 @> ARRAY[state_abbrev::text]      -- If state is contained in list of states input generate script for it
AND platform.os = $2  -- generate script for selected platform
;
$$
  LANGUAGE sql VOLATILE;
  
--update with census tract loading logic
DELETE FROM loader_lookuptables WHERE lookup_name IN('tract','tabblock','bg');           
INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process, columns_exclude )
VALUES(10, 'tract', 'tract', true, true, false,false, 'a', 
'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${table_name}(CONSTRAINT pk_${state_abbrev}_${table_name} PRIMARY KEY (gid)) INHERITS(${table_name});" ',
'${psql} -c "UPDATE ${data_schema}.${state_abbrev}_${table_name} SET statefp = ''${state_fips}''  WHERE statefp IS NULL;"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_snd_name ON ${data_schema}.${state_abbrev}_${table_name} USING btree (soundex(name));"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_lname ON ${data_schema}.${state_abbrev}_${table_name} USING btree (lower(name));"
${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${lookup_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(geom);"
${psql} -c "vacuum analyze ${data_schema}.${state_abbrev}_${lookup_name};"', ARRAY['gid','statefp']);

INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process, columns_exclude )
VALUES(11, 'tabblock', 'tabblock', true, true, false,false, 'a', 
'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${table_name}(CONSTRAINT pk_${state_abbrev}_${table_name} PRIMARY KEY (gid)) INHERITS(${table_name});" ',
'${psql} -c "UPDATE ${data_schema}.${state_abbrev}_${table_name} SET statefp = ''${state_fips}''  WHERE statefp IS NULL;"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_snd_name ON ${data_schema}.${state_abbrev}_${table_name} USING btree (soundex(name));"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_lname ON ${data_schema}.${state_abbrev}_${table_name} USING btree (lower(name));"
${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${lookup_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(geom);"
${psql} -c "vacuum analyze ${data_schema}.${state_abbrev}_${lookup_name};"', ARRAY['gid','statefp']);

INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process, columns_exclude )
VALUES(12, 'bg', 'bg', true, true, false,false, 'a', 
'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${table_name}(CONSTRAINT pk_${state_abbrev}_${table_name} PRIMARY KEY (gid)) INHERITS(${table_name});" ',
'${psql} -c "UPDATE ${data_schema}.${state_abbrev}_${table_name} SET statefp = ''${state_fips}''  WHERE statefp IS NULL;"
${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${lookup_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(geom);"
${psql} -c "vacuum analyze ${data_schema}.${state_abbrev}_${lookup_name};"', ARRAY['gid','statefp']);
