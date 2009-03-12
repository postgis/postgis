CREATE OR REPLACE FUNCTION geocode_location(
    parsed NORM_ADDY
) RETURNS REFCURSOR
AS $_$
DECLARE
  result REFCURSOR;
  tempString VARCHAR;
  tempInt INTEGER;
BEGIN
  -- Try to match the city/state to a zipcode first
  SELECT INTO tempInt count(*)
    FROM zip_lookup_base zip
    JOIN state_lookup sl ON (zip.state = sl.name)
    JOIN zt99_d00 zl ON (zip.zip = zl.zcta::integer)
    WHERE soundex(zip.city) = soundex(parsed.location) and sl.abbrev = parsed.stateAbbrev;

  -- If that worked, just use the zipcode lookup
  IF tempInt > 0 THEN
    OPEN result FOR
    SELECT
        NULL::varchar(2) as fedirp,
        NULL::varchar(30) as fename,
        NULL::varchar(4) as fetype,
        NULL::varchar(2) as fedirs,
        coalesce(zip.city) as place,
        sl.abbrev as state,
        parsed.zip as zip,
        centroid(wkb_geometry) as address_geom,
        100::integer + levenshtein_ignore_case(coalesce(zip.city), parsed.location) as rating
    FROM
      zip_lookup_base zip
      JOIN state_lookup sl on (zip.state = sl.name)
      JOIN zt99_d00 zl ON (zip.zip = zl.zcta::integer)
    WHERE
      soundex(zip.city) = soundex(parsed.location) and sl.abbrev = parsed.stateAbbrev;

    RETURN result;
  END IF;

  -- Try to match the city/state to a place next
  SELECT INTO tempInt count(*)
    FROM pl99_d00 pl
    JOIN state_lookup sl ON (pl.state::integer = sl.st_code)
    WHERE soundex(pl.name) = soundex(parsed.location) and sl.abbrev = parsed.stateAbbrev;

  -- If that worked then use it
  IF tempInt > 0 THEN
    OPEN result FOR
    SELECT
        NULL::varchar(2) as fedirp,
        NULL::varchar(30) as fename,
        NULL::varchar(4) as fetype,
        NULL::varchar(2) as fedirs,
        pl.name as place,
        sl.abbrev as state,
        NULL::integer as zip,
        centroid(wkb_geometry) as address_geom,
        100::integer + levenshtein_ignore_case(coalesce(zip.city), parsed.location) as rating
    FROM pl99_d00 pl
    JOIN state_lookup sl ON (pl.state::integer = sl.st_code)
    WHERE soundex(pl.name) = soundex(parsed.location) and sl.abbrev = parsed.stateAbbrev;

    RETURN result;
  END IF;
  RETURN result;
END;
$_$ LANGUAGE plpgsql;
