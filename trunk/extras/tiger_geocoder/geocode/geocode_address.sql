-- geocode(cursor, address, directionPrefix, streetName,
-- streetTypeAbbreviation, directionSuffix, location, stateAbbreviation,
-- zipCode)
CREATE OR REPLACE FUNCTION geocode_address(
    parsed NORM_ADDY
) RETURNS REFCURSOR
AS $_$
DECLARE
  result REFCURSOR;
  tempString VARCHAR;
  ziplookup RECORD;
BEGIN
  -- The first step is to determine what weve been given, and if its enough.
  IF parsed.address IS NULL THEN
    -- The address is manditory.
    -- Without it, wed be wandering into strangers homes all the time.
    RETURN NULL;
  END IF;

  IF parsed.streetName IS NULL THEN
    -- A street name must be given.  Think about it.
    RETURN NULL;
  END IF;

  IF parsed.zip IS NOT NULL THEN
    -- If the zip code is given, it is the most useful way to narrow the
    -- search.  We will try it first, and if no results match, we will move
    -- on to a location search.  There is no fuzzy searching on zip codes.
    result := geocode_address_zip(result, parsed);
    IF result IS NOT NULL THEN
      RETURN result;
    END IF;
    -- If we weren't able to find one using the zip code, but the zip code
    -- exists, and location is null, then fill in the location and/or state
    -- based on the zip code so that the location lookup has a chance.
    IF parsed.stateAbbrev IS NULL OR parsed.location IS NULL THEN
        SELECT INTO ziplookup * FROM zip_lookup_base JOIN state_lookup ON (state = name) WHERE zip = parsed.zip;
        IF FOUND THEN
            parsed.stateAbbrev := coalesce(parsed.stateAbbrev,ziplookup.abbrev);
            parsed.location := coalesce(parsed.location,ziplookup.city);
        END IF;
    END IF;
  END IF;

  -- After now, the location becomes manditory.
  IF parsed.location IS NOT NULL THEN
    -- location may be useful, it may not. The first step is to determine if
    -- there are any potenial matches in the place and countysub fields.
    -- This is done against the lookup tables, and will save us time on much
    -- larger queries if they dont match.
    tempString := location_extract_place_exact(parsed.location, parsed.stateAbbrev);
    IF tempString IS NOT NULL THEN
      result := geocode_address_place_exact(result, parsed);
      IF result IS NOT NULL THEN
        RETURN result;
      END IF;
    END IF;

    tempString := location_extract_countysub_exact(parsed.location, parsed.stateAbbrev);
    IF tempString IS NOT NULL THEN
      result := geocode_address_countysub_exact(result, parsed);
      IF result IS NOT NULL THEN
        RETURN result;
      END IF;
    END IF;

    tempString := location_extract_place_fuzzy(parsed.location, parsed.stateAbbrev);
    IF tempString IS NOT NULL THEN
      result := geocode_address_place_fuzzy(result, parsed);
      IF result IS NOT NULL THEN
        RETURN result;
      END IF;
    END IF;

    tempString := location_extract_countysub_fuzzy(parsed.location, parsed.stateAbbrev);
    IF tempString IS NOT NULL THEN
      result := geocode_address_countysub_fuzzy(result, parsed);
      IF result IS NOT NULL THEN
        RETURN result;
      END IF;
    END IF;
  END IF;

  -- Try with just the state if we can't find the location
  IF parsed.stateAbbrev IS NOT NULL THEN
    result := geocode_address_state(result, parsed);
    IF result IS NOT NULL THEN
      RETURN result;
    END IF;
  END IF;

  RETURN NULL;
END;
$_$ LANGUAGE plpgsql;
