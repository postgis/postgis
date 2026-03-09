-- location_extract_place_exact(string, stateAbbrev)
-- This function checks the place_lookup table to find a potential match to
-- the location described at the end of the given string.  If an exact match
-- fails, a fuzzy match is performed.  The location as found in the given
-- string is returned.
CREATE OR REPLACE FUNCTION location_extract_place_exact(
    fullStreet VARCHAR,
    stateAbbrev VARCHAR
) RETURNS VARCHAR
AS $$
  SELECT (
      SELECT substring(fullStreet, '(?i)(' || name || ')$')
      FROM place
      WHERE (stateAbbrev IS NULL OR place.statefp = (SELECT statefp FROM state WHERE stusps = stateAbbrev))
        AND fullStreet ILIKE '%' || name || '%'
        AND texticregexeq(fullStreet, '(?i)' || name || '$')
      ORDER BY length(name) DESC
      LIMIT 1
  );
$$ LANGUAGE sql STABLE COST 100;
