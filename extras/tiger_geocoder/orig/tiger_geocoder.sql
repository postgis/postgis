-- Runs the soundex function on the last word in the string provided.
-- Words are allowed to be seperated by space, comma, period, new-line
-- tab or form feed.
CREATE OR REPLACE FUNCTION end_soundex(VARCHAR) RETURNS VARCHAR
AS '
DECLARE
  tempString VARCHAR;
BEGIN
  tempString := substring($1, ''[ ,\.\n\t\f]([a-zA-Z0-9]*)$'');
  IF tempString IS NOT NULL THEN
    tempString := soundex(tempString);
  ELSE
    tempString := soundex($1);
  END IF;
  return tempString;
END;
' LANGUAGE plpgsql;

-- Returns the value passed, or an empty string if null.
-- This is used to concatinate values that may be null.
CREATE OR REPLACE FUNCTION cull_null(VARCHAR) RETURNS VARCHAR
AS '
BEGIN
  IF $1 IS NULL THEN
    return '''';
  ELSE
    return $1;
  END IF;
END;
' LANGUAGE plpgsql;

-- Determine the number of words in a string.  Words are allowed to
-- be seperated only by spaces, but multiple spaces between
-- words are allowed.
CREATE OR REPLACE FUNCTION count_words(VARCHAR) RETURNS INTEGER
AS '
DECLARE
  tempString VARCHAR;
  tempInt INTEGER;
  count INTEGER := 1;
  lastSpace BOOLEAN := FALSE;
BEGIN
  IF $1 IS NULL THEN
    return -1;
  END IF;
  tempInt := length($1);
  IF tempInt = 0 THEN
    return 0;
  END IF;
  FOR i IN 1..tempInt LOOP
    tempString := substring($1 from i for 1);
    IF tempString = '' '' THEN
      IF NOT lastSpace THEN
        count := count + 1;
      END IF;
      lastSpace := TRUE;
    ELSE
      lastSpace := FALSE;
    END IF;
  END LOOP;
  return count;
END;
' LANGUAGE plpgsql;



CREATE OR REPLACE FUNCTION geocode(VARCHAR) RETURNS REFCURSOR
AS '
BEGIN
  return geocode(NULL, $1);
END;
' LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION geocode(REFCURSOR, VARCHAR) RETURNS REFCURSOR
AS '
DECLARE
  result REFCURSOR;
  input VARCHAR;
  parsed VARCHAR;
  addressString VARCHAR;
  address INTEGER;
  directionPrefix VARCHAR;
  streetName VARCHAR;
  streetType VARCHAR;
  directionSuffix VARCHAR;
  location VARCHAR;
  state VARCHAR;
  zipCodeString VARCHAR;
  zipCode INTEGER;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''geocode()'';
  END IF;
  -- Check inputs.
  IF $1 IS NOT NULL THEN
    result := $1;
  END IF;
  IF $2 IS NULL THEN
    -- The address string is manditory.
    RAISE EXCEPTION ''geocode() - No address string provided.'';
  ELSE
    input := $2;
  END IF;

  -- Pass the input string into the address normalizer
  parsed := normalize_address(input);
  IF parsed IS NULL THEN
    RAISE EXCEPTION ''geocode() - address string failed to parse.'';
  END IF;

  addressString := split_part(parsed, '':'', 1);
  directionPrefix := split_part(parsed, '':'', 2);
  streetName := split_part(parsed, '':'', 3);
  streetType := split_part(parsed, '':'', 4);
  directionSuffix := split_part(parsed, '':'', 5);
  location := split_part(parsed, '':'', 6);
  state := split_part(parsed, '':'', 7);
  zipCodeString := split_part(parsed, '':'', 8);

  -- Empty strings must be converted to nulls;
  IF addressString = '''' THEN
    addressString := NULL;
  END IF;
  IF directionPrefix = '''' THEN
    directionPrefix := NULL;
  END IF;
  IF streetName = '''' THEN
    streetName := NULL;
  END IF;
  IF streetType = '''' THEN
    streetType := NULL;
  END IF;
  IF directionSuffix = '''' THEN
    directionSuffix := NULL;
  END IF;
  IF location = '''' THEN
    location := NULL;
  END IF;
  IF state = '''' THEN 
    state := NULL;
  END IF;
  IF zipCodeString = '''' THEN
    zipCodeString := NULL;
  END IF;

  -- address and zipCode must be integers
  IF addressString IS NOT NULL THEN
    address := to_number(addressString, ''99999999999'');
  END IF;
  IF zipCodeString IS NOT NULL THEN
    zipCode := to_number(zipCodeString, ''99999'');
  END IF;
    
  IF verbose THEN
    RAISE NOTICE ''geocode() - address %'', address;
    RAISE NOTICE ''geocode() - directionPrefix %'', directionPrefix;
    RAISE NOTICE ''geocode() - streetName "%"'', streetName;
    RAISE NOTICE ''geocode() - streetType %'', streetType;
    RAISE NOTICE ''geocode() - directionSuffix %'', directionSuffix;
    RAISE NOTICE ''geocode() - location "%"'', location;
    RAISE NOTICE ''geocode() - state %'', state;
    RAISE NOTICE ''geocode() - zipCode %'', zipCode;
  END IF;
  -- This is where any validation above the geocode_address functions would go.

  -- Call geocode_address
  result := geocode_address(result, address, directionPrefix, streetName,
      streetType, directionSuffix, location, state, zipCode);
  RETURN result;
END;
' LANGUAGE plpgsql;



-- geocode(cursor, address, directionPrefix, streetName, 
-- streetTypeAbbreviation, directionSuffix, location, stateAbbreviation,
-- zipCode)
CREATE OR REPLACE FUNCTION geocode_address(refcursor, INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR, INTEGER) RETURNS REFCURSOR
AS '
DECLARE
  result REFCURSOR;
  address INTEGER;
  directionPrefix VARCHAR;
  streetName VARCHAR;
  streetTypeAbbrev VARCHAR;
  directionSuffix VARCHAR;
  location VARCHAR;
  stateAbbrev VARCHAR;
  state VARCHAR;
  zipCode INTEGER;
  tempString VARCHAR;
  tempInt VARCHAR;
  locationPlaceExact BOOLEAN := FALSE;
  locationPlaceFuzzy BOOLEAN := FALSE;
  locationCountySubExact BOOLEAN := FALSE;
  locationCountySubFuzzy BOOLEAN := FALSE;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''geocode_address()'';
  END IF;
  -- The first step is to determine what weve been given, and if its enough.
  IF $1 IS NOT NULL THEN
    -- The result was not provided.  No matter, we can use an unnamed one.
    result := $1;
  END IF;
  IF $2 IS NULL THEN
    -- The address is manditory.  
    -- Without it, wed be wandering into strangers homes all the time.
    RAISE EXCEPTION ''geocode_address() -  No address provided!'';
  ELSE
    address := $2;
  END IF;
  IF $3 IS NOT NULL THEN
    -- The direction prefix really isnt important.
    -- It will be used for rating if provided. 
    directionPrefix := $3;
  END IF;
  IF $4 IS NULL THEN
    -- A street name must be given.  Think about it.
    RAISE EXCEPTION ''geocode_address() - No street name provided!'';
  ELSE
    streetName := $4;
  END IF;
  IF $5 IS NOT NULL THEN
    -- A street type will be used for rating if provided, but isnt required.
    streetTypeAbbrev := $5;
  END IF;
  IF $6 IS NOT NULL THEN
    -- Same as direction prefix, only later.
    directionSuffix := $6;
  END IF;
  IF $7 IS NOT NULL THEN
    -- Location is not needed iff a zip is given.  The check occurs after 
    -- the geocode_address_zip call.
    location := $7;
  END IF;
  IF $8 IS NULL THEN
    -- State abbreviation is manditory.  It is also assumed to be valid.
  ELSE
    stateAbbrev := $8;
  END IF;
  IF $9 IS NOT NULL THEN
    -- Zip code is optional, but nice.
    zipCode := $9;
  END IF;

  -- The geocoding tables store the state name rather than the abbreviation.
  -- We can validate the abbreviation while retrieving the name.
  IF stateAbbrev IS NOT NULL THEN
    SELECT INTO state name FROM state_lookup 
        WHERE state_lookup.abbrev = stateAbbrev;
    IF state IS NULL THEN
    END IF;
  END IF;

  IF zipCode IS NOT NULL THEN
    IF verbose THEN
      RAISE NOTICE ''geocode_address() - calling geocode_address_zip()'';
    END IF;
    -- If the zip code is given, it is the most useful way to narrow the 
    -- search.  We will try it first, and if no results match, we will move 
    -- on to a location search.  There is no fuzzy searching on zip codes.
    result := geocode_address_zip(result, address, directionPrefix, streetName,
        streetTypeAbbrev, directionSuffix, zipCode);
    IF result IS NOT NULL THEN
      RETURN result;
    ELSE
      result := $1;
    END IF;
  END IF;
  -- After now, the location becomes manditory.
  IF location IS NOT NULL THEN
    -- location may be useful, it may not. The first step is to determine if
    -- there are any potenial matches in the place and countysub fields.
    -- This is done against the lookup tables, and will save us time on much
    -- larger queries if they dont match.
    IF verbose THEN
      RAISE NOTICE ''geocode_address() - calling location_extract_place_*()'';
    END IF;
    tempString := location_extract_place_exact(location, stateAbbrev);
    IF tempString IS NOT NULL THEN
      locationPlaceExact := TRUE;
    ELSE
      locationPlaceExact := FALSE;
    END IF;
    tempString := location_extract_place_fuzzy(location, stateAbbrev);
    IF tempString IS NOT NULL THEN
      locationPlaceFuzzy := true;
    ELSE
      locationPlaceFuzzy := false;
    END IF;
    IF verbose THEN
      RAISE NOTICE ''geocode_address() - calling location_extract_countysub_*()'';
    END IF;
    tempString := location_extract_countysub_exact(location, stateAbbrev);
    IF tempString IS NOT NULL THEN
      locationCountySubExact := TRUE;
    ELSE
      locationCountySubExact := FALSE;
    END IF;
    tempString := location_extract_countysub_fuzzy(location, stateAbbrev);
    IF tempString IS NOT NULL THEN
      locationCountySubFuzzy := true;
    ELSE
      locationCountySubFuzzy := false;
    END IF;
  END IF;
  IF locationPlaceExact THEN
    IF verbose THEN
      RAISE NOTICE ''geocode_address() - calling geocode_address_place_exact()'';
    END IF;
    result := geocode_address_place_exact(result, address, directionPrefix, 
        streetName, streetTypeAbbrev, directionSuffix, location, state);
    IF result IS NOT NULL THEN
      RETURN result;
    ELSE
      result := $1;
    END IF;
  END IF;
  IF locationCountySubExact THEN
    IF verbose THEN
      RAISE NOTICE ''geocode_address() - calling geocode_address_countysub_exact()'';
    END IF;
    result := geocode_address_countysub_exact(result, address, directionPrefix,
        streetName, streetTypeAbbrev, directionSuffix, location, state);
    IF result IS NOT NULL THEN
      RETURN result;
    ELSE
      result := $1;
    END IF;
  END IF;
  IF locationPlaceFuzzy THEN
    IF verbose THEN
      RAISE NOTICE ''geocode_address() - calling geocode_address_place_fuzzy()'';
    END IF;
    result := geocode_address_place_fuzzy(result, address, directionPrefix,
        streetName, streetTypeAbbrev, directionSuffix, location, state);
    IF result IS NOT NULL THEN
      RETURN result;
    ELSE
      result := $1;
    END IF;
  END IF;
  IF locationCountySubFuzzy THEN
    IF verbose THEN
      RAISE NOTICE ''geocode_address() - calling geocode_address_countysub_fuzzy()'';
    END IF;
    result := geocode_address_countysub_fuzzy(result, address, directionPrefix,
        streetName, streetTypeAbbrev, directionSuffix, location, state);
    IF result IS NOT NULL THEN
      RETURN result;
    ELSE
      result := $1;
    END IF;
  END IF;
  IF state IS NOT NULL THEN
    IF verbose THEN
      RAISE NOTICE ''geocode_address() - calling geocode_address_state()'';
    END IF;
    result := geocode_address_state(result, address, directionPrefix,
        streetName, streetTypeAbbrev, directionSuffix, state);
    IF result IS NOT NULL THEN
      RETURN result;
    ELSE
      result := $1;
    END IF;
  END IF;
  RETURN NULL;
END;
' LANGUAGE plpgsql;



CREATE OR REPLACE FUNCTION geocode_address_countysub_exact(REFCURSOR, INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS REFCURSOR
AS '
DECLARE
  result REFCURSOR;
  address INTEGER;
  directionPrefix VARCHAR;
  streetName VARCHAR;
  streetTypeAbbrev VARCHAR;
  directionSuffix VARCHAR;
  state VARCHAR;
  location VARCHAR;
  tempString VARCHAR;
  tempInt VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''geocode_address_countysub_exact()'';
  END IF;
  -- The first step is to determine what weve been given, and if its enough.
  IF $1 IS NOT NULL THEN
    -- The cursor was not provided.  No matter, we can use an unnamed one.
    result := $1;
  END IF;
  IF $2 IS NULL THEN
    -- The address is manditory.  
    -- Without it, wed be wandering into strangers homes all the time.
    RAISE EXCEPTION ''geocode_address_countysub_exact() -  No address provided!'';
  ELSE
    address := $2;
  END IF;
  IF $3 IS NOT NULL THEN
    -- The direction prefix really isnt important.
    -- It will be used for rating if provided. 
    directionPrefix := $3;
  END IF;
  IF $4 IS NULL THEN
    -- A street name must be given.  Think about it.
    RAISE EXCEPTION ''geocode_address_countysub_exact() - No street name provided!'';
  ELSE
    streetName := $4;
  END IF;
  IF $5 IS NOT NULL THEN
    -- A street type will be used for rating if provided, but isnt required.
    streetTypeAbbrev := $5;
  END IF;
  IF $6 IS NOT NULL THEN
    -- Same as direction prefix, only later.
    directionSuffix := $6;
  END IF;
  IF $7 IS NULL THEN
    -- location is manditory.  This is the location geocoder after all.
    RAISE EXCEPTION ''geocode_address_countysub_exact() - No location provided!'';
  ELSE
    location := $7;
  END IF;
  IF $8 IS NOT NULL THEN
    state := $8;
  END IF;

  -- Check to see if the road name can be matched.
  IF state IS NOT NULL THEN
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads 
        WHERE location = tiger_geocode_roads.cousub
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
        AND state = tiger_geocode_roads.state;
  ELSE
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads
        WHERE location = tiger_geocode_roads.cousub
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename);
  END IF;
  IF verbose THEN
    RAISE NOTICE ''geocode_address_countysub_exact() - % potential matches.'', tempInt;
  END IF;
  IF tempInt = 0 THEN
    RETURN NULL;
  ELSE
    -- The road name matches, now we check to see if the addresses match
    IF state IS NOT NULL THEN
      SELECT INTO tempInt count(*) 
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE location = tiger_geocode_roads.cousub
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          AND state = tiger_geocode_roads.state
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid;
    ELSE
      SELECT INTO tempInt count(*) 
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE location = tiger_geocode_roads.cousub
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid;
    END IF;
    IF verbose THEN
      RAISE NOTICE ''geocode_address_countysub_exact() - % address matches.'', tempInt;
    END IF;
    IF tempInt = 0 THEN
      return NULL;
    ELSE
      IF state IS NOT NULL THEN
        OPEN result FOR
        SELECT *, interpolate_from_address(address, roads_local.fraddl, 
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
            roads_local.geom) as address_geom
        FROM (
          SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
            streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
            tiger_geocode_roads.fetype, directionSuffix, 
            tiger_geocode_roads.fedirs, location, 
            tiger_geocode_roads.cousub) as rating 
          FROM tiger_geocode_roads 
          WHERE location = tiger_geocode_roads.cousub
            AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
            AND state = tiger_geocode_roads.state
            ) AS subquery, tiger_geocode_join, roads_local 
        WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
            roads_local.fraddr, roads_local.toaddr)
          AND subquery.id = tiger_geocode_join.id
          AND tiger_geocode_join.tlid = roads_local.tlid
        ORDER BY subquery.rating;
        return result;
      ELSE
        OPEN result FOR
        SELECT *, interpolate_from_address(address, roads_local.fraddl, 
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
            roads_local.geom) as address_geom
        FROM (
          SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
            streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
            tiger_geocode_roads.fetype, directionSuffix, 
            tiger_geocode_roads.fedirs, location, 
            tiger_geocode_roads.cousub) as rating 
          FROM tiger_geocode_roads 
          WHERE location = tiger_geocode_roads.cousub
            AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
            ) AS subquery, tiger_geocode_join, roads_local 
        WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
            roads_local.fraddr, roads_local.toaddr)
            AND subquery.id = tiger_geocode_join.id
            AND tiger_geocode_join.tlid = roads_local.tlid
            ORDER BY subquery.rating;
        RETURN result;
      END IF;
    END IF;
  END IF;
END;
' LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION geocode_address_countysub_fuzzy(REFCURSOR, INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS REFCURSOR
AS '
DECLARE
  result REFCURSOR;
  address INTEGER;
  directionPrefix VARCHAR;
  streetName VARCHAR;
  streetTypeAbbrev VARCHAR;
  directionSuffix VARCHAR;
  state VARCHAR;
  location VARCHAR;
  tempString VARCHAR;
  tempInt VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''geocode_address_countysub_fuzzy()'';
  END IF;
  -- The first step is to determine what weve been given, and if its enough.
  IF $1 IS NOT NULL THEN
    -- The cursor was not provided.  No matter, we can use an unnamed one.
    result := $1;
  END IF;
  IF $2 IS NULL THEN
    -- The address is manditory.  
    -- Without it, wed be wandering into strangers homes all the time.
    RAISE EXCEPTION ''geocode_address_countysub_fuzzy() -  No address provided!'';
  ELSE
    address := $2;
  END IF;
  IF $3 IS NOT NULL THEN
    -- The direction prefix really isnt important.
    -- It will be used for rating if provided. 
    directionPrefix := $3;
  END IF;
  IF $4 IS NULL THEN
    -- A street name must be given.  Think about it.
    RAISE EXCEPTION ''geocode_address_countysub_fuzzy() - No street name provided!'';
  ELSE
    streetName := $4;
  END IF;
  IF $5 IS NOT NULL THEN
    -- A street type will be used for rating if provided, but isnt required.
    streetTypeAbbrev := $5;
  END IF;
  IF $6 IS NOT NULL THEN
    -- Same as direction prefix, only later.
    directionSuffix := $6;
  END IF;
  IF $7 IS NULL THEN
    -- location is manditory.  This is the location geocoder after all.
    RAISE EXCEPTION ''geocode_address_countysub_fuzzy() - No location provided!'';
  ELSE
    location := $7;
  END IF;
  IF $8 IS NOT NULL THEN
    state := $8;
  END IF;

  -- Check to see if the road name can be matched.
  IF state IS NOT NULL THEN
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads 
        WHERE soundex(location) = soundex(tiger_geocode_roads.cousub)
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
        AND state = tiger_geocode_roads.state;
  ELSE
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads
        WHERE soundex(location) = soundex(tiger_geocode_roads.cousub)
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename);
  END IF;
  IF verbose THEN
    RAISE NOTICE ''geocode_address_countysub_fuzzy() - % potential matches.'', tempInt;
  END IF;
  IF tempInt = 0 THEN
    RETURN NULL;
  ELSE
    -- The road name matches, now we check to see if the addresses match
    IF state IS NOT NULL THEN
      SELECT INTO tempInt count(*) 
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE soundex(location) = soundex(tiger_geocode_roads.cousub)
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          AND state = tiger_geocode_roads.state
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid;
    ELSE
      SELECT INTO tempInt count(*) 
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE soundex(location) = soundex(tiger_geocode_roads.cousub)
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid;
    END IF;
    IF verbose THEN
      RAISE NOTICE ''geocode_address_countysub_fuzzy() - % address matches.'', tempInt;
    END IF;
    IF tempInt = 0 THEN
      return NULL;
    ELSE
      IF state IS NOT NULL THEN
        OPEN result FOR
        SELECT *, interpolate_from_address(address, roads_local.fraddl, 
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
            roads_local.geom) as address_geom
        FROM (
          SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
            streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
            tiger_geocode_roads.fetype, directionSuffix, 
            tiger_geocode_roads.fedirs, location, 
            tiger_geocode_roads.cousub) as rating 
          FROM tiger_geocode_roads 
          WHERE soundex(location) = soundex(tiger_geocode_roads.cousub)
            AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
            AND state = tiger_geocode_roads.state
            ) AS subquery, tiger_geocode_join, roads_local 
        WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
            roads_local.fraddr, roads_local.toaddr)
          AND subquery.id = tiger_geocode_join.id
          AND tiger_geocode_join.tlid = roads_local.tlid
        ORDER BY subquery.rating;
        return result;
      ELSE
        OPEN result FOR
        SELECT *, interpolate_from_address(address, roads_local.fraddl, 
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
            roads_local.geom) as address_geom
        FROM (
          SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
            streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
            tiger_geocode_roads.fetype, directionSuffix, 
            tiger_geocode_roads.fedirs, location, 
            tiger_geocode_roads.cousub) as rating 
          FROM tiger_geocode_roads 
          WHERE soundex(location) = soundex(tiger_geocode_roads.cousub)
            AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
            ) AS subquery, tiger_geocode_join, roads_local 
        WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
            roads_local.fraddr, roads_local.toaddr)
            AND subquery.id = tiger_geocode_join.id
            AND tiger_geocode_join.tlid = roads_local.tlid
            ORDER BY subquery.rating;
        RETURN result;
      END IF;
    END IF;
  END IF;
END;
' LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION geocode_address_place_exact(REFCURSOR, INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS REFCURSOR
AS '
DECLARE
  result REFCURSOR;
  address INTEGER;
  directionPrefix VARCHAR;
  streetName VARCHAR;
  streetTypeAbbrev VARCHAR;
  directionSuffix VARCHAR;
  state VARCHAR;
  location VARCHAR;
  tempString VARCHAR;
  tempInt VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''geocode_address_place_exact()'';
  END IF;
  -- The first step is to determine what weve been given, and if its enough.
  IF $1 IS NOT NULL THEN
    -- The cursor was not provided.  No matter, we can use an unnamed one.
    result := $1;
  END IF;
  IF $2 IS NULL THEN
    -- The address is manditory.  
    -- Without it, wed be wandering into strangers homes all the time.
    RAISE EXCEPTION ''geocode_address_place_exact() -  No address provided!'';
  ELSE
    address := $2;
  END IF;
  IF $3 IS NOT NULL THEN
    -- The direction prefix really isnt important.
    -- It will be used for rating if provided. 
    directionPrefix := $3;
  END IF;
  IF $4 IS NULL THEN
    -- A street name must be given.  Think about it.
    RAISE EXCEPTION ''geocode_address_place_exact() - No street name provided!'';
  ELSE
    streetName := $4;
  END IF;
  IF $5 IS NOT NULL THEN
    -- A street type will be used for rating if provided, but isnt required.
    streetTypeAbbrev := $5;
  END IF;
  IF $6 IS NOT NULL THEN
    -- Same as direction prefix, only later.
    directionSuffix := $6;
  END IF;
  IF $7 IS NULL THEN
    -- location is manditory.  This is the location geocoder after all.
    RAISE EXCEPTION ''geocode_address_place_exact() - No location provided!'';
  ELSE
    location := $7;
  END IF;
  IF $8 IS NOT NULL THEN
    state := $8;
  END IF;

  -- Check to see if the road name can be matched.
  IF state IS NOT NULL THEN
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads 
        WHERE location = tiger_geocode_roads.place
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
        AND state = tiger_geocode_roads.state;
  ELSE
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads
        WHERE location = tiger_geocode_roads.place
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename);
  END IF;
  IF verbose THEN
    RAISE NOTICE ''geocode_address_place_exact() - % potential matches.'', tempInt;
  END IF;
  IF tempInt = 0 THEN
    RETURN NULL;
  ELSE
    -- The road name matches, now we check to see if the addresses match
    IF state IS NOT NULL THEN
      SELECT INTO tempInt count(*) 
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE location = tiger_geocode_roads.place
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          AND state = tiger_geocode_roads.state
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid;
    ELSE
      SELECT INTO tempInt count(*) 
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE location = tiger_geocode_roads.place
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid;
    END IF;
    IF verbose THEN
      RAISE NOTICE ''geocode_address_place_exact() - % address matches.'', tempInt;
    END IF;
    IF tempInt = 0 THEN
      return NULL;
    ELSE
      IF state IS NOT NULL THEN
        OPEN result FOR
        SELECT *, interpolate_from_address(address, roads_local.fraddl, 
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
            roads_local.geom) as address_geom
        FROM (
          SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
            streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
            tiger_geocode_roads.fetype, directionSuffix, 
            tiger_geocode_roads.fedirs, location, 
            tiger_geocode_roads.place) as rating 
          FROM tiger_geocode_roads 
          WHERE location = tiger_geocode_roads.place
            AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
            AND state = tiger_geocode_roads.state
            ) AS subquery, tiger_geocode_join, roads_local 
        WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
            roads_local.fraddr, roads_local.toaddr)
          AND subquery.id = tiger_geocode_join.id
          AND tiger_geocode_join.tlid = roads_local.tlid
        ORDER BY subquery.rating;
        return result;
      ELSE
        OPEN result FOR
        SELECT *, interpolate_from_address(address, roads_local.fraddl, 
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
            roads_local.geom) as address_geom
        FROM (
          SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
            streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
            tiger_geocode_roads.fetype, directionSuffix, 
            tiger_geocode_roads.fedirs, location, 
            tiger_geocode_roads.place) as rating 
          FROM tiger_geocode_roads 
          WHERE location = tiger_geocode_roads.place
            AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
            ) AS subquery, tiger_geocode_join, roads_local 
        WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
            roads_local.fraddr, roads_local.toaddr)
            AND subquery.id = tiger_geocode_join.id
            AND tiger_geocode_join.tlid = roads_local.tlid
            ORDER BY subquery.rating;
        RETURN result;
      END IF;
    END IF;
  END IF;
END;
' LANGUAGE plpgsql;



CREATE OR REPLACE FUNCTION geocode_address_place_fuzzy(REFCURSOR, INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS REFCURSOR
AS '
DECLARE
  result REFCURSOR;
  address INTEGER;
  directionPrefix VARCHAR;
  streetName VARCHAR;
  streetTypeAbbrev VARCHAR;
  directionSuffix VARCHAR;
  state VARCHAR;
  location VARCHAR;
  tempString VARCHAR;
  tempInt VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''geocode_address_place_fuzzy()'';
  END IF;
  -- The first step is to determine what weve been given, and if its enough.
  IF $1 IS NOT NULL THEN
    -- The cursor was not provided.  No matter, we can use an unnamed one.
    result := $1;
  END IF;
  IF $2 IS NULL THEN
    -- The address is manditory.  
    -- Without it, wed be wandering into strangers homes all the time.
    RAISE EXCEPTION ''geocode_address_place_fuzzy() -  No address provided!'';
  ELSE
    address := $2;
  END IF;
  IF $3 IS NOT NULL THEN
    -- The direction prefix really isnt important.
    -- It will be used for rating if provided. 
    directionPrefix := $3;
  END IF;
  IF $4 IS NULL THEN
    -- A street name must be given.  Think about it.
    RAISE EXCEPTION ''geocode_address_place_fuzzy() - No street name provided!'';
  ELSE
    streetName := $4;
  END IF;
  IF $5 IS NOT NULL THEN
    -- A street type will be used for rating if provided, but isnt required.
    streetTypeAbbrev := $5;
  END IF;
  IF $6 IS NOT NULL THEN
    -- Same as direction prefix, only later.
    directionSuffix := $6;
  END IF;
  IF $7 IS NULL THEN
    -- location is manditory.  This is the location geocoder after all.
    RAISE EXCEPTION ''geocode_address_place_fuzzy() - No location provided!'';
  ELSE
    location := $7;
  END IF;
  IF $8 IS NOT NULL THEN
    state := $8;
  END IF;

  -- Check to see if the road name can be matched.
  IF state IS NOT NULL THEN
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads 
        WHERE soundex(location) = soundex(tiger_geocode_roads.place)
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
        AND state = tiger_geocode_roads.state;
  ELSE
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads
        WHERE soundex(location) = soundex(tiger_geocode_roads.place)
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename);
  END IF;
  IF verbose THEN
    RAISE NOTICE ''geocode_address_place_fuzzy() - % potential matches.'', tempInt;
  END IF;
  IF tempInt = 0 THEN
    RETURN NULL;
  ELSE
    -- The road name matches, now we check to see if the addresses match
    IF state IS NOT NULL THEN
      SELECT INTO tempInt count(*) 
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE soundex(location) = soundex(tiger_geocode_roads.place)
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          AND state = tiger_geocode_roads.state
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid;
    ELSE
      SELECT INTO tempInt count(*) 
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE soundex(location) = soundex(tiger_geocode_roads.place)
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid;
    END IF;
    IF verbose THEN
      RAISE NOTICE ''geocode_address_place_fuzzy() - % address matches.'', tempInt;
    END IF;
    IF tempInt = 0 THEN
      return NULL;
    ELSE
      IF state IS NOT NULL THEN
        OPEN result FOR
        SELECT *, interpolate_from_address(address, roads_local.fraddl, 
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
            roads_local.geom) as address_geom
        FROM (
          SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
            streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
            tiger_geocode_roads.fetype, directionSuffix, 
            tiger_geocode_roads.fedirs, location, 
            tiger_geocode_roads.place) as rating 
          FROM tiger_geocode_roads 
          WHERE soundex(location) = soundex(tiger_geocode_roads.place)
            AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
            AND state = tiger_geocode_roads.state
            ) AS subquery, tiger_geocode_join, roads_local 
        WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
            roads_local.fraddr, roads_local.toaddr)
          AND subquery.id = tiger_geocode_join.id
          AND tiger_geocode_join.tlid = roads_local.tlid
        ORDER BY subquery.rating;
        return result;
      ELSE
        OPEN result FOR
        SELECT *, interpolate_from_address(address, roads_local.fraddl, 
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
            roads_local.geom) as address_geom
        FROM (
          SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
            streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
            tiger_geocode_roads.fetype, directionSuffix, 
            tiger_geocode_roads.fedirs, location, 
            tiger_geocode_roads.place) as rating 
          FROM tiger_geocode_roads 
          WHERE soundex(location) = soundex(tiger_geocode_roads.place)
            AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
            ) AS subquery, tiger_geocode_join, roads_local 
        WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
            roads_local.fraddr, roads_local.toaddr)
            AND subquery.id = tiger_geocode_join.id
            AND tiger_geocode_join.tlid = roads_local.tlid
            ORDER BY subquery.rating;
        RETURN result;
      END IF;
    END IF;
  END IF;
END;
' LANGUAGE plpgsql;



CREATE OR REPLACE FUNCTION geocode_address_state(REFCURSOR, INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS REFCURSOR
AS '
DECLARE
  result REFCURSOR;
  address INTEGER;
  directionPrefix VARCHAR;
  streetName VARCHAR;
  streetTypeAbbrev VARCHAR;
  directionSuffix VARCHAR;
  state VARCHAR;
  tempString VARCHAR;
  tempInt VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''geocode_address_state()'';
  END IF;
  -- The first step is to determine what weve been given, and if its enough.
  IF $1 IS NOT NULL THEN
    -- The cursor was not provided.  No matter, we can use an unnamed one.
    result := $1;
  END IF;
  IF $2 IS NULL THEN
    -- The address is manditory.  
    -- Without it, wed be wandering into strangers homes all the time.
    RAISE EXCEPTION ''geocode_address_state() -  No address provided!'';
  ELSE
    address := $2;
  END IF;
  IF $3 IS NOT NULL THEN
    -- The direction prefix really isnt important.
    -- It will be used for rating if provided. 
    directionPrefix := $3;
  END IF;
  IF $4 IS NULL THEN
    -- A street name must be given.  Think about it.
    RAISE EXCEPTION ''geocode_address_state() - No street name provided!'';
  ELSE
    streetName := $4;
  END IF;
  IF $5 IS NOT NULL THEN
    -- A street type will be used for rating if provided, but isnt required.
    streetTypeAbbrev := $5;
  END IF;
  IF $6 IS NOT NULL THEN
    -- Same as direction prefix, only later.
    directionSuffix := $6;
  END IF;
  IF $7 IS NOT NULL THEN
    state := $7;
  ELSE
    -- It is unreasonable to do a country wide search.  State is already
    -- pretty sketchy.  No state, no search.
    RAISE EXCEPTION ''geocode_address_state() - No state name provided!'';
  END IF;

  -- Check to see if the road name can be matched.
  SELECT INTO tempInt count(*) FROM tiger_geocode_roads 
      WHERE soundex(streetName) = soundex(tiger_geocode_roads.fename)
      AND state = tiger_geocode_roads.state;
  IF verbose THEN
    RAISE NOTICE ''geocode_address_state() - % potential matches.'', tempInt;
  END IF;
  IF tempInt = 0 THEN
    RETURN NULL;
  ELSE
    -- The road name matches, now we check to see if the addresses match
    SELECT INTO tempInt count(*) 
    FROM (
      SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
        streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
        tiger_geocode_roads.fetype, directionSuffix, 
        tiger_geocode_roads.fedirs) as rating 
      FROM tiger_geocode_roads 
      WHERE soundex(streetName) = soundex(tiger_geocode_roads.fename)
        AND state = tiger_geocode_roads.state
        ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
      AND subquery.id = tiger_geocode_join.id
      AND tiger_geocode_join.tlid = roads_local.tlid;
    IF verbose THEN
      RAISE NOTICE ''geocode_address_state() - % address matches.'', tempInt;
    END IF;
    IF tempInt = 0 THEN
      return NULL;
    ELSE
      OPEN result FOR
      SELECT *, interpolate_from_address(address, roads_local.fraddl, 
          roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
          roads_local.geom) as address_geom
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE soundex(streetName) = soundex(tiger_geocode_roads.fename)
          AND state = tiger_geocode_roads.state
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid
      ORDER BY subquery.rating;
      return result;
    END IF;
  END IF;
END;
' LANGUAGE plpgsql;



CREATE OR REPLACE FUNCTION geocode_address_zip(REFCURSOR, INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, INTEGER) RETURNS REFCURSOR
AS '
DECLARE
  result REFCURSOR;
  address INTEGER;
  directionPrefix VARCHAR;
  streetName VARCHAR;
  streetTypeAbbrev VARCHAR;
  directionSuffix VARCHAR;
  zipCode INTEGER;
  tempString VARCHAR;
  tempInt VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''geocode_address_zip()'';
  END IF;
  -- The first step is to determine what weve been given, and if its enough.
  IF $1 IS NOT NULL THEN
    -- The cursor was not provided.  No matter, we can use an unnamed one.
    result := $1;
  END IF;
  IF $2 IS NULL THEN
    -- The address is manditory.  
    -- Without it, wed be wandering into strangers homes all the time.
    RAISE EXCEPTION ''geocode_address_zip() -  No address provided!'';
  ELSE
    address := $2;
  END IF;
  IF $3 IS NOT NULL THEN
    -- The direction prefix really isnt important.
    -- It will be used for rating if provided. 
    directionPrefix := $3;
  END IF;
  IF $4 IS NULL THEN
    -- A street name must be given.  Think about it.
    RAISE EXCEPTION ''geocode_address_zip() - No street name provided!'';
  ELSE
    streetName := $4;
  END IF;
  IF $5 IS NOT NULL THEN
    -- A street type will be used for rating if provided, but isnt required.
    streetTypeAbbrev := $5;
  END IF;
  IF $6 IS NOT NULL THEN
    -- Same as direction prefix, only later.
    directionSuffix := $6;
  END IF;
  IF $7 IS NULL THEN
    -- Zip code is not optional.
    RAISE EXCEPTION ''geocode_address_zip() - No zip provided!'';
  ELSE
    zipCode := $7;
  END IF;

  -- Check to see if the road name can be matched.
  SELECT INTO tempInt count(*) FROM tiger_geocode_roads 
      WHERE zipCode = tiger_geocode_roads.zip 
      AND soundex(streetName) = soundex(tiger_geocode_roads.fename);
  IF tempInt = 0 THEN
    return NULL;
  ELSE
    -- The road name matches, now we check to see if the addresses match
    SELECT INTO tempInt count(*) 
    FROM (
      SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
        streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
        tiger_geocode_roads.fetype, directionSuffix, 
        tiger_geocode_roads.fedirs) as rating 
      FROM tiger_geocode_roads 
      WHERE zipCode = tiger_geocode_roads.zip 
        AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
        ) AS subquery, tiger_geocode_join, roads_local 
    WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
        roads_local.fraddr, roads_local.toaddr)
      AND subquery.id = tiger_geocode_join.id
      AND tiger_geocode_join.tlid = roads_local.tlid;
    IF tempInt = 0 THEN
      return NULL;
    ELSE
      OPEN result FOR
      SELECT *, interpolate_from_address(address, roads_local.fraddl, 
          roads_local.toaddl, roads_local.fraddr, roads_local.toaddr, 
          roads_local.geom) as address_geom
      FROM (
        SELECT *, rate_attributes(directionPrefix, tiger_geocode_roads.fedirp, 
          streetName, tiger_geocode_roads.fename, streetTypeAbbrev, 
          tiger_geocode_roads.fetype, directionSuffix, 
          tiger_geocode_roads.fedirs) as rating 
        FROM tiger_geocode_roads 
        WHERE zipCode = tiger_geocode_roads.zip 
          AND soundex(streetName) = soundex(tiger_geocode_roads.fename)
          ) AS subquery, tiger_geocode_join, roads_local 
      WHERE includes_address(address, roads_local.fraddl, roads_local.toaddl, 
          roads_local.fraddr, roads_local.toaddr)
        AND subquery.id = tiger_geocode_join.id
        AND tiger_geocode_join.tlid = roads_local.tlid
      ORDER BY subquery.rating;
      return result;
    END IF;
  END IF;
END;
' LANGUAGE plpgsql;



-- Returns a string consisting of the last N words.  Words are allowed 
-- to be seperated only by spaces, but multiple spaces between 
-- words are allowed.  Words must be alphanumberic.
-- If more words are requested than exist, the full input string is 
-- returned.
CREATE OR REPLACE FUNCTION get_last_words(VARCHAR, INTEGER) RETURNS VARCHAR
AS '
DECLARE
  inputString VARCHAR;
  tempString VARCHAR;
  count VARCHAR;
  result VARCHAR := '''';
