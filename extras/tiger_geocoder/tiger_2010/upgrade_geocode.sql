--$Id$
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://www.postgis.org
--
-- Copyright (C) 2010, 2011 Regina Obe and Leo Hsu
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Regina Obe and Leo Hsu <lr@pcorp.us>
--  
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- Tiger is where we're going to create the functions, but we need
-- the PostGIS functions/types which are in public.
SET search_path TO tiger,public;
--this will fail if the column already exists which is fine
ALTER TABLE state_lookup ADD COLUMN statefp char(2);
UPDATE state_lookup SET statefp = lpad(st_code::text,2,'0') WHERE statefp IS NULL;
ALTER TABLE state_lookup ADD CONSTRAINT state_lookup_statefp_key UNIQUE(statefp);
CREATE INDEX idx_tiger_edges_countyfp ON edges USING btree(countyfp);
CREATE INDEX idx_tiger_faces_countyfp ON faces USING btree(countyfp);
CREATE INDEX tiger_place_the_geom_gist ON place USING gist(the_geom);
CREATE INDEX tiger_edges_the_geom_gist ON edges USING gist(the_geom);
CREATE INDEX tiger_state_the_geom_gist ON faces USING gist(the_geom);
DROP FUNCTION IF EXISTS reverse_geocode(geometry); /** changed to use default parameters **/
DROP FUNCTION IF EXISTS geocode_location(norm_addy); /** changed to include default parameter for restrict_geom**/
DROP FUNCTION IF EXISTS geocode(varchar); /** changed to include default parameter for max_results and restrict_geom**/
DROP FUNCTION IF EXISTS geocode(norm_addy); /** changed to include default parameter for max_results and restrict_geom **/
DROP FUNCTION IF EXISTS geocode(varchar, integer); /** changed to include default parameter for max_results and restrict_geom **/
DROP FUNCTION IF EXISTS geocode(norm_addy,integer); /** changed to include default parameter for max_results and restrict_geom **/
DROP FUNCTION IF EXISTS geocode_address(norm_addy); /** changed to include default parameter for max_results **/
DROP FUNCTION IF EXISTS geocode_address(norm_addy,integer); /** changed to include default parameter for max_results and restrict_geom **/

-- TODO: Put in logic to update lookup tables as they change.  street_type_lookup has changed since initial release --
CREATE TABLE zcta5
(
  gid serial NOT NULL,
  statefp character varying(2),
  zcta5ce character varying(5),
  classfp character varying(2),
  mtfcc character varying(5),
  funcstat character varying(1),
  aland double precision,
  awater double precision,
  intptlat character varying(11),
  intptlon character varying(12),
  partflg character varying(1),
  the_geom geometry,
  CONSTRAINT uidx_tiger_zcta5_gid UNIQUE (gid),
  CONSTRAINT enforce_dims_the_geom CHECK (st_ndims(the_geom) = 2),
  CONSTRAINT enforce_geotype_the_geom CHECK (geometrytype(the_geom) = 'MULTIPOLYGON'::text OR the_geom IS NULL),
  CONSTRAINT enforce_srid_the_geom CHECK (st_srid(the_geom) = 4269),
  CONSTRAINT pk_tiger_zcta5_zcta5ce PRIMARY KEY (zcta5ce,statefp)
 );

ALTER TABLE street_type_lookup ALTER COLUMN abbrev  TYPE varchar(50);
ALTER TABLE street_type_lookup ALTER COLUMN name  TYPE varchar(50);
ALTER TABLE street_type_lookup ADD COLUMN is_hw boolean NOT NULL DEFAULT false;

BEGIN;
-- Type used to pass around a normalized address between functions
-- This is s bit dangerous since it could potentially drop peoples tables
-- TODO: put in logic to check if any tables have norm_addy and don't drop if they do
-- Remarking this out for now since we aren't changing norm_addy anyway
/*DROP TYPE IF EXISTS norm_addy CASCADE;
CREATE TYPE norm_addy AS (
    address INTEGER,
    preDirAbbrev VARCHAR,
    streetName VARCHAR,
    streetTypeAbbrev VARCHAR,
    postDirAbbrev VARCHAR,
    internal VARCHAR,
    location VARCHAR,
    stateAbbrev VARCHAR,
    zip VARCHAR,
    parsed BOOLEAN);*/
