--$Id$
-- Tiger is where we're going to create the functions, but we need
-- the PostGIS functions/types which are in public.
SET search_path TO tiger,public;

-- Type used to pass around a normalized address between functions
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