BEGIN
  IF $1 IS NULL THEN
    return NULL;
  ELSE
    inputString := $1;
  END IF;
  IF $2 IS NULL THEN
    RAISE EXCEPTION ''get_last_words() - word count is null!'';
  ELSE
    count := $2;
  END IF;
  FOR i IN 1..count LOOP
    tempString := substring(inputString from ''((?: )+[a-zA-Z0-9_]*)'' || result || ''$'');
    IF tempString IS NULL THEN
      return inputString;
    END IF;
    result := tempString || result;
  END LOOP;
  result := trim(both from result);
  return result;
END;
' LANGUAGE plpgsql;



-- This function converts the string addresses to integers and passes them
-- to the other includes_address function.
CREATE OR REPLACE FUNCTION includes_address(INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS BOOLEAN
AS '
DECLARE
  given_address INTEGER;
  addr1 INTEGER;
  addr2 INTEGER;
  addr3 INTEGER;
  addr4 INTEGER;
  result BOOLEAN;
BEGIN
  given_address = $1;
  addr1 = to_number($2, ''999999'');
  addr2 = to_number($3, ''999999'');
  addr3 = to_number($4, ''999999'');
  addr4 = to_number($5, ''999999'');
  result = includes_address(given_address, addr1, addr2, addr3, addr4);
  RETURN result;
END
' LANGUAGE plpgsql;



-- This function requires the addresses to be grouped, such that the second and
-- third arguments are from one side of the street, and the fourth and fifth
-- from the other.
CREATE OR REPLACE FUNCTION includes_address(INTEGER, INTEGER, INTEGER, INTEGER, INTEGER) RETURNS BOOLEAN
AS '
DECLARE
  given_address INTEGER;
  addr1 INTEGER;
  addr2 INTEGER;
  addr3 INTEGER;
  addr4 INTEGER;
  lmaxaddr INTEGER := -1;
  rmaxaddr INTEGER := -1;
  lminaddr INTEGER := -1;
  rminaddr INTEGER := -1;
  maxaddr INTEGER := -1;
  minaddr INTEGER := -1;
BEGIN
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''includes_address() - local address is NULL!'';
  ELSE
    given_address := $1;
  END IF;

  IF $2 IS NOT NULL THEN
    addr1 := $2;
    maxaddr := addr1;
    minaddr := addr1;
    lmaxaddr := addr1;
    lminaddr := addr1;
  END IF;

  IF $3 IS NOT NULL THEN
    addr2 := $3;
    IF addr2 < minaddr OR minaddr = -1 THEN
      minaddr := addr2;
    END IF;
    IF addr2 > maxaddr OR maxaddr = -1 THEN
      maxaddr := addr2;
    END IF;
    IF addr2 > lmaxaddr OR lmaxaddr = -1 THEN
      lmaxaddr := addr2;
    END IF;
    IF addr2 < lminaddr OR lminaddr = -1 THEN
      lminaddr := addr2;
    END IF;
  END IF;

  IF $4 IS NOT NULL THEN
    addr3 := $4;
    IF addr3 < minaddr OR minaddr = -1 THEN
      minaddr := addr3;
    END IF;
    IF addr3 > maxaddr OR maxaddr = -1 THEN
      maxaddr := addr3;
    END IF;
    rmaxaddr := addr3;
    rminaddr := addr3;
  END IF;

  IF $5 IS NOT NULL THEN
    addr4 := $5;
    IF addr4 < minaddr OR minaddr = -1 THEN
      minaddr := addr4;
    END IF;
    IF addr4 > maxaddr OR maxaddr = -1 THEN
      maxaddr := addr4;
    END IF;
    IF addr4 > rmaxaddr OR rmaxaddr = -1 THEN
      rmaxaddr := addr4;
    END IF;
    IF addr4 < rminaddr OR rminaddr = -1 THEN
      rminaddr := addr4;
    END IF;
  END IF;

  IF minaddr = -1 OR maxaddr = -1 THEN
    -- No addresses were non-null, return FALSE (arbitrary)
    RETURN FALSE;
  ELSIF given_address >= minaddr AND given_address <= maxaddr THEN
    -- The address is within the given range
    IF given_address >= lminaddr AND given_address <= lmaxaddr THEN
      -- This checks to see if the address is on this side of the
      -- road, ie if the address is even, the street range must be even
      IF (given_address % 2) = (lminaddr % 2)
          OR (given_address % 2) = (lmaxaddr % 2) THEN
        RETURN TRUE;
      END IF;
    END IF;
    IF given_address >= rminaddr AND given_address <= rmaxaddr THEN
      -- See above
      IF (given_address % 2) = (rminaddr % 2) 
          OR (given_address % 2) = (rmaxaddr % 2) THEN
        RETURN TRUE;
      END IF;
    END IF;
  END IF;
  -- The address is not within the range
  RETURN FALSE;
END;

' LANGUAGE plpgsql;



-- This function converts string addresses to integers and passes them to
-- the other interpolate_from_address function.
CREATE OR REPLACE FUNCTION interpolate_from_address(INTEGER, VARCHAR, VARCHAR, VARCHAR, VARCHAR, GEOMETRY) RETURNS GEOMETRY
AS '
DECLARE
  given_address INTEGER;
  addr1 INTEGER;
  addr2 INTEGER;
  addr3 INTEGER;
  addr4 INTEGER;
  road GEOMETRY;
  result GEOMETRY;
BEGIN
  given_address := $1;
  addr1 := to_number($2, ''999999'');
  addr2 := to_number($3, ''999999'');
  addr3 := to_number($4, ''999999'');
  addr4 := to_number($5, ''999999'');
  road := $6;
  result = interpolate_from_address(given_address, addr1, addr2, addr3, addr4, road);
  RETURN result;
END
' LANGUAGE plpgsql;

-- interpolate_from_address(local_address, from_address_l, to_address_l, from_address_r, to_address_r, local_road)
-- This function returns a point along the given geometry (must be linestring)
-- corresponding to the given address.  If the given address is not within
-- the address range of the road, null is returned.
-- This function requires that the address be grouped, such that the second and 
-- third arguments are from one side of the street, while the fourth and 
-- fifth are from the other.
CREATE OR REPLACE FUNCTION interpolate_from_address(INTEGER, INTEGER, INTEGER, INTEGER, INTEGER, GEOMETRY) RETURNS GEOMETRY
AS '
DECLARE
  given_address INTEGER;
  lmaxaddr INTEGER := -1;
  rmaxaddr INTEGER := -1;
  lminaddr INTEGER := -1;
  rminaddr INTEGER := -1;
  lfrgreater BOOLEAN;
  rfrgreater BOOLEAN;
  frgreater BOOLEAN;
  addrwidth INTEGER;
  part DOUBLE PRECISION;
  road GEOMETRY;
  result GEOMETRY;
BEGIN
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''interpolate_from_address() - local address is NULL!'';
  ELSE
    given_address := $1;
  END IF;

  IF $6 IS NULL THEN
    RAISE EXCEPTION ''interpolate_from_address() - local road is NULL!'';
  ELSE
    IF geometrytype($6) = ''LINESTRING'' THEN
      road := $6;
    ELSIF geometrytype($6) = ''MULTILINESTRING'' THEN
      road := geometryn($6,1);
    ELSE
      RAISE EXCEPTION ''interpolate_from_address() - local road is not a line!'';
    END IF;
  END IF;

  IF $2 IS NOT NULL THEN
    lfrgreater := TRUE;
    lmaxaddr := $2;
    lminaddr := $2;
  END IF;

  IF $3 IS NOT NULL THEN
    IF $3 > lmaxaddr OR lmaxaddr = -1 THEN
      lmaxaddr := $3;
      lfrgreater := FALSE;
    END IF;
    IF $3 < lminaddr OR lminaddr = -1 THEN
      lminaddr := $3;
    END IF;
  END IF;

  IF $4 IS NOT NULL THEN
    rmaxaddr := $4;
    rminaddr := $4;
    rfrgreater := TRUE;
  END IF;

  IF $5 IS NOT NULL THEN
    IF $5 > rmaxaddr OR rmaxaddr = -1 THEN
      rmaxaddr := $5;
      rfrgreater := FALSE;
    END IF;
    IF $5 < rminaddr OR rminaddr = -1 THEN
      rminaddr := $5;
    END IF;
  END IF;

  IF given_address >= lminaddr AND given_address <= lmaxaddr THEN
    IF (given_address % 2) = (lminaddr % 2)
        OR (given_address % 2) = (lmaxaddr % 2) THEN
      addrwidth := lmaxaddr - lminaddr;
      part := (given_address - lminaddr) / trunc(addrwidth, 1);
      frgreater := lfrgreater;
    END IF;
  END IF;
  IF given_address >= rminaddr AND given_address <= rmaxaddr THEN
    IF (given_address % 2) = (rminaddr % 2) 
        OR (given_address % 2) = (rmaxaddr % 2) THEN
      addrwidth := rmaxaddr - rminaddr;
      part := (given_address - rminaddr) / trunc(addrwidth, 1);
      frgreater := rfrgreater;
    END IF;
  ELSE
    RETURN null;
  END IF;

  IF frgreater THEN
    part := 1 - part;
  END IF;

  result = line_interpolate_point(road, part);
  RETURN result;
END;
' LANGUAGE plpgsql;


-- This function determines the levenshtein distance irespective of case.
CREATE OR REPLACE FUNCTION levenshtein_ignore_case(VARCHAR, VARCHAR) RETURNS INTEGER
AS '
DECLARE
  result INTEGER;
BEGIN
  result := levenshtein(upper($1), upper($2));
  RETURN result;
END
' LANGUAGE plpgsql;

-- This function take two arguements.  The first is the "given string" and
-- must not be null.  The second arguement is the "compare string" and may 
-- or may not be null.  If the second string is null, the value returned is
-- 3, otherwise it is the levenshtein difference between the two.
CREATE OR REPLACE FUNCTION nullable_levenshtein(VARCHAR, VARCHAR) RETURNS INTEGER
AS '
DECLARE
  given_string VARCHAR;
  result INTEGER := 3;
BEGIN
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''nullable_levenshtein - given string is NULL!'';
  ELSE
    given_string := $1;
  END IF;

  IF $2 IS NOT NULL AND $2 != '''' THEN
    result := levenshtein_ignore_case(given_string, $2);
  END IF;

  RETURN result;
END
' LANGUAGE plpgsql;



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
CREATE OR REPLACE FUNCTION location_extract(VARCHAR, VARCHAR) RETURNS VARCHAR
AS '
DECLARE
  fullStreet VARCHAR;
  stateAbbrev VARCHAR;
  location VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''location_extract()'';
  END IF;
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''location_extract() - No input given!'';
  ELSE
    fullStreet := $1;
  END IF;
  IF $2 IS NULL THEN
  ELSE
    stateAbbrev := $2;
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
  return location;
END;
' LANGUAGE plpgsql;



-- location_extract_countysub_exact(string, stateAbbrev)
-- This function checks the place_lookup table to find a potential match to
-- the location described at the end of the given string.  If an exact match
-- fails, a fuzzy match is performed.  The location as found in the given
-- string is returned.
CREATE OR REPLACE FUNCTION location_extract_countysub_exact(VARCHAR, VARCHAR) RETURNS VARCHAR
AS '
DECLARE
  fullStreet VARCHAR;
  ws VARCHAR;
  tempString VARCHAR;
  location VARCHAR;
  tempInt INTEGER;
  word_count INTEGER;
  stateAbbrev VARCHAR;
  rec RECORD;
  test BOOLEAN;
  result VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''location_extract_countysub_exact()'';
  END IF;
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''location_extract_countysub_exact() - No input given!'';
  ELSE
    fullStreet := $1;
  END IF;
  IF $2 IS NOT NULL THEN
    stateAbbrev := $2;
  END IF;
  ws := ''[ ,\.\n\f\t]'';
  -- No hope of determining the location from place. Try countysub.
  IF stateAbbrev IS NOT NULL THEN
    SELECT INTO tempInt count(*) FROM countysub_lookup
        WHERE countysub_lookup.state = stateAbbrev
        AND texticregexeq(fullStreet, ''(?i)'' || name || ''$'');
  ELSE
    SELECT INTO tempInt count(*) FROM countysub_lookup
        WHERE texticregexeq(fullStreet, ''(?i)'' || name || ''$'');
  END IF;
  IF tempInt > 0 THEN
    IF stateAbbrev IS NOT NULL THEN
      FOR rec IN SELECT substring(fullStreet, ''(?i)(''
          || name || '')$'') AS value, name FROM countysub_lookup
          WHERE countysub_lookup.state = stateAbbrev
          AND texticregexeq(fullStreet, ''(?i)'' || ws || name ||
          ''$'') ORDER BY length(name) DESC LOOP
        -- Only the first result is needed.
        location := rec.value;
        EXIT;
      END LOOP;
    ELSE
      FOR rec IN SELECT substring(fullStreet, ''(?i)(''
          || name || '')$'') AS value, name FROM countysub_lookup
          WHERE texticregexeq(fullStreet, ''(?i)'' || ws || name ||
          ''$'') ORDER BY length(name) DESC LOOP
        -- again, only the first is needed.
        location := rec.value;
        EXIT;
      END LOOP;
    END IF;
  END IF;
  RETURN location;
END;
' LANGUAGE plpgsql;


-- location_extract_countysub_fuzzy(string, stateAbbrev)
-- This function checks the place_lookup table to find a potential match to
-- the location described at the end of the given string.  If an exact match
-- fails, a fuzzy match is performed.  The location as found in the given
-- string is returned.
CREATE OR REPLACE FUNCTION location_extract_countysub_fuzzy(VARCHAR, VARCHAR) RETURNS VARCHAR
AS '
DECLARE
  fullStreet VARCHAR;
  ws VARCHAR;
  tempString VARCHAR;
  location VARCHAR;
  tempInt INTEGER;
  word_count INTEGER;
  stateAbbrev VARCHAR;
  rec RECORD;
  test BOOLEAN;
  result VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''location_extract_countysub_fuzzy()'';
  END IF;
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''location_extract_countysub_fuzzy() - No input given!'';
  ELSE
    fullStreet := $1;
  END IF;
  IF $2 IS NOT NULL THEN
    stateAbbrev := $2;
  END IF;
  ws := ''[ ,\.\n\f\t]'';

  -- Fuzzy matching.
  tempString := substring(fullStreet, ''(?i)'' || ws ||
      ''([a-zA-Z0-9]+)$'');
  IF tempString IS NULL THEN
    tempString := fullStreet;
  END IF;
  IF stateAbbrev IS NOT NULL THEN
    SELECT INTO tempInt count(*) FROM countysub_lookup
        WHERE countysub_lookup.state = stateAbbrev
        AND soundex(tempString) = end_soundex(name);
  ELSE
    SELECT INTO tempInt count(*) FROM countysub_lookup
        WHERE soundex(tempString) = end_soundex(name);
  END IF;
  IF tempInt > 0 THEN
    tempInt := 50;
    -- Some potentials were found.  Begin a word-by-word soundex on each.
    IF stateAbbrev IS NOT NULL THEN
      FOR rec IN SELECT name FROM countysub_lookup
          WHERE countysub_lookup.state = stateAbbrev
          AND soundex(tempString) = end_soundex(name) LOOP
        word_count := count_words(rec.name);
        test := TRUE;
        tempString := get_last_words(fullStreet, word_count);
        FOR i IN 1..word_count LOOP
          IF soundex(split_part(tempString, '' '', i)) !=
            soundex(split_part(rec.name, '' '', i)) THEN
            test := FALSE;
          END IF;
        END LOOP;
        IF test THEN
      -- The soundex matched, determine if the distance is better.
      IF levenshtein_ignore_case(rec.name, tempString) < tempInt THEN
            location := tempString;
        tempInt := levenshtein_ignore_case(rec.name, tempString);
      END IF;
        END IF;
      END LOOP;
    ELSE
      FOR rec IN SELECT name FROM countysub_lookup
          WHERE soundex(tempString) = end_soundex(name) LOOP
        word_count := count_words(rec.name);
        test := TRUE;
        tempString := get_last_words(fullStreet, word_count);
        FOR i IN 1..word_count LOOP
          IF soundex(split_part(tempString, '' '', i)) !=
            soundex(split_part(rec.name, '' '', i)) THEN
            test := FALSE;
          END IF;
        END LOOP;
        IF test THEN
      -- The soundex matched, determine if the distance is better.
      IF levenshtein_ignore_case(rec.name, tempString) < tempInt THEN
            location := tempString;
        tempInt := levenshtein_ignore_case(rec.name, tempString);
      END IF;
        END IF;
      END LOOP;
    END IF;
  END IF; -- If no fuzzys were found, leave location null.
  RETURN location;
END;
' LANGUAGE plpgsql;



-- location_extract_place_exact(string, stateAbbrev)
-- This function checks the place_lookup table to find a potential match to
-- the location described at the end of the given string.  If an exact match
-- fails, a fuzzy match is performed.  The location as found in the given
-- string is returned.
CREATE OR REPLACE FUNCTION location_extract_place_exact(VARCHAR, VARCHAR) RETURNS VARCHAR
AS '
DECLARE
  fullStreet VARCHAR;
  ws VARCHAR;
  tempString VARCHAR;
  location VARCHAR;
  tempInt INTEGER;
  word_count INTEGER;
  stateAbbrev VARCHAR;
  rec RECORD;
  test BOOLEAN;
  result VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''location_extract_place_exact()'';
  END IF;
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''location_extract_place_exact() - No input given!'';
  ELSE
    fullStreet := $1;
  END IF;
  IF verbose THEN
    RAISE NOTICE ''location_extract_place_exact() - input: "%"'', fullStreet;
  END IF;
  IF $2 IS NOT NULL THEN
    stateAbbrev := $2;
  END IF;
  ws := ''[ ,\.\n\f\t]'';
  -- Try for an exact match against places
  IF stateAbbrev IS NOT NULL THEN
    SELECT INTO tempInt count(*) FROM place_lookup 
        WHERE place_lookup.state = stateAbbrev 
        AND texticregexeq(fullStreet, ''(?i)'' || name || ''$'');
  ELSE
    SELECT INTO tempInt count(*) FROM place_lookup
        WHERE texticregexeq(fullStreet, ''(?i)'' || name || ''$'');
  END IF;
  IF verbose THEN
    RAISE NOTICE ''location_extract_place_exact() - Exact Matches %'', tempInt;
  END IF;
  IF tempInt > 0 THEN
    -- Some matches were found.  Look for the last one in the string.
    IF stateAbbrev IS NOT NULL THEN
      FOR rec IN SELECT substring(fullStreet, ''(?i)('' 
          || name || '')$'') AS value, name FROM place_lookup 
          WHERE place_lookup.state = stateAbbrev 
          AND texticregexeq(fullStreet, ''(?i)'' 
          || name || ''$'') ORDER BY length(name) DESC LOOP
        -- Since the regex is end of string, only the longest (first) result
        -- is useful.
        location := rec.value;
        EXIT;
      END LOOP;
    ELSE
      FOR rec IN SELECT substring(fullStreet, ''(?i)('' 
          || name || '')$'') AS value, name FROM place_lookup 
          WHERE texticregexeq(fullStreet, ''(?i)'' 
          || name || ''$'') ORDER BY length(name) DESC LOOP
        -- Since the regex is end of string, only the longest (first) result
        -- is useful.
        location := rec.value;
        EXIT;
      END LOOP;
    END IF;
  END IF;
  RETURN location;
END;
' LANGUAGE plpgsql;



-- location_extract_place_fuzzy(string, stateAbbrev)
-- This function checks the place_lookup table to find a potential match to
-- the location described at the end of the given string.  If an exact match
-- fails, a fuzzy match is performed.  The location as found in the given
-- string is returned.
CREATE OR REPLACE FUNCTION location_extract_place_fuzzy(VARCHAR, VARCHAR) RETURNS VARCHAR
AS '
DECLARE
  fullStreet VARCHAR;
  ws VARCHAR;
  tempString VARCHAR;
  location VARCHAR;
  tempInt INTEGER;
  word_count INTEGER;
  stateAbbrev VARCHAR;
  rec RECORD;
  test BOOLEAN;
  result VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''location_extract_place_fuzzy()'';
  END IF;
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''location_extract_place_fuzzy() - No input given!'';
  ELSE
    fullStreet := $1;
  END IF;
  IF verbose THEN
    RAISE NOTICE ''location_extract_place_fuzzy() - input: "%"'', fullStreet;
  END IF;
  IF $2 IS NOT NULL THEN
    stateAbbrev := $2;
  END IF;
  ws := ''[ ,\.\n\f\t]'';

  tempString := substring(fullStreet, ''(?i)'' || ws 
      || ''([a-zA-Z0-9]+)$'');
  IF tempString IS NULL THEN
      tempString := fullStreet;
  END IF;
  IF stateAbbrev IS NOT NULL THEN
    SELECT into tempInt count(*) FROM place_lookup
        WHERE place_lookup.state = stateAbbrev 
        AND soundex(tempString) = end_soundex(name);
  ELSE 
    SELECT into tempInt count(*) FROM place_lookup
        WHERE soundex(tempString) = end_soundex(name);
  END IF;
  IF verbose THEN
    RAISE NOTICE ''location_extract_place_fuzzy() - Fuzzy matches %'', tempInt;
  END IF;
  IF tempInt > 0 THEN
    -- Some potentials were found.  Begin a word-by-word soundex on each.
    tempInt := 50;
    IF stateAbbrev IS NOT NULL THEN
      FOR rec IN SELECT name FROM place_lookup 
          WHERE place_lookup.state = stateAbbrev
          AND soundex(tempString) = end_soundex(name) LOOP
        IF verbose THEN
          RAISE NOTICE ''location_extract_place_fuzzy() - Fuzzy: "%"'', rec.name;
        END IF;
        word_count := count_words(rec.name);
        test := TRUE;
        tempString := get_last_words(fullStreet, word_count);
        FOR i IN 1..word_count LOOP
          IF soundex(split_part(tempString, '' '', i)) != 
            soundex(split_part(rec.name, '' '', i)) THEN
            IF verbose THEN
              RAISE NOTICE ''location_extract_place_fuzzy() - No Match.'';
            END IF;
            test := FALSE;
          END IF;
        END LOOP;
          IF test THEN
            -- The soundex matched, determine if the distance is better.
            IF levenshtein_ignore_case(rec.name, tempString) < tempInt THEN
              location := tempString;
              tempInt := levenshtein_ignore_case(rec.name, tempString);
            END IF;
          END IF;
      END LOOP;            
    ELSE
      FOR rec IN SELECT name FROM place_lookup 
          WHERE soundex(tempString) = end_soundex(name) LOOP
        word_count := count_words(rec.name);
        test := TRUE;
        tempString := get_last_words(fullStreet, word_count);
        FOR i IN 1..word_count LOOP
          IF soundex(split_part(tempString, '' '', i)) != 
            soundex(split_part(rec.name, '' '', i)) THEN
            test := FALSE;
          END IF;
        END LOOP;
          IF test THEN
            -- The soundex matched, determine if the distance is better.
            IF levenshtein_ignore_case(rec.name, tempString) < tempInt THEN
              location := tempString;
            tempInt := levenshtein_ignore_case(rec.name, tempString);
          END IF;
        END IF;
      END LOOP;   
    END IF;
  END IF;
  RETURN location;
END;
' LANGUAGE plpgsql;


            
-- normalize_address(addressString)
-- This takes an address string and parses it into address (internal/street)
-- street name, type, direction prefix and suffix, location, state and 
-- zip code, depending on what can be found in the string.
--
-- The US postal address standard is used:
-- <Street Number> <Direction Prefix> <Street Name> <Street Type> 
-- <Direction Suffix> <Internal Address> <Location> <State> <Zip Code>
--
-- State is assumed to be included in the string, and MUST be matchable to
-- something in the state_lookup table.  Fuzzy matching is used if no direct
-- match is found.
--
-- Two formats of zip code are acceptable: five digit, and five + 4.
--
-- The internal addressing indicators are looked up from the 
-- secondary_unit_lookup table.  A following identifier is accepted
-- but it must start with a digit.
--
-- The location is parsed from the string using other indicators, such
-- as street type, direction suffix or internal address, if available.
-- If these are not, the location is extracted using comparisons against
-- the places_lookup table, then the countysub_lookup table to determine
-- what, in the original string, is intended to be the location.  In both
-- cases, an exact match is first pursued, then a word-by-word fuzzy match.
-- The result is not the name of the location from the tables, but the 
-- section of the given string that corresponds to the name from the tables.
--
-- Zip codes and street names are not validated.
--
-- Direction indicators are extracted by comparison with the direction_lookup
-- table.
-- 
-- Street addresses are assumed to be a single word, starting with a number.
-- Address is manditory; if no address is given, and the street is numbered,
-- the resulting address will be the street name, and the street name
-- will be an empty string.
--
-- In some cases, the street type is part of the street name.  
-- eg State Hwy 22a.  As long as the word following the type starts with a
-- number (this is usually the case) this will be caught.  Some street names
-- include a type name, and have a street type that differs.  This will be
-- handled properly, so long as both are given.  If the street type is 
-- omitted, the street names included type will be parsed as the street type.
--
-- The output is currently a colon seperated list of values:
-- InternalAddress:StreetAddress:DirectionPrefix:StreetName:StreetType:
-- DirectionSuffix:Location:State:ZipCode
-- This returns each element as entered.  It's mainly meant for debugging.
-- There is also another option that returns:
-- StreetAddress:DirectionPrefixAbbreviation:StreetName:StreetTypeAbbreviation:
-- DirectionSuffixAbbreviation:Location:StateAbbreviation:ZipCode
-- This is more standardized and better for use with a geocoder.
CREATE OR REPLACE FUNCTION normalize_address(VARCHAR) RETURNS VARCHAR
AS '
DECLARE
  rawInput VARCHAR;
  address VARCHAR;
  preDir VARCHAR;
  preDirAbbrev VARCHAR;
  postDir VARCHAR;
  postDirAbbrev VARCHAR;
  fullStreet VARCHAR;
  reducedStreet VARCHAR;
  streetName VARCHAR;
  streetType VARCHAR;
  streetTypeAbbrev VARCHAR;
  internal VARCHAR;
  location VARCHAR;
  state VARCHAR;
  stateAbbrev VARCHAR;
  tempString VARCHAR;
  tempInt INTEGER;
  result VARCHAR;
  zip VARCHAR;
  test BOOLEAN;
  working REFCURSOR;
  rec RECORD;
  ws VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''normalize_address()'';
  END IF;
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''normalise_address() - address string is null!'';
  ELSE
    rawInput := $1;
  END IF;
  ws := ''[ ,\.\t\n\f\r]'';

  -- Assume that the address begins with a digit, and extract it from 
  -- the input string.
  address := substring(rawInput from ''^([0-9].*?)[ ,/.]'');

  -- There are two formats for zip code, the normal 5 digit, and
  -- the nine digit zip-4.  It may also not exist.
  zip := substring(rawInput from ws || ''([0-9]{5})$'');
  IF zip IS NULL THEN
    zip := substring(rawInput from ws || ''([0-9]{5})-[0-9]{4}$'');
  END IF;

  IF zip IS NOT NULL THEN
    fullStreet := substring(rawInput from ''(.*)'' 
        || ws || ''+'' || cull_null(zip) || ''[- ]?([0-9]{4})?$''); 
  ELSE
    fullStreet := rawInput;
  END IF;
  IF verbose THEN
    RAISE NOTICE ''normalize_address() - after zip extract "%"'', fullStreet;
  END IF;
  tempString := state_extract(fullStreet);
  IF tempString IS NOT NULL THEN
    state := split_part(tempString, '':'', 1);
    stateAbbrev := split_part(tempString, '':'', 2);
  END IF;

  -- The easiest case is if the address is comma delimited.  There are some
  -- likely cases:
  --   street level, location, state
  --   street level, location state
  --   street level, location
  --   street level, internal address, location, state
  --   street level, internal address, location state
  --   street level, internal address location state
  --   street level, internal address, location
  --   street level, internal address location
  -- The first three are useful.
  tempString := substring(fullStreet, ''(?i),'' || ws || ''+(.*)(,?'' || ws ||
      ''+'' || cull_null(state) || ''|$)'');
  IF tempString IS NOT NULL THEN
    location := tempString;
    IF address IS NOT NULL THEN
      fullStreet := substring(fullStreet, ''(?i)'' || address || ws || 
          ''+(.*),'' || ws || ''+'' || location);
    ELSE
      fullStreet := substring(fullStreet, ''(?i)(.*),'' || ws || ''+'' ||
          location);
    END IF;
    IF verbose THEN
      RAISE NOTICE ''normalize_address() - Parsed by punctuation.'';
      RAISE NOTICE ''normalize_address() - Location "%"'', location;
      RAISE NOTICE ''normalize_address() - FullStreet "%"'', fullStreet;
    END IF;
  END IF;
  
  -- Pull out the full street information, defined as everything between the
  -- address and the state.  This includes the location.
  -- This doesnt need to be done if location has already been found.
  IF location IS NULL THEN
    IF address IS NOT NULL THEN
      IF state IS NOT NULL THEN
        fullStreet := substring(fullStreet, ''(?i)'' || address || 
            ws || ''+(.*?)'' || ws || ''+'' || state);
      ELSE
        fullStreet := substring(fullStreet, ''(?i)'' || address || 
            ws || ''+(.*?)'');
      END IF;
    ELSE
      IF state IS NOT NULL THEN
        fullStreet := substring(fullStreet, ''(?i)(.*?)'' || ws || 
            ''+'' || state);
      ELSE
        fullStreet := substring(fullStreet, ''(?i)(.*?)'');
      END IF;
    END IF;
  END IF;
  IF verbose THEN
    RAISE NOTICE ''normalize_address() - after addy extract "%"'', fullStreet;
  END IF;

  -- Determine if any internal address is included, such as apartment 
  -- or suite number.  
  SELECT INTO tempInt count(*) FROM secondary_unit_lookup 
      WHERE texticregexeq(fullStreet, ''(?i)'' || ws || name || ''('' 
          || ws || ''|$)'');
  IF tempInt = 1 THEN
    SELECT INTO internal substring(fullStreet, ''(?i)'' || ws || ''('' 
        || name ||  ws || ''*#?'' || ws 
        || ''*(?:[0-9][0-9a-zA-Z\-]*)?'' || '')(?:'' || ws || ''|$)'') 
        FROM secondary_unit_lookup 
        WHERE texticregexeq(fullStreet, ''(?i)'' || ws || name || ''('' 
        || ws || ''|$)'');
    ELSIF tempInt > 1 THEN
    -- In the event of multiple matches to a secondary unit designation, we
    -- will assume that the last one is the true one.
    tempInt := 0;
    FOR rec in SELECT trim(substring(fullStreet, ''(?i)'' || ws || ''('' 
        || name || ''(?:'' || ws || ''*#?'' || ws 
        || ''*(?:[0-9][0-9a-zA-Z\-]*)?)'' || ws || ''?|$)'')) as value 
        FROM secondary_unit_lookup 
        WHERE texticregexeq(fullStreet, ''(?i)'' || ws || name || ''('' 
        || ws || ''|$)'') LOOP
      IF tempInt < position(rec.value in fullStreet) THEN
        tempInt := position(rec.value in fullStreet);
        internal := rec.value;
      END IF;
    END LOOP;
  END IF;

  IF verbose THEN
    RAISE NOTICE ''normalize_address() - internal: "%"'', internal;
  END IF;

  IF location IS NULL THEN
    -- If the internal address is given, the location is everything after it.
    location := substring(fullStreet, internal || ws || ''+(.*)$'');
  END IF;
  
  -- Pull potential street types from the full street information
  SELECT INTO tempInt count(*) FROM street_type_lookup 
      WHERE texticregexeq(fullStreet, ''(?i)'' || ws || ''('' || name 
      || '')(?:'' || ws || ''|$)'');
  IF tempInt = 1 THEN
    SELECT INTO rec abbrev, substring(fullStreet, ''(?i)'' || ws || ''('' 
        || name || '')(?:'' || ws || ''|$)'') AS given FROM street_type_lookup 
        WHERE texticregexeq(fullStreet, ''(?i)'' || ws || ''('' || name 
        || '')(?:'' || ws || ''|$)'');
    streetType := rec.given;
    streetTypeAbbrev := rec.abbrev;
  ELSIF tempInt > 1 THEN
    tempInt := 0;
    FOR rec IN SELECT abbrev, substring(fullStreet, ''(?i)'' || ws || ''('' 
        || name || '')(?:'' || ws || ''|$)'') AS given FROM street_type_lookup 
        WHERE texticregexeq(fullStreet, ''(?i)'' || ws || ''('' || name 
        || '')(?:'' || ws || ''|$)'') LOOP
      -- If we have found an internal address, make sure the type 
      -- precedes it.
      IF internal IS NOT NULL THEN
        IF position(rec.given IN fullStreet) < 
        position(internal IN fullStreet) THEN
          IF tempInt < position(rec.given IN fullStreet) THEN
        streetType := rec.given;
        streetTypeAbbrev := rec.abbrev;
        tempInt := position(rec.given IN fullStreet);
      END IF;
    END IF;
      ELSIF tempInt < position(rec.given IN fullStreet) THEN
        streetType := rec.given;
        streetTypeAbbrev := rec.abbrev;
    tempInt := position(rec.given IN fullStreet);
      END IF;
    END LOOP;
  END IF;
  IF verbose THEN
    RAISE NOTICE ''normalize_address() - street Type: "%"'', streetType;
  END IF;

  -- There is a little more processing required now.  If the word after the
  -- street type begins with a number, the street type should be considered
  -- part of the name, as well as the next word.  eg, State Route 225a.  If
  -- the next word starts with a char, then everything after the street type
  -- will be considered location.  If there is no street type, then Im sad.
  IF streetType IS NOT NULL THEN
    tempString := substring(fullStreet, streetType || ws || 
        ''+([0-9][^ ,\.\t\r\n\f]*?)'' || ws);
    IF tempString IS NOT NULL THEN
      IF location IS NULL THEN
        location := substring(fullStreet, streetType || ws || ''+'' 
        || tempString || ws || ''+(.*)$'');
      END IF;
      reducedStreet := substring(fullStreet, ''(.*)'' || ws || ''+'' 
          || location || ''$'');
      streetType := NULL;
      streetTypeAbbrev := NULL;
    ELSE
      IF location IS NULL THEN
        location := substring(fullStreet, streetType || ws || ''+(.*)$'');
      END IF;
      reducedStreet := substring(fullStreet, ''^(.*)'' || ws || ''+'' 
          || streetType);
    END IF;

    -- The pre direction should be at the beginning of the fullStreet string.
    -- The post direction should be at the beginning of the location string
    -- if there is no internal address
    SELECT INTO tempString substring(reducedStreet, ''(?i)(^'' || name 
        || '')'' || ws) FROM direction_lookup WHERE 
        texticregexeq(reducedStreet, ''(?i)(^'' || name || '')'' || ws)
        ORDER BY length(name) DESC;
    IF tempString IS NOT NULL THEN
      preDir := tempString;
      SELECT INTO preDirAbbrev abbrev FROM direction_lookup 
          where texticregexeq(reducedStreet, ''(?i)(^'' || name || '')'' || ws)
          ORDER BY length(name) DESC;
      streetName := substring(reducedStreet, ''^'' || preDir || ws || ''(.*)'');
    ELSE
      streetName := reducedStreet;
    END IF;

    IF texticregexeq(location, ''(?i)'' || internal || ''$'') THEN
      -- If the internal address is at the end of the location, then no 
      -- location was given.  We still need to look for post direction.
      SELECT INTO rec abbrev,
          substring(location, ''(?i)^('' || name || '')'' || ws) as value
          FROM direction_lookup WHERE texticregexeq(location, ''(?i)^'' 
          || name || ws) ORDER BY length(name) desc;
      IF rec.value IS NOT NULL THEN
        postDir := rec.value;
        postDirAbbrev := rec.abbrev;
      END IF;
      location := null;
    ELSIF internal IS NULL THEN
      -- If no location is given, the location string will be the post direction
      SELECT INTO tempInt count(*) FROM direction_lookup WHERE
          upper(location) = upper(name);
      IF tempInt != 0 THEN
        postDir := location;
        SELECT INTO postDirAbbrev abbrev FROM direction_lookup WHERE
            upper(postDir) = upper(name);
        location := NULL;
      ELSE
        -- postDirection is not equal location, but may be contained in it.
        SELECT INTO tempString substring(location, ''(?i)(^'' || name 
            || '')'' || ws) FROM direction_lookup WHERE 
            texticregexeq(location, ''(?i)(^'' || name || '')'' || ws)
            ORDER BY length(name) desc;
        IF tempString IS NOT NULL THEN
          postDir := tempString;
          SELECT INTO postDirAbbrev abbrev FROM direction_lookup 
          where texticregexeq(location, ''(?i)(^'' || name || '')'' || ws);
          location := substring(location, ''^'' || postDir || ws || ''+(.*)'');
        END IF;
      END IF;
    ELSE
      -- internal is not null, but is not at the end of the location string
      -- look for post direction before the internal address
      SELECT INTO tempString substring(fullStreet, ''(?i)'' || streetType 
          || ws || ''+('' || name || '')'' || ws || ''+'' || internal) 
      FROM direction_lookup WHERE texticregexeq(fullStreet, ''(?i)'' 
      || ws || name || ws || ''+'' || internal) ORDER BY length(name) desc;
      IF tempString IS NOT NULL THEN
        postDir := tempString;
    SELECT INTO postDirAbbrev abbrev FROM direction_lookup 
        WHERE texticregexeq(fullStreet, ''(?i)'' || ws || name || ws);
      END IF;
    END IF;
  ELSE
  -- No street type was found

    -- If an internal address was given, then the split becomes easy, and the
    -- street name is everything before it, without directions.
    IF internal IS NOT NULL THEN
      reducedStreet := substring(fullStreet, ''(?i)^(.*?)'' || ws || ''+'' 
          || internal);
      SELECT INTO tempInt count(*) FROM direction_lookup WHERE 
          texticregexeq(reducedStreet, ''(?i)'' || ws || name || ''$'');
      IF tempInt > 0 THEN
        SELECT INTO postDir substring(reducedStreet, ''(?i)'' || ws || ''('' 
            || name || '')'' || ''$'') FROM direction_lookup 
            WHERE texticregexeq(reducedStreet, ''(?i)'' || ws || name || ''$'');
        SELECT INTO postDirAbbrev abbrev FROM direction_lookup 
            WHERE texticregexeq(reducedStreet, ''(?i)'' || ws || name || ''$'');
      END IF;
        SELECT INTO tempString substring(reducedStreet, ''(?i)^('' || name 
            || '')'' || ws) FROM direction_lookup WHERE 
            texticregexeq(reducedStreet, ''(?i)^('' || name || '')'' || ws)
            ORDER BY length(name) DESC;
      IF tempString IS NOT NULL THEN
        preDir := tempString;
        SELECT INTO preDirAbbrev abbrev FROM direction_lookup WHERE 
            texticregexeq(reducedStreet, ''(?i)(^'' || name || '')'' || ws)
            ORDER BY length(name) DESC;
        streetName := substring(reducedStreet, ''(?i)^'' || preDir || ws 
            || ''+(.*?)(?:'' || ws || ''+'' || cull_null(postDir) || ''|$)'');
      ELSE
        streetName := substring(reducedStreet, ''(?i)^(.*?)(?:'' || ws 
            || ''+'' || cull_null(postDir) || ''|$)'');
      END IF;
    ELSE

      -- If a post direction is given, then the location is everything after,
      -- the street name is everything before, less any pre direction.
      SELECT INTO tempInt count(*) FROM direction_lookup 
          WHERE texticregexeq(fullStreet, ''(?i)'' || ws || name || ''(?:'' 
              || ws || ''|$)'');

      IF tempInt = 1 THEN
        -- A single postDir candidate was found.  This makes it easier.
        SELECT INTO postDir substring(fullStreet, ''(?i)'' || ws || ''('' 
        || name || '')(?:'' || ws || ''|$)'') FROM direction_lookup WHERE 
        texticregexeq(fullStreet, ''(?i)'' || ws || name || ''(?:'' 
            || ws || ''|$)'');
        SELECT INTO postDirAbbrev abbrev FROM direction_lookup 
        WHERE texticregexeq(fullStreet, ''(?i)'' || ws || name 
            || ''(?:'' || ws || ''|$)'');
        IF location IS NULL THEN
          location := substring(fullStreet, ''(?i)'' || ws || postDir 
              || ws || ''+(.*?)$'');
        END IF;
    reducedStreet := substring(fullStreet, ''^(.*?)'' || ws || ''+'' 
        || postDir);
        SELECT INTO tempString substring(reducedStreet, ''(?i)(^'' || name 
        || '')'' || ws) FROM direction_lookup WHERE 
        texticregexeq(reducedStreet, ''(?i)(^'' || name || '')'' || ws)
        ORDER BY length(name) DESC;
        IF tempString IS NOT NULL THEN
          preDir := tempString;
      SELECT INTO preDirAbbrev abbrev FROM direction_lookup WHERE 
          texticregexeq(reducedStreet, ''(?i)(^'' || name || '')'' || ws)
          ORDER BY length(name) DESC;
      streetName := substring(reducedStreet, ''^'' || preDir || ws 
          || ''+(.*)'');
    ELSE
      streetName := reducedStreet;
        END IF;
      ELSIF tempInt > 1 THEN
        -- Multiple postDir candidates were found.  We need to find the last
        -- incident of a direction, but avoid getting the last word from 
        -- a two word direction. eg extracting "East" from "North East"
        -- We do this by sorting by length, and taking the last direction
        -- in the results that is not included in an earlier one.
        -- This wont be a problem it preDir is North East and postDir is
        -- East as the regex requires a space before the direction.  Only 
        -- the East will return from the preDir.
        tempInt := 0;
        FOR rec IN SELECT abbrev, substring(fullStreet, ''(?i)'' || ws || ''('' 
            || name || '')(?:'' || ws || ''|$)'') AS value 
            FROM direction_lookup 
            WHERE texticregexeq(fullStreet, ''(?i)'' || ws || name 
            || ''(?:'' || ws || ''|$)'') 
            ORDER BY length(name) desc LOOP
          tempInt := 0;
          IF tempInt < position(rec.value in fullStreet) THEN
            IF postDir IS NULL THEN
                tempInt := position(rec.value in fullStreet);
              postDir := rec.value;
              postDirAbbrev := rec.abbrev;
            ELSIF NOT texticregexeq(postDir, ''(?i)'' || rec.value) THEN
              tempInt := position(rec.value in fullStreet);
              postDir := rec.value;
              postDirAbbrev := rec.abbrev;
             END IF;
          END IF;
        END LOOP;
	IF location IS NULL THEN
          location := substring(fullStreet, ''(?i)'' || ws || postDir || ws 
          || ''+(.*?)$'');
        END IF;
        reducedStreet := substring(fullStreet, ''(?i)^(.*?)'' || ws || ''+'' 
        || postDir);
        SELECT INTO tempString substring(reducedStreet, ''(?i)(^'' || name 
            || '')'' || ws) FROM direction_lookup WHERE 
            texticregexeq(reducedStreet, ''(?i)(^'' || name || '')'' || ws)
            ORDER BY length(name) DESC;
        IF tempString IS NOT NULL THEN
          preDir := tempString;
	  SELECT INTO preDirAbbrev abbrev FROM direction_lookup WHERE 
              texticregexeq(reducedStreet, ''(?i)(^'' || name || '')'' || ws)
              ORDER BY length(name) DESC;
		  streetName := substring(reducedStreet, ''^'' || preDir || ws 
          || ''+(.*)'');
		ELSE
		  streetName := reducedStreet;
        END IF;        
      ELSE

        -- There is no street type, directional suffix or internal address
        -- to allow distinction between street name and location.
	IF location IS NULL THEN
          location := location_extract(fullStreet, stateAbbrev);
	END IF;
        -- Check for a direction prefix.
        SELECT INTO tempString substring(fullStreet, ''(?i)(^'' || name 
            || '')'' || ws) FROM direction_lookup WHERE 
            texticregexeq(fullStreet, ''(?i)(^'' || name || '')'' || ws)
            ORDER BY length(name);
        RAISE NOTICE ''DEBUG 1'';
        IF tempString IS NOT NULL THEN
          preDir := tempString;
          SELECT INTO preDirAbbrev abbrev FROM direction_lookup WHERE 
              texticregexeq(fullStreet, ''(?i)(^'' || name || '')'' || ws)
              ORDER BY length(name) DESC;
          IF location IS NOT NULL THEN
            -- The location may still be in the fullStreet, or may
            -- have been removed already
            streetName := substring(fullStreet, ''^'' || preDir || ws 
                || ''+(.*?)('' || ws || ''+'' || location || ''|$)'');
            RAISE NOTICE ''DEBUG 2.1 "%", "%"'', streetName, fullStreet;
          ELSE
            streetName := substring(fullStreet, ''^'' || preDir || ws
                || ''+(.*?)'' || ws || ''*'');
          END IF;
        ELSE
          IF location IS NOT NULL THEN
            -- The location may still be in the fullStreet, or may 
            -- have been removed already
            streetName := substring(fullStreet, ''^(.*?)('' || ws 
                || ''+'' || location || ''|$)'');
            RAISE NOTICE ''DEBUG 2.2 "%", "%"'', streetName, fullStreet;
          ELSE
            streetName := fullStreet;
          END IF;
        END IF;
      END IF;
    END IF;
  END IF;



  RAISE NOTICE ''normalize_address() - final internal "%"'', internal;
  RAISE NOTICE ''normalize_address() - prefix_direction "%"'', preDir;
  RAISE NOTICE ''normalize_address() - street_type "%"'', streetType;
  RAISE NOTICE ''normalize_address() - suffix_direction "%"'', postDir;
  RAISE NOTICE ''normalize_address() - state "%"'', state;

  -- This is useful for scripted checking.  It returns what was entered
  -- for each field, rather than what should be used by the geocoder.
  --result := cull_null(internal) || '':'' || cull_null(address) || '':'' 
      --|| cull_null(preDir) || '':'' || cull_null(streetName) || '':'' 
      --|| cull_null(streetType) || '':'' || cull_null(postDir) 
      --|| '':'' || cull_null(location) || '':'' || cull_null(state) || '':'' 
      --|| cull_null(zip);

  -- This is the standardized return.
  result := cull_null(address) || '':'' || cull_null(preDirAbbrev) || '':''
      || cull_null(streetName) || '':'' || cull_null(streetTypeAbbrev) || '':''
      || cull_null(postDirAbbrev) || '':'' || cull_null(location) || '':''
      || cull_null(stateAbbrev) || '':'' || cull_null(zip);
  return result;
END
' LANGUAGE plpgsql;



-- rate_attributes(dirpA, dirpB, streetNameA, streetNameB, streetTypeA, 
-- streetTypeB, dirsA, dirsB, locationA, locationB)
-- Rates the street based on the given attributes.  The locations must be
-- non-null.  The other eight values are handled by the other rate_attributes
-- function, so it's requirements must also be met.
CREATE OR REPLACE FUNCTION rate_attributes(VARCHAR, VARCHAR, VARCHAR, VARCHAR, 
    VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS INTEGER
AS '
DECLARE
  result INTEGER := 0;
  locationWeight INTEGER := 14;
BEGIN
  IF $9 IS NOT NULL AND $10 IS NOT NULL THEN
    result := levenshtein_ignore_case($9, $10);
  ELSE
    RAISE EXCEPTION ''rate_attributes() - Location names cannot be null!'';
  END IF;
  result := result + rate_attributes($1, $2, $3, $4, $5, $6, $7, $8);
  RETURN result;
END;
' LANGUAGE plpgsql;

-- rate_attributes(dirpA, dirpB, streetNameA, streetNameB, streetTypeA, 
-- streetTypeB, dirsA, dirsB)
-- Rates the street based on the given attributes.  Only streetNames are 
-- required.  If any others are null (either A or B) they are treated as
-- empty strings.
CREATE OR REPLACE FUNCTION rate_attributes(VARCHAR, VARCHAR, VARCHAR, VARCHAR, 
    VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS INTEGER
AS '
DECLARE
  result INTEGER := 0;
  directionWeight INTEGER := 2;
  nameWeight INTEGER := 10;
  typeWeight INTEGER := 5;
BEGIN
  result := result + levenshtein_ignore_case(cull_null($1), cull_null($2)) * 
      directionWeight;
  IF $3 IS NOT NULL AND $4 IS NOT NULL THEN
    result := result + levenshtein_ignore_case($3, $4) * nameWeight;
  ELSE
    RAISE EXCEPTION ''rate_attributes() - Street names cannot be null!'';
  END IF;
  result := result + levenshtein_ignore_case(cull_null($5), cull_null($6)) * 
      typeWeight;
  result := result + levenshtein_ignore_case(cull_null($7), cull_null($7)) *
      directionWeight;
  return result;
END;
' LANGUAGE plpgsql;



-- state_extract(addressStringLessZipCode)
-- Extracts the state from end of the given string.
--
-- This function uses the state_lookup table to determine which state
-- the input string is indicating.  First, an exact match is pursued,
-- and in the event of failure, a word-by-word fuzzy match is attempted.
--
-- The result is the state as given in the input string, and the approved
-- state abbreviation, seperated by a colon.
CREATE OR REPLACE FUNCTION state_extract(VARCHAR) RETURNS VARCHAR
AS '
DECLARE
  tempInt INTEGER;
  tempString VARCHAR;
  rawInput VARCHAR;
  state VARCHAR;
  stateAbbrev VARCHAR;
  result VARCHAR;
  rec RECORD;
  test BOOLEAN;
  ws VARCHAR;
  verbose BOOLEAN := TRUE;
BEGIN
  IF verbose THEN
    RAISE NOTICE ''state_extract()'';
  END IF;
  IF $1 IS NULL THEN
    RAISE EXCEPTION ''state_extract() - no input'';
  ELSE
    rawInput := $1;
  END IF;
  ws := ''[ ,\.\t\n\f\r]'';

  -- Separate out the last word of the state, and use it to compare to
  -- the state lookup table to determine the entire name, as well as the
  -- abbreviation associated with it.  The zip code may or may not have
  -- been found.
  tempString := substring(rawInput from ws || ''+([^ ,\.\t\n\f\r0-9]*?)$'');
  SELECT INTO tempInt count(*) FROM (select distinct abbrev from state_lookup 
      WHERE upper(abbrev) = upper(tempString)) as blah;
  IF tempInt = 1 THEN
    state := tempString;
    SELECT INTO stateAbbrev abbrev FROM (select distinct abbrev from 
        state_lookup WHERE upper(abbrev) = upper(tempString)) as blah;
  ELSE
    SELECT INTO tempInt count(*) FROM state_lookup WHERE upper(name) 
        like upper(''%'' || tempString);
    IF tempInt >= 1 THEN
      FOR rec IN SELECT name from state_lookup WHERE upper(name) 
          like upper(''%'' || tempString) LOOP
    SELECT INTO test texticregexeq(rawInput, name) FROM state_lookup 
        WHERE rec.name = name;
    IF test THEN
          SELECT INTO stateAbbrev abbrev FROM state_lookup 
          WHERE rec.name = name;
          state := substring(rawInput, ''(?i)'' || rec.name);
          EXIT;
    END IF;
      END LOOP;
    ELSE
      -- No direct match for state, so perform fuzzy match.
      SELECT INTO tempInt count(*) FROM state_lookup 
          WHERE soundex(tempString) = end_soundex(name);
      IF tempInt >= 1 THEN
        FOR rec IN SELECT name, abbrev FROM state_lookup
        WHERE soundex(tempString) = end_soundex(name) LOOP
          tempInt := count_words(rec.name);
      tempString := get_last_words(rawInput, tempInt);
      test := TRUE;
      FOR i IN 1..tempInt LOOP
            IF soundex(split_part(tempString, '' '', i)) != 
            soundex(split_part(rec.name, '' '', i)) THEN
              test := FALSE;
        END IF;
      END LOOP;
      IF test THEN
            state := tempString;
        stateAbbrev := rec.abbrev;
        EXIT;
      END IF;
    END LOOP;
      END IF;
    END IF;
  END IF;
  IF state IS NOT NULL AND stateAbbrev IS NOT NULL THEN
    result := state || '':'' || stateAbbrev;
  END IF;
  return result;
END;
' LANGUAGE plpgsql;
