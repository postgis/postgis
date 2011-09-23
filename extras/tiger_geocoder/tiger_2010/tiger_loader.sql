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
BEGIN;
CREATE OR REPLACE FUNCTION loader_macro_replace(param_input text, param_keys text[],param_values text[]) 
RETURNS text AS
$$
	DECLARE var_result text = param_input;
	DECLARE var_count integer = array_upper(param_keys,1);
	BEGIN
		FOR i IN 1..var_count LOOP
			var_result := replace(var_result, '${' || param_keys[i] || '}', param_values[i]);
		END LOOP;
		return var_result;
	END;
$$
  LANGUAGE 'plpgsql' IMMUTABLE
  COST 100;

-- Helper function that generates script to drop all tables in a particular schema for a particular table
-- This is useful in case you need to reload a state
CREATE OR REPLACE FUNCTION drop_state_tables_generate_script(param_state text, param_schema text DEFAULT 'tiger_data')
  RETURNS text AS
$$
SELECT array_to_string(array_agg('DROP TABLE ' || quote_ident(table_schema) || '.' || quote_ident(table_name) || ';'),E'\n')
	FROM (SELECT * FROM information_schema.tables
	WHERE table_schema = $2 AND table_name like lower($1) || '_%' ORDER BY table_name) AS foo; 
;
$$
  LANGUAGE sql VOLATILE;

DROP TABLE IF EXISTS loader_platform;
CREATE TABLE loader_platform(os varchar(50) PRIMARY KEY, declare_sect text, pgbin text, wget text, unzip_command text, psql text, path_sep text, loader text, environ_set_command text, county_process_command text);
INSERT INTO loader_platform(os, wget, pgbin, declare_sect, unzip_command, psql,path_sep,loader, environ_set_command, county_process_command)
VALUES('windows', '%WGETTOOL%', '%PGBIN%', 
E'set STATEDIR="${staging_fold}\\${website_root}\\${state_fold}"
set TMPDIR=${staging_fold}\\temp\\
set UNZIPTOOL="C:\\Program Files\\7-Zip\\7z.exe"
set WGETTOOL="C:\\wget\\wget.exe"
set PGBIN=C:\\Program Files\\PostgreSQL\\8.4\\bin\\
set PGPORT=5432
set PGHOST=localhost
set PGUSER=postgres
set PGPASSWORD=yourpasswordhere
set PGDATABASE=geocoder
set PSQL="%PGBIN%psql"
set SHP2PGSQL="%PGBIN%shp2pgsql"
cd ${staging_fold}
', E'del %TMPDIR%\\*.* /Q
%PSQL% -c "DROP SCHEMA ${staging_schema} CASCADE;"
%PSQL% -c "CREATE SCHEMA ${staging_schema};"
cd %STATEDIR%
for /r %%z in (*.zip) do %UNZIPTOOL% e %%z  -o%TMPDIR% 
cd %TMPDIR%', E'%PSQL%', E'\\', E'%SHP2PGSQL%', 'set ', 
'for /r %%z in (*${table_name}.dbf) do (${loader}  -s 4269 -g the_geom -W "latin1" %%z tiger_staging.${state_abbrev}_${table_name} | ${psql} & ${psql} -c "SELECT loader_load_staged_data(lower(''${state_abbrev}_${table_name}''), lower(''${state_abbrev}_${lookup_name}''));")'
);


