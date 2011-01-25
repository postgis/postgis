CREATE OR REPLACE FUNCTION geocode_location(
    parsed NORM_ADDY,
    OUT ADDY NORM_ADDY,
    OUT GEOMOUT GEOMETRY,
    OUT RATING INTEGER
) RETURNS SETOF RECORD
AS $_$
DECLARE
  result RECORD;
  in_statefp VARCHAR;
  stmt VARCHAR;
BEGIN

  in_statefp := statefp FROM state WHERE state.stusps = parsed.stateAbbrev;

  FOR result IN
    SELECT
        coalesce(zip.city)::varchar as place,
        zip.zip as zip,
        centroid(zcta500.the_geom) as address_geom,
        stusps as state,
        100::integer + coalesce(levenshtein_ignore_case(coalesce(zip.city), parsed.location),0) as in_rating
    FROM
      zip_lookup_base zip
      JOIN zcta500 ON (zip.zip = zcta500.zcta5ce)
      JOIN state USING (statefp)
    WHERE
      parsed.zip = zip.zip OR
      (soundex(zip.city) = soundex(parsed.location) and zip.statefp = in_statefp)
    ORDER BY levenshtein_ignore_case(coalesce(zip.city), parsed.location), zip.zip
  LOOP
    ADDY.location := result.place;
    ADDY.stateAbbrev := result.state;
    ADDY.zip := result.zip;
    ADDY.parsed := true;
    GEOMOUT := result.address_geom;
    RATING := result.in_rating;

    RETURN NEXT;

    IF RATING = 100 THEN
      RETURN;
    END IF;

  END LOOP;

  IF parsed.location IS NULL THEN
    parsed.location := city FROM zip_lookup_base WHERE zip_lookup_base.zip = parsed.zip ORDER BY zip_lookup_base.zip LIMIT 1;
    in_statefp := statefp FROM zip_lookup_base WHERE zip_lookup_base.zip = parsed.zip ORDER BY zip_lookup_base.zip LIMIT 1;
  END IF;

  stmt := 'SELECT '
       || ' pl.name as place, '
       || ' state.stusps as stateAbbrev, '
       || ' centroid(pl.the_geom) as address_geom, '
       || ' 100::integer + levenshtein_ignore_case(coalesce(pl.name), ' || quote_literal(coalesce(parsed.location,'')) || ') as in_rating '
       || ' FROM place pl '
       || ' JOIN state USING (statefp)'
       || ' WHERE soundex(pl.name) = soundex(' || quote_literal(coalesce(parsed.location,'')) || ') and pl.statefp = ' || quote_literal(coalesce(in_statefp,''))
       || ' ORDER BY levenshtein_ignore_case(coalesce(pl.name), ' || quote_literal(coalesce(parsed.location,'')) || ');'
       ;

  FOR result IN EXECUTE stmt
  LOOP

    ADDY.location := result.place;
    ADDY.stateAbbrev := result.stateAbbrev;
    ADDY.zip = parsed.zip;
    ADDY.parsed := true;
    GEOMOUT := result.address_geom;
    RATING := result.in_rating;

    RETURN NEXT;

    IF RATING = 100 THEN
      RETURN;
    END IF;
  END LOOP;

  RETURN;

END;
$_$ LANGUAGE plpgsql;
