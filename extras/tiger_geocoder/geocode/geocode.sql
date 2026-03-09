-- Geocode an already parsed address, preferring precise address matches and
-- falling back to broader location matches when needed.
CREATE OR REPLACE FUNCTION geocode(
    IN_ADDY NORM_ADDY,
    max_results integer DEFAULT 10,
    restrict_geom geometry DEFAULT null,
    OUT ADDY NORM_ADDY,
    OUT GEOMOUT GEOMETRY,
    OUT RATING INTEGER
) RETURNS SETOF RECORD
AS $_$
DECLARE
  rec RECORD;
BEGIN

  IF NOT IN_ADDY.parsed THEN
    RETURN;
  END IF;

  -- Go for the full monty if we've got enough info
  IF IN_ADDY.streetName IS NOT NULL AND
      (IN_ADDY.zip IS NOT NULL OR IN_ADDY.stateAbbrev IS NOT NULL) THEN

    FOR rec IN
        SELECT *
        FROM
          (SELECT
            DISTINCT ON (
              (a.addy).address,
              (a.addy).predirabbrev,
              (a.addy).streetname,
              (a.addy).streettypeabbrev,
              (a.addy).postdirabbrev,
              (a.addy).internal,
              (a.addy).location,
              (a.addy).stateabbrev,
              (a.addy).zip
              )
            *
           FROM
             tiger.geocode_address(IN_ADDY, max_results, restrict_geom) a
           ORDER BY
              (a.addy).address,
              (a.addy).predirabbrev,
              (a.addy).streetname,
              (a.addy).streettypeabbrev,
              (a.addy).postdirabbrev,
              (a.addy).internal,
              (a.addy).location,
              (a.addy).stateabbrev,
              (a.addy).zip,
              a.rating
          ) as b
        ORDER BY b.rating LIMIT max_results
    LOOP

      ADDY := rec.addy;
      GEOMOUT := rec.geomout;
      RATING := rec.rating;

      RETURN NEXT;

      IF RATING = 0 THEN
        RETURN;
      END IF;

    END LOOP;

    IF RATING IS NOT NULL THEN
      RETURN;
    END IF;
  END IF;

  -- No zip code, try state/location, need both or we'll get too much stuffs.
  IF IN_ADDY.zip IS NOT NULL OR (IN_ADDY.stateAbbrev IS NOT NULL AND IN_ADDY.location IS NOT NULL) THEN
    FOR rec in SELECT * FROM tiger.geocode_location(IN_ADDY, restrict_geom) As b ORDER BY b.rating LIMIT max_results
    LOOP
      ADDY := rec.addy;
      GEOMOUT := rec.geomout;
      RATING := rec.rating;

      RETURN NEXT;
      IF RATING = 100 THEN
        RETURN;
      END IF;
    END LOOP;

  END IF;

  RETURN;

END;
$_$ LANGUAGE plpgsql
COST 1000
STABLE PARALLEL SAFE
ROWS 1;

-- Normalize free-form input once, then only delegate to the structured
-- geocoder overload when parsing succeeds.
CREATE OR REPLACE FUNCTION geocode(
    input VARCHAR, max_results integer DEFAULT 10,
    restrict_geom geometry DEFAULT NULL,
    OUT ADDY NORM_ADDY,
    OUT GEOMOUT GEOMETRY,
    OUT RATING INTEGER
) RETURNS SETOF RECORD
AS $$
  WITH normalized AS (
      SELECT @extschema@.normalize_address(input) AS addy
      WHERE input IS NOT NULL
  ),
  parsed AS (
      SELECT addy
      FROM normalized
      WHERE (addy).parsed
  )
  SELECT g.addy, g.geomout, g.rating
  FROM parsed
    CROSS JOIN LATERAL @extschema@.geocode(addy, max_results, restrict_geom) AS g
  ORDER BY g.rating;
$$ LANGUAGE sql COST 1000
STABLE PARALLEL SAFE
ROWS 1;
