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

BEGIN;
-- Type used to pass around a normalized address between functions
-- This is s bit dangerous since it could potentially drop peoples tables
-- TODO: put in logic to check if any tables have norm_addy and don't drop if they do
DROP TYPE IF EXISTS norm_addy CASCADE;
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
    parsed BOOLEAN);

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
CREATE INDEX idx_tiger_addr_least_address ON addr USING btree (least_hn(fromhn,tohn));