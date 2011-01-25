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
CREATE OR REPLACE FUNCTION normalize_address(
    rawInput VARCHAR
) RETURNS norm_addy
AS $_$
DECLARE
  result norm_addy;
  addressString VARCHAR;
  zipString VARCHAR;
  preDir VARCHAR;
  postDir VARCHAR;
  fullStreet VARCHAR;
  reducedStreet VARCHAR;
  streetType VARCHAR;
  state VARCHAR;
  tempString VARCHAR;
  tempInt INTEGER;
  rec RECORD;
  ws VARCHAR;
BEGIN
  result.parsed := FALSE;

  IF rawInput IS NULL THEN
    RETURN result;
  END IF;

  ws := E'[ ,.\t\n\f\r]';

  -- Assume that the address begins with a digit, and extract it from
  -- the input string.
  addressString := substring(rawInput from '^([0-9].*?)[ ,/.]');

  -- There are two formats for zip code, the normal 5 digit, and
  -- the nine digit zip-4.  It may also not exist.
  zipString := substring(rawInput from ws || '([0-9]{5})$');
  IF zipString IS NULL THEN
    zipString := substring(rawInput from ws || '([0-9]{5})-[0-9]{4}$');
    -- Check if all we got was a zipcode, of either form
    IF zipString IS NULL THEN
      zipString := substring(rawInput from '^([0-9]{5})$');
      IF zipString IS NULL THEN
        zipString := substring(rawInput from '^([0-9]{5})-[0-9]{4}$');
      END IF;
      -- If it was only a zipcode, then just return it.
      IF zipString IS NOT NULL THEN
        result.zip := to_number(zipString, '99999');
        result.parsed := TRUE;
        RETURN result;
      END IF;
    END IF;
  END IF;

  IF zipString IS NOT NULL THEN
    fullStreet := substring(rawInput from '(.*)'
        || ws || '+' || cull_null(zipString) || '[- ]?([0-9]{4})?$');
  ELSE
    fullStreet := rawInput;
  END IF;

  -- FIXME: state_extract should probably be returning a record so we can
  -- avoid having to parse the result from it.
  tempString := state_extract(fullStreet);
  IF tempString IS NOT NULL THEN
    state := split_part(tempString, ':', 1);
    result.stateAbbrev := split_part(tempString, ':', 2);
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
  tempString := substring(fullStreet, '(?i),' || ws || '+(.*?)(,?' || ws ||
      '*' || cull_null(state) || '$)');
  IF tempString = '' THEN tempString := NULL; END IF;
  IF tempString IS NOT NULL THEN
    result.location := tempString;
    IF addressString IS NOT NULL THEN
      fullStreet := substring(fullStreet, '(?i)' || addressString || ws ||
          '+(.*),' || ws || '+' || result.location);
    ELSE
      fullStreet := substring(fullStreet, '(?i)(.*),' || ws || '+' ||
          result.location);
    END IF;
  END IF;

  -- Pull out the full street information, defined as everything between the
  -- address and the state.  This includes the location.
  -- This doesnt need to be done if location has already been found.
  IF result.location IS NULL THEN
    IF addressString IS NOT NULL THEN
      IF state IS NOT NULL THEN
        fullStreet := substring(fullStreet, '(?i)' || addressString ||
            ws || '+(.*?)' || ws || '+' || state);
      ELSE
        fullStreet := substring(fullStreet, '(?i)' || addressString ||
            ws || '+(.*?)');
      END IF;
    ELSE
      IF state IS NOT NULL THEN
        fullStreet := substring(fullStreet, '(?i)(.*?)' || ws ||
            '+' || state);
      ELSE
        fullStreet := substring(fullStreet, '(?i)(.*?)');
      END IF;
    END IF;
  END IF;

  -- Determine if any internal address is included, such as apartment
  -- or suite number.
  SELECT INTO tempInt count(*) FROM secondary_unit_lookup
      WHERE texticregexeq(fullStreet, '(?i)' || ws || name || '('
          || ws || '|$)');
  IF tempInt = 1 THEN
    SELECT INTO result.internal substring(fullStreet, '(?i)' || ws || '('
        || name ||  ws || '*#?' || ws
        || '*(?:[0-9][-0-9a-zA-Z]*)?' || ')(?:' || ws || '|$)')
        FROM secondary_unit_lookup
        WHERE texticregexeq(fullStreet, '(?i)' || ws || name || '('
        || ws || '|$)');
    ELSIF tempInt > 1 THEN
    -- In the event of multiple matches to a secondary unit designation, we
    -- will assume that the last one is the true one.
    tempInt := 0;
    FOR rec in SELECT trim(substring(fullStreet, '(?i)' || ws || '('
        || name || '(?:' || ws || '*#?' || ws
        || '*(?:[0-9][-0-9a-zA-Z]*)?)' || ws || '?|$)')) as value
        FROM secondary_unit_lookup
        WHERE texticregexeq(fullStreet, '(?i)' || ws || name || '('
        || ws || '|$)') LOOP
      IF tempInt < position(rec.value in fullStreet) THEN
        tempInt := position(rec.value in fullStreet);
        result.internal := rec.value;
      END IF;
    END LOOP;
  END IF;

  IF result.location IS NULL THEN
    -- If the internal address is given, the location is everything after it.
    result.location := substring(fullStreet, result.internal || ws || '+(.*)$');
  END IF;

  -- Pull potential street types from the full street information
  SELECT INTO tempInt count(*) FROM street_type_lookup
      WHERE texticregexeq(fullStreet, '(?i)' || ws || '(' || name
      || ')(?:' || ws || '|$)');
  IF tempInt = 1 THEN
    SELECT INTO rec abbrev, substring(fullStreet, '(?i)' || ws || '('
        || name || ')(?:' || ws || '|$)') AS given FROM street_type_lookup
        WHERE texticregexeq(fullStreet, '(?i)' || ws || '(' || name
        || ')(?:' || ws || '|$)');
    streetType := rec.given;
    result.streetTypeAbbrev := rec.abbrev;
  ELSIF tempInt > 1 THEN
    tempInt := 0;
    FOR rec IN SELECT abbrev, substring(fullStreet, '(?i)' || ws || '('
        || name || ')(?:' || ws || '|$)') AS given FROM street_type_lookup
        WHERE texticregexeq(fullStreet, '(?i)' || ws || '(' || name
        || ')(?:' || ws || '|$)') LOOP
      -- If we have found an internal address, make sure the type
      -- precedes it.
      IF result.internal IS NOT NULL THEN
        IF position(rec.given IN fullStreet) < position(result.internal IN fullStreet) THEN
          IF tempInt < position(rec.given IN fullStreet) THEN
            streetType := rec.given;
            result.streetTypeAbbrev := rec.abbrev;
            tempInt := position(rec.given IN fullStreet);
          END IF;
        END IF;
      ELSIF tempInt < position(rec.given IN fullStreet) THEN
        streetType := rec.given;
        result.streetTypeAbbrev := rec.abbrev;
        tempInt := position(rec.given IN fullStreet);
      END IF;
    END LOOP;
  END IF;

  -- There is a little more processing required now.  If the word after the
  -- street type begins with a number, the street type should be considered
  -- part of the name, as well as the next word.  eg, State Route 225a.  If
  -- the next word starts with a char, then everything after the street type
  -- will be considered location.  If there is no street type, then I'm sad.
  IF streetType IS NOT NULL THEN
    tempString := substring(fullStreet, streetType || ws ||
        E'+([0-9][^ ,.\t\r\n\f]*?)' || ws);
    IF tempString IS NOT NULL THEN
      IF result.location IS NULL THEN
        result.location := substring(fullStreet, streetType || ws || '+'
                 || tempString || ws || '+(.*)$');
      END IF;
      reducedStreet := substring(fullStreet, '(.*)' || ws || '+'
                    || result.location || '$');
      streetType := NULL;
      result.streetTypeAbbrev := NULL;
    ELSE
      IF result.location IS NULL THEN
        result.location := substring(fullStreet, streetType || ws || '+(.*)$');
      END IF;
      reducedStreet := substring(fullStreet, '^(.*)' || ws || '+'
                    || streetType);
    END IF;

    -- The pre direction should be at the beginning of the fullStreet string.
    -- The post direction should be at the beginning of the location string
    -- if there is no internal address
    SELECT INTO tempString substring(reducedStreet, '(?i)(^' || name
        || ')' || ws) FROM direction_lookup WHERE
        texticregexeq(reducedStreet, '(?i)(^' || name || ')' || ws)
        ORDER BY length(name) DESC;
    IF tempString IS NOT NULL THEN
      preDir := tempString;
      SELECT INTO result.preDirAbbrev abbrev FROM direction_lookup
          where texticregexeq(reducedStreet, '(?i)(^' || name || ')' || ws)
          ORDER BY length(name) DESC;
      result.streetName := substring(reducedStreet, '^' || preDir || ws || '(.*)');
    ELSE
      result.streetName := reducedStreet;
    END IF;

    IF texticregexeq(result.location, '(?i)' || result.internal || '$') THEN
      -- If the internal address is at the end of the location, then no
      -- location was given.  We still need to look for post direction.
      SELECT INTO rec abbrev,
          substring(result.location, '(?i)^(' || name || ')' || ws) as value
          FROM direction_lookup WHERE texticregexeq(result.location, '(?i)^'
          || name || ws) ORDER BY length(name) desc;
      IF rec.value IS NOT NULL THEN
        postDir := rec.value;
        result.postDirAbbrev := rec.abbrev;
      END IF;
      result.location := null;
    ELSIF result.internal IS NULL THEN
      -- If no location is given, the location string will be the post direction
      SELECT INTO tempInt count(*) FROM direction_lookup WHERE
          upper(result.location) = upper(name);
      IF tempInt != 0 THEN
        postDir := result.location;
        SELECT INTO result.postDirAbbrev abbrev FROM direction_lookup WHERE
            upper(postDir) = upper(name);
        result.location := NULL;
      ELSE
        -- postDirection is not equal location, but may be contained in it.
        SELECT INTO tempString substring(result.location, '(?i)(^' || name
            || ')' || ws) FROM direction_lookup WHERE
            texticregexeq(result.location, '(?i)(^' || name || ')' || ws)
            ORDER BY length(name) desc;
        IF tempString IS NOT NULL THEN
          postDir := tempString;
          SELECT INTO result.postDirAbbrev abbrev FROM direction_lookup
              where texticregexeq(result.location, '(?i)(^' || name || ')' || ws);
          result.location := substring(result.location, '^' || postDir || ws || '+(.*)');
        END IF;
      END IF;
    ELSE
      -- internal is not null, but is not at the end of the location string
      -- look for post direction before the internal address
      SELECT INTO tempString substring(fullStreet, '(?i)' || streetType
          || ws || '+(' || name || ')' || ws || '+' || result.internal)
          FROM direction_lookup WHERE texticregexeq(fullStreet, '(?i)'
          || ws || name || ws || '+' || result.internal) ORDER BY length(name) desc;
      IF tempString IS NOT NULL THEN
        postDir := tempString;
        SELECT INTO result.postDirAbbrev abbrev FROM direction_lookup
            WHERE texticregexeq(fullStreet, '(?i)' || ws || name || ws);
      END IF;
    END IF;
  ELSE
  -- No street type was found

    -- If an internal address was given, then the split becomes easy, and the
    -- street name is everything before it, without directions.
    IF result.internal IS NOT NULL THEN
      reducedStreet := substring(fullStreet, '(?i)^(.*?)' || ws || '+'
                    || result.internal);
      SELECT INTO tempInt count(*) FROM direction_lookup WHERE
          texticregexeq(reducedStreet, '(?i)' || ws || name || '$');
      IF tempInt > 0 THEN
        SELECT INTO postDir substring(reducedStreet, '(?i)' || ws || '('
            || name || ')' || '$') FROM direction_lookup
            WHERE texticregexeq(reducedStreet, '(?i)' || ws || name || '$');
        SELECT INTO result.postDirAbbrev abbrev FROM direction_lookup
            WHERE texticregexeq(reducedStreet, '(?i)' || ws || name || '$');
      END IF;
      SELECT INTO tempString substring(reducedStreet, '(?i)^(' || name
          || ')' || ws) FROM direction_lookup WHERE
          texticregexeq(reducedStreet, '(?i)^(' || name || ')' || ws)
          ORDER BY length(name) DESC;
      IF tempString IS NOT NULL THEN
        preDir := tempString;
        SELECT INTO result.preDirAbbrev abbrev FROM direction_lookup WHERE
            texticregexeq(reducedStreet, '(?i)(^' || name || ')' || ws)
            ORDER BY length(name) DESC;
        result.streetName := substring(reducedStreet, '(?i)^' || preDir || ws
                   || '+(.*?)(?:' || ws || '+' || cull_null(postDir) || '|$)');
      ELSE
        result.streetName := substring(reducedStreet, '(?i)^(.*?)(?:' || ws
                   || '+' || cull_null(postDir) || '|$)');
      END IF;
    ELSE

      -- If a post direction is given, then the location is everything after,
      -- the street name is everything before, less any pre direction.
      SELECT INTO tempInt count(*) FROM direction_lookup
          WHERE texticregexeq(fullStreet, '(?i)' || ws || name || '(?:'
              || ws || '|$)');

      IF tempInt = 1 THEN
        -- A single postDir candidate was found.  This makes it easier.
        SELECT INTO postDir substring(fullStreet, '(?i)' || ws || '('
            || name || ')(?:' || ws || '|$)') FROM direction_lookup WHERE
            texticregexeq(fullStreet, '(?i)' || ws || name || '(?:'
            || ws || '|$)');
        SELECT INTO result.postDirAbbrev abbrev FROM direction_lookup
            WHERE texticregexeq(fullStreet, '(?i)' || ws || name
            || '(?:' || ws || '|$)');
        IF result.location IS NULL THEN
          result.location := substring(fullStreet, '(?i)' || ws || postDir
                   || ws || '+(.*?)$');
        END IF;
        reducedStreet := substring(fullStreet, '^(.*?)' || ws || '+'
                      || postDir);
        SELECT INTO tempString substring(reducedStreet, '(?i)(^' || name
            || ')' || ws) FROM direction_lookup WHERE
            texticregexeq(reducedStreet, '(?i)(^' || name || ')' || ws)
            ORDER BY length(name) DESC;
        IF tempString IS NOT NULL THEN
          preDir := tempString;
          SELECT INTO result.preDirAbbrev abbrev FROM direction_lookup WHERE
              texticregexeq(reducedStreet, '(?i)(^' || name || ')' || ws)
              ORDER BY length(name) DESC;
          result.streetName := substring(reducedStreet, '^' || preDir || ws
                     || '+(.*)');
        ELSE
          result.streetName := reducedStreet;
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
        FOR rec IN SELECT abbrev, substring(fullStreet, '(?i)' || ws || '('
            || name || ')(?:' || ws || '|$)') AS value
            FROM direction_lookup
            WHERE texticregexeq(fullStreet, '(?i)' || ws || name
            || '(?:' || ws || '|$)')
            ORDER BY length(name) desc LOOP
          tempInt := 0;
          IF tempInt < position(rec.value in fullStreet) THEN
            IF postDir IS NULL THEN
              tempInt := position(rec.value in fullStreet);
              postDir := rec.value;
              result.postDirAbbrev := rec.abbrev;
            ELSIF NOT texticregexeq(postDir, '(?i)' || rec.value) THEN
              tempInt := position(rec.value in fullStreet);
              postDir := rec.value;
              result.postDirAbbrev := rec.abbrev;
             END IF;
          END IF;
        END LOOP;
        IF result.location IS NULL THEN
          result.location := substring(fullStreet, '(?i)' || ws || postDir || ws
                   || '+(.*?)$');
        END IF;
        reducedStreet := substring(fullStreet, '(?i)^(.*?)' || ws || '+'
                      || postDir);
        SELECT INTO tempString substring(reducedStreet, '(?i)(^' || name
            || ')' || ws) FROM direction_lookup WHERE
            texticregexeq(reducedStreet, '(?i)(^' || name || ')' || ws)
            ORDER BY length(name) DESC;
        IF tempString IS NOT NULL THEN
          preDir := tempString;
          SELECT INTO result.preDirAbbrev abbrev FROM direction_lookup WHERE
              texticregexeq(reducedStreet, '(?i)(^' || name || ')' || ws)
              ORDER BY length(name) DESC;
          result.streetName := substring(reducedStreet, '^' || preDir || ws
                     || '+(.*)');
        ELSE
          result.streetName := reducedStreet;
        END IF;
      ELSE

        -- There is no street type, directional suffix or internal address
        -- to allow distinction between street name and location.
        IF result.location IS NULL THEN
          result.location := location_extract(fullStreet, result.stateAbbrev);
          -- If the location was found, remove it from fullStreet
          fullStreet := substring(fullStreet, '(?i)(.*),' || ws || '+' ||
              result.location);
        END IF;

        -- Check for a direction prefix.
        SELECT INTO tempString substring(fullStreet, '(?i)(^' || name
            || ')' || ws) FROM direction_lookup WHERE
            texticregexeq(fullStreet, '(?i)(^' || name || ')' || ws)
            ORDER BY length(name);
        IF tempString IS NOT NULL THEN
          preDir := tempString;
          SELECT INTO result.preDirAbbrev abbrev FROM direction_lookup WHERE
              texticregexeq(fullStreet, '(?i)(^' || name || ')' || ws)
              ORDER BY length(name) DESC;
          IF result.location IS NOT NULL THEN
            -- The location may still be in the fullStreet, or may
            -- have been removed already
            result.streetName := substring(fullStreet, '^' || preDir || ws
                       || '+(.*?)(' || ws || '+' || result.location || '|$)');
          ELSE
            result.streetName := substring(fullStreet, '^' || preDir || ws
                       || '+(.*?)' || ws || '*');
          END IF;
        ELSE
          IF result.location IS NOT NULL THEN
            -- The location may still be in the fullStreet, or may
            -- have been removed already
            result.streetName := substring(fullStreet, '^(.*?)(' || ws
                       || '+' || result.location || '|$)');
          ELSE
            result.streetName := fullStreet;
          END IF;
        END IF;
      END IF;
    END IF;
  END IF;

  result.address := to_number(addressString, '99999999999');
  result.zip := to_number(zipString, '99999');

  result.parsed := TRUE;
  RETURN result;
END
$_$ LANGUAGE plpgsql;