-- prefix and suffix street names for numbered highways
CREATE TEMPORARY TABLE temp_types AS
SELECT name, abbrev
    FROM (VALUES 
        ('COUNTY HWY', 'Co Hwy'),
        ('COUNTY HIGHWAY', 'Co Hwy'),
        ('COUNTY HIGH WAY', 'Co Hwy'),
        ('COUNTY ROAD', 'Co Rd'),
        ('CO RD', 'Co Rd'),
        ('CORD', 'Co Rd'),
        ('CO RTE', 'Co Rte'),
        ('COUNTY ROUTE', 'Co Rte'),
        ('CO ST AID HWY', 'Co St Aid Hwy'),
        ('FARM RD', 'Farm Rd'),
        ('FIRE RD', 'Fire Rd'),
        ('FOREST RD', 'Forest Rd'),
        ('FOREST ROAD', 'Forest Rd'),
        ('FOREST RTE', 'Forest Rte'),
        ('FOREST ROUTE', 'Forest Rte'),
        ('FREEWAY', 'Fwy'),
        ('FREEWY', 'Fwy'),
        ('FRWAY', 'Fwy'),
        ('FRWY', 'Fwy'),
        ('FWY', 'Fwy'),
        ('HIGHWAY', 'Hwy'),
        ('HIGHWY', 'Hwy'),
        ('HIWAY', 'Hwy'),
        ('HIWY', 'Hwy'),
        ('HWAY', 'Hwy'),
        ('HWY', 'Hwy'),
        ('ROUTE', 'Rte'),
        ('RTE', 'Rte'),
        ('STATE HWY', 'State Hwy'),
        ('STATE HIGHWAY', 'State Hwy'),
        ('STATE HIGH WAY', 'State Hwy'),
        ('STATE RD', 'State Rd'),
        ('STATE ROAD', 'State Rd'),
        ('STATE ROUTE', 'State Rte'),
        ('STATE RTE', 'State Rte'),
        ('TPK', 'Tpke'),
        ('TPKE', 'Tpke'),
        ('TRNPK', 'Tpke'),
        ('TRPK', 'Tpke'),
        ('TURNPIKE', 'Tpke'),
        ('TURNPK', 'Tpke'),
        ('US HWY', 'US Hwy'),
        ('US HIGHWAY', 'US Hwy'),
        ('US HIGH WAY', 'US Hwy'),
        ('US RTE', 'US Rte'),
        ('US ROUTE', 'US Rte'),
        ('US RT', 'US Rte'),
        ('USFS HWY', 'USFS Hwy'),
        ('USFS HIGHWAY', 'USFS Hwy'),
        ('USFS HIGH WAY', 'USFS Hwy'),
        ('USFS RD', 'USFS Rd'),
        ('USFS ROAD', 'USFS Rd')
           ) t(name, abbrev);
           
DELETE FROM street_type_lookup WHERE name IN(SELECT name FROM temp_types);         
INSERT INTO street_type_lookup (name, abbrev, is_hw) 
SELECT name, abbrev, true
    FROM temp_types As t
           WHERE t.name NOT IN(SELECT name FROM street_type_lookup);
DROP TABLE temp_types;           
DELETE FROM street_type_lookup WHERE name = 'FOREST';
-- System/General helper functions
\i utility/utmzone.sql
\i utility/cull_null.sql
\i utility/nullable_levenshtein.sql
\i utility/levenshtein_ignore_case.sql

---- Address normalizer
-- General helpers
\i normalize/end_soundex.sql
\i normalize/count_words.sql
\i normalize/state_extract.sql
\i normalize/get_last_words.sql
-- Location extraction/normalization helpers
\i normalize/location_extract_countysub_exact.sql
\i normalize/location_extract_countysub_fuzzy.sql
\i normalize/location_extract_place_exact.sql
\i normalize/location_extract_place_fuzzy.sql
\i normalize/location_extract.sql
-- Normalization API, called by geocode mainly.
\i normalize/normalize_address.sql
\i normalize/pprint_addy.sql

---- Geocoder functions
-- General helpers
\i geocode/other_helper_functions.sql
\i geocode/rate_attributes.sql
\i geocode/includes_address.sql
\i geocode/interpolate_from_address.sql
-- Actual lookups/geocoder helpers
\i geocode/geocode_address.sql
\i geocode/geocode_location.sql
-- Geocode API, called by user
\i geocode/geocode.sql

-- Reverse Geocode API, called by user
\i geocode/reverse_geocode.sql
COMMIT;