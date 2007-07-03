-- location_extract(streetAddressString, stateAbbreviation)
-- This function extracts a location name from the end of the given string.
-- The first attempt is to find an exact match against the place_lookup
-- table.  If this fails, a word-by-word soundex match is tryed against the
-- same table.  If multiple candidates are found, the one with the smallest
-- levenshtein distance from the given string is assumed the correct one.
-- If no match is found against the place_lookup table, the same tests are
-- run against the countysub_lookup table.
--
-- The section of the given string corresponding to the location found is
-- returned, rather than the string found from the tables.  All the searching
-- is done largely to determine the length (words) of the location, to allow
-- the intended street name to be correctly identified.
CREATE OR REPLACE FUNCTION location_extract(fullStreet VARCHAR, stateAbbrev VARCHAR) RETURNS VARCHAR
AS $_$
DECLARE
  location VARCHAR;
BEGIN
  IF fullStreet IS NULL THEN
    RETURN NULL;
  END IF;

  location := location_extract_place_exact(fullStreet, stateAbbrev);
  IF location IS NULL THEN
    location := location_extract_countysub_exact(fullStreet, stateAbbrev);
    IF location IS NULL THEN
      location := location_extract_place_fuzzy(fullStreet, stateAbbrev);
      IF location IS NULL THEN
        location := location_extract_countysub_fuzzy(fullStreet, stateAbbrev);
      END IF;
    END IF;
  END IF;

  RETURN location;
END;
$_$ LANGUAGE plpgsql;