INSERT INTO loader_platform(os, wget, pgbin, declare_sect, unzip_command, psql, path_sep, loader, environ_set_command, county_process_command)
VALUES('sh', 'wget', '', 
E'STATEDIR="${staging_fold}/${website_root}/${state_fold}" 
TMPDIR="${staging_fold}/temp/"
UNZIPTOOL=unzip
WGETTOOL="/usr/bin/wget"
export PGBIN=/usr/pgsql-9.0/bin
export PGPORT=5432
export PGHOST=localhost
export PGUSER=postgres
export PGPASSWORD=yourpasswordhere
export PGDATABASE=geocoder
PSQL=${PGBIN}/psql
SHP2PGSQL=${PGBIN}/shp2pgsql
cd ${staging_fold}
', E'rm -f ${TMPDIR}/*.*
${PSQL} -c "DROP SCHEMA tiger_staging CASCADE;"
${PSQL} -c "CREATE SCHEMA tiger_staging;"
cd $STATEDIR
for z in *.zip; do $UNZIPTOOL -o -d $TMPDIR $z; done
for z in */*.zip; do $UNZIPTOOL -o -d $TMPDIR $z; done
cd $TMPDIR;\n', '${PSQL}', '/', '${SHP2PGSQL}', 'export ',
'for z in *${table_name}.dbf; do 
${loader}  -s 4269 -g the_geom -W "latin1" $z ${staging_schema}.${state_abbrev}_${table_name} | ${psql} 
${PSQL} -c "SELECT loader_load_staged_data(lower(''${state_abbrev}_${table_name}''), lower(''${state_abbrev}_${lookup_name}''));"
done');
-- variables table
DROP TABLE IF EXISTS loader_variables;
CREATE TABLE loader_variables(tiger_year varchar(4) PRIMARY KEY, website_root text, staging_fold text, data_schema text, staging_schema text);
INSERT INTO loader_variables(tiger_year, website_root , staging_fold, data_schema, staging_schema)
	VALUES('2010', 'www2.census.gov/geo/pvs/tiger2010st', '/gisdata', 'tiger_data', 'tiger_staging');

DROP TABLE IF EXISTS loader_lookuptables;
CREATE TABLE loader_lookuptables(process_order integer NOT NULL DEFAULT 1000, 
		lookup_name text primary key, 
		table_name text, single_mode boolean NOT NULL DEFAULT true, 
		load boolean NOT NULL DEFAULT true, 
		level_county boolean NOT NULL DEFAULT false, 
		level_state boolean NOT NULL DEFAULT false,
		post_load_process text, single_geom_mode boolean DEFAULT false, 
		insert_mode char(1) NOT NULL DEFAULT 'c', 
		pre_load_process text,columns_exclude text[]);
		
-- put in explanatory comments of what each column is for
COMMENT ON COLUMN loader_lookuptables.lookup_name IS 'This is the table name to inherit from and suffix of resulting output table -- how the table will be named --  edges where would mean -- ma_edges , pa_edges etc.';
COMMENT ON COLUMN loader_lookuptables.table_name IS 'suffix of the tables to load e.g.  edges would load all tables like *edges.dbf(shp)  -- so tl_2010_42129_edges.dbf .  ';
COMMENT ON COLUMN loader_lookuptables.load IS 'Whether or not to load the table.  For states and zcta5 (you may just want to download states10, zcta510 nationwide file manually) load your own into a single table that inherits from tiger.states, tiger.zcta5.  You''ll get improved performance for some geocoding cases.';
COMMENT ON COLUMN loader_lookuptables.columns_exclude IS 'List of columns to exclude as an array. This is excluded from both input table and output table and rest of columns remaining are assumed to be in same order in both tables. gid, geoid10,cpi,suffix1ce are excluded if no columns are specified.';

INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state,  single_geom_mode, pre_load_process, post_load_process)
VALUES(2, 'county', 'county10', true, false, true,
	false, '${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${lookup_name}(CONSTRAINT pk_${state_abbrev}_${lookup_name} PRIMARY KEY (gid) ) INHERITS(county); " ',
	'${psql} -c "ALTER TABLE ${staging_schema}.${state_abbrev}_${table_name} RENAME geoid10 TO cntyidfp;  SELECT loader_load_staged_data(lower(''${state_abbrev}_${table_name}''), lower(''${state_abbrev}_${lookup_name}'')); ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT uidx_${state_abbrev}_${lookup_name}_cntyidfp UNIQUE (cntyidfp);"
	${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${lookup_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(the_geom);"
	${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_countyfp ON ${data_schema}.${state_abbrev}_${lookup_name} USING btree(countyfp);"
	${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_county_lookup ( CONSTRAINT pk_${state_abbrev}_county_lookup PRIMARY KEY (st_code, co_code)) INHERITS (county_lookup);"
	${psql} -c "VACUUM ANALYZE ${data_schema}.${state_abbrev}_${lookup_name};"
	${psql} -c "INSERT INTO ${data_schema}.${state_abbrev}_${lookup_name}_lookup(st_code, state, co_code, name) SELECT CAST(statefp as integer), ''${state_abbrev}'', CAST(countyfp as integer), name FROM ${data_schema}.${state_abbrev}_${lookup_name};"
	${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
	${psql} -c "VACUUM ANALYZE ${data_schema}.${state_abbrev}_${lookup_name}_lookup;" ');
	
INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process )
VALUES(1, 'state', 'state10', true, false, true,false, 'c', 
	'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_state(CONSTRAINT pk_${state_abbrev}_${lookup_name} PRIMARY KEY (gid) ) INHERITS(state); "',
	'${psql} -c "SELECT loader_load_staged_data(lower(''${state_abbrev}_${table_name}''), lower(''${state_abbrev}_${lookup_name}'')); ALTER TABLE ${data_schema}.${state_abbrev}_state ADD CONSTRAINT uidx_${state_abbrev}_${lookup_name}_stusps UNIQUE (stusps);"
	${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_state_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(the_geom);"
	${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
	${psql} -c "VACUUM ANALYZE ${data_schema}.${state_abbrev}_${lookup_name}"' );

INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process )
VALUES(3, 'place', 'place10', true, false, true,false, 'c', 
	'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${lookup_name}(CONSTRAINT pk_${state_abbrev}_${table_name} PRIMARY KEY (plcidfp) ) INHERITS(place);" ',
	'${psql} -c "ALTER TABLE ${staging_schema}.${state_abbrev}_${table_name} RENAME geoid10 TO plcidfp;SELECT loader_load_staged_data(lower(''${state_abbrev}_${table_name}''), lower(''${state_abbrev}_${lookup_name}'')); ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT uidx_${state_abbrev}_${lookup_name}_gid UNIQUE (gid);"
${psql} -c "CREATE INDEX idx_${state_abbrev}_${lookup_name}_soundex_name ON ${data_schema}.${state_abbrev}_${lookup_name} USING btree (soundex(name));" 
${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${lookup_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(the_geom);"
${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"'  
	);

INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process )
VALUES(4, 'cousub', 'cousub10', true, false, true,false, 'c', 
	'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${lookup_name}(CONSTRAINT pk_${state_abbrev}_${lookup_name} PRIMARY KEY (cosbidfp), CONSTRAINT uidx_${state_abbrev}_${lookup_name}_gid UNIQUE (gid)) INHERITS(${lookup_name});" ',
	'${psql} -c "ALTER TABLE ${staging_schema}.${state_abbrev}_${table_name} RENAME geoid10 TO cosbidfp;SELECT loader_load_staged_data(lower(''${state_abbrev}_${table_name}''), lower(''${state_abbrev}_${lookup_name}'')); ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${lookup_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(the_geom);"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_countyfp ON ${data_schema}.${state_abbrev}_${lookup_name} USING btree(countyfp);"');
	
INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process )
VALUES(4, 'zcta5', 'zcta510', true, false, true,false, 'c', 
	'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${lookup_name}(CONSTRAINT pk_${state_abbrev}_${lookup_name} PRIMARY KEY (zcta5ce,statefp), CONSTRAINT uidx_${state_abbrev}_${lookup_name}_gid UNIQUE (gid)) INHERITS(${lookup_name});" ',
	'${psql} -c "SELECT loader_load_staged_data(lower(''${state_abbrev}_${table_name}''), lower(''${state_abbrev}_${lookup_name}''));ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${lookup_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(the_geom);"');


INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process )
VALUES(6, 'faces', 'faces', true, true, false,false, 'c', 
	'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${table_name}(CONSTRAINT pk_${state_abbrev}_${lookup_name} PRIMARY KEY (gid)) INHERITS(${lookup_name});" ',
	'${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${table_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${lookup_name} USING gist(the_geom);"
	${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_tfid ON ${data_schema}.${state_abbrev}_${lookup_name} USING btree (tfid);"
	${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${table_name}_countyfp ON ${data_schema}.${state_abbrev}_${table_name} USING btree (countyfp);"
	${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
	${psql} -c "vacuum analyze ${data_schema}.${state_abbrev}_${lookup_name};" ');

INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process, columns_exclude )
VALUES(7, 'featnames', 'featnames', true, true, false,false, 'a', 
'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${table_name}(CONSTRAINT pk_${state_abbrev}_${table_name} PRIMARY KEY (gid)) INHERITS(${table_name});" ',
'${psql} -c "UPDATE ${data_schema}.${state_abbrev}_${table_name} SET statefp = ''${state_fips}''  WHERE statefp IS NULL;"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_snd_name ON ${data_schema}.${state_abbrev}_${table_name} USING btree (soundex(name));"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_lname ON ${data_schema}.${state_abbrev}_${table_name} USING btree (lower(name));"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_tlid_statefp ON ${data_schema}.${state_abbrev}_${table_name} USING btree (tlid,statefp);"
${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "vacuum analyze ${data_schema}.${state_abbrev}_${lookup_name};"', ARRAY['gid','statefp']);
	
INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process )
VALUES(8, 'edges', 'edges', true, true, false,false, 'a', 
'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${table_name}(CONSTRAINT pk_${state_abbrev}_${table_name} PRIMARY KEY (gid)) INHERITS(${table_name});" ',
'${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${table_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${table_name}_tlid ON ${data_schema}.${state_abbrev}_${table_name} USING btree (tlid);"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${table_name}_tfidr ON ${data_schema}.${state_abbrev}_${table_name} USING btree (tfidr);"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${table_name}_tfidl ON ${data_schema}.${state_abbrev}_${table_name} USING btree (tfidl);"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${table_name}_countyfp ON ${data_schema}.${state_abbrev}_${table_name} USING btree (countyfp);"
${psql} -c "CREATE INDEX ${data_schema}_${state_abbrev}_${table_name}_the_geom_gist ON ${data_schema}.${state_abbrev}_${table_name} USING gist(the_geom);"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_zipl ON ${data_schema}.${state_abbrev}_${lookup_name} USING btree (zipl);"
${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_zip_state_loc(CONSTRAINT pk_${state_abbrev}_zip_state_loc PRIMARY KEY(zip,stusps,place)) INHERITS(zip_state_loc);"
${psql} -c "INSERT INTO ${data_schema}.${state_abbrev}_zip_state_loc(zip,stusps,statefp,place) SELECT DISTINCT e.zipl, ''${state_abbrev}'', ''${state_fips}'', p.name FROM ${data_schema}.${state_abbrev}_edges AS e INNER JOIN ${data_schema}.${state_abbrev}_faces AS f ON (e.tfidl = f.tfid OR e.tfidr = f.tfid) INNER JOIN ${data_schema}.${state_abbrev}_place As p ON(f.statefp = p.statefp AND f.placefp = p.placefp ) WHERE e.zipl IS NOT NULL;"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_zip_state_loc_place ON ${data_schema}.${state_abbrev}_zip_state_loc USING btree(soundex(place));"
${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_zip_state_loc ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "vacuum analyze ${data_schema}.${state_abbrev}_${lookup_name};"
${psql} -c "vacuum analyze ${data_schema}.${state_abbrev}_zip_state_loc;"
${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_zip_lookup_base(CONSTRAINT pk_${state_abbrev}_zip_state_loc_city PRIMARY KEY(zip,state, county, city, statefp)) INHERITS(zip_lookup_base);"
${psql} -c "INSERT INTO ${data_schema}.${state_abbrev}_zip_lookup_base(zip,state,county,city, statefp) SELECT DISTINCT e.zipl, ''${state_abbrev}'', c.name,p.name,''${state_fips}''  FROM ${data_schema}.${state_abbrev}_edges AS e INNER JOIN ${data_schema}.${state_abbrev}_county As c  ON (e.countyfp = c.countyfp AND e.statefp = c.statefp AND e.statefp = ''${state_fips}'') INNER JOIN ${data_schema}.${state_abbrev}_faces AS f ON (e.tfidl = f.tfid OR e.tfidr = f.tfid) INNER JOIN ${data_schema}.${state_abbrev}_place As p ON(f.statefp = p.statefp AND f.placefp = p.placefp ) WHERE e.zipl IS NOT NULL;"
${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_zip_lookup_base ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_zip_lookup_base_citysnd ON ${data_schema}.${state_abbrev}_zip_lookup_base USING btree(soundex(city));" ');
	
INSERT INTO loader_lookuptables(process_order, lookup_name, table_name, load, level_county, level_state, single_geom_mode, insert_mode, pre_load_process, post_load_process,columns_exclude )
VALUES(9, 'addr', 'addr', true, true, false,false, 'a', 
	'${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_${lookup_name}(CONSTRAINT pk_${state_abbrev}_${table_name} PRIMARY KEY (gid)) INHERITS(${table_name});" ',
	'${psql} -c "UPDATE ${data_schema}.${state_abbrev}_${lookup_name} SET statefp = ''${state_fips}'' WHERE statefp IS NULL;"
	${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_${lookup_name} ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
	${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${lookup_name}_least_address ON tiger_data.${state_abbrev}_addr USING btree (least_hn(fromhn,tohn) );"
	${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${table_name}_tlid_statefp ON ${data_schema}.${state_abbrev}_${table_name} USING btree (tlid, statefp);"
	${psql} -c "CREATE INDEX idx_${data_schema}_${state_abbrev}_${table_name}_zip ON ${data_schema}.${state_abbrev}_${table_name} USING btree (zip);"
	${psql} -c "CREATE TABLE ${data_schema}.${state_abbrev}_zip_state(CONSTRAINT pk_${state_abbrev}_zip_state PRIMARY KEY(zip,stusps)) INHERITS(zip_state); "
	${psql} -c "INSERT INTO ${data_schema}.${state_abbrev}_zip_state(zip,stusps,statefp) SELECT DISTINCT zip, ''${state_abbrev}'', ''${state_fips}'' FROM ${data_schema}.${state_abbrev}_${lookup_name} WHERE zip is not null;"
	${psql} -c "ALTER TABLE ${data_schema}.${state_abbrev}_zip_state ADD CONSTRAINT chk_statefp CHECK (statefp = ''${state_fips}'');"
	${psql} -c "vacuum analyze ${data_schema}.${state_abbrev}_${lookup_name};"',  ARRAY['gid','statefp','fromarmid', 'toarmid']);


CREATE OR REPLACE FUNCTION loader_generate_script(param_states text[], os text)
  RETURNS SETOF text AS
$BODY$
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
				WHERE level_state = true AND load = true
				ORDER BY process_order, lookup_name), E'\n') ::text 
	-- County Level files
	|| E'\n' ||
		array_to_string( ARRAY(SELECT loader_macro_replace(COALESCE(lu.pre_load_process || E'\n', '') || COALESCE(county_process_command || E'\n','')
				|| COALESCE(E'\n' ||lu.post_load_process , '') , ARRAY['loader','table_name','lookup_name'], ARRAY[platform.loader, lu.table_name, lu.lookup_name ]) 
				FROM loader_lookuptables AS lu
				WHERE level_county = true AND load = true
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
$BODY$
  LANGUAGE sql VOLATILE;

CREATE OR REPLACE FUNCTION loader_load_staged_data(param_staging_table text, param_target_table text, param_columns_exclude text[]) RETURNS integer
AS
$$
DECLARE 
	var_sql text;
	var_staging_schema text; var_data_schema text;
	var_temp text;
	var_num_records bigint;
BEGIN
-- Add all the fields except geoid and gid
-- Assume all the columns are in same order as target
	SELECT staging_schema, data_schema INTO var_staging_schema, var_data_schema FROM loader_variables;
	var_sql := 'INSERT INTO ' || var_data_schema || '.' || quote_ident(param_target_table) || '(' ||
			array_to_string(ARRAY(SELECT quote_ident(column_name::text) 
				FROM information_schema.columns 
				 WHERE table_name = param_target_table
					AND table_schema = var_data_schema 
					AND column_name <> ALL(param_columns_exclude) ), ',') || ') SELECT ' 
					|| array_to_string(ARRAY(SELECT quote_ident(column_name::text) 
				FROM information_schema.columns 
				 WHERE table_name = param_staging_table
					AND table_schema = var_staging_schema 
					AND column_name <> ALL( param_columns_exclude) ), ',') ||' FROM ' 
					|| var_staging_schema || '.' || param_staging_table || ';';
	RAISE NOTICE '%', var_sql;
	EXECUTE (var_sql);
	GET DIAGNOSTICS var_num_records = ROW_COUNT;
	SELECT DropGeometryTable(var_staging_schema,param_staging_table) INTO var_temp;
	RETURN var_num_records;
END;
$$
LANGUAGE 'plpgsql' VOLATILE;

CREATE OR REPLACE FUNCTION loader_load_staged_data(param_staging_table text, param_target_table text) 
RETURNS integer AS
$$
   SELECT  loader_load_staged_data($1, $2,(SELECT COALESCE(columns_exclude,ARRAY['gid', 'geoid10','cpi','suffix1ce']) FROM loader_lookuptables WHERE $2 LIKE '%' || lookup_name))
$$
language 'sql' VOLATILE;
COMMIT;