CREATE OR REPLACE FUNCTION geocode(
    input VARCHAR,
    OUT ADDY NORM_ADDY,
    OUT GEOMOUT GEOMETRY,
    OUT RATING INTEGER
) RETURNS SETOF RECORD
AS $_$
DECLARE
  result REFCURSOR;
  rec RECORD;
BEGIN

  IF input IS NULL THEN
    RETURN;
  END IF;

  -- Pass the input string into the address normalizer
  ADDY := normalize_address(input);
  IF NOT ADDY.parsed THEN
    RETURN;
  END IF;

  OPEN result FOR SELECT * FROM geocode(ADDY);

  LOOP
    FETCH result INTO rec;

    IF NOT FOUND THEN
        RETURN;
    END IF;

    ADDY := rec.addy;
    GEOMOUT := rec.geomout;
    RATING := rec.rating;

    RETURN NEXT;
  END LOOP;

  RETURN;

END;
$_$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION geocode(
    IN_ADDY NORM_ADDY,
    OUT ADDY NORM_ADDY,
    OUT GEOMOUT GEOMETRY,
    OUT RATING INTEGER
) RETURNS SETOF RECORD
AS $_$
DECLARE
  result REFCURSOR;
  rec RECORD;
BEGIN

  IF NOT IN_ADDY.parsed THEN
    RETURN;
  END IF;

  -- Go for the full monty if we've got enough info
  IF IN_ADDY.address IS NOT NULL AND
      IN_ADDY.streetName IS NOT NULL AND
      (IN_ADDY.zip IS NOT NULL OR IN_ADDY.stateAbbrev IS NOT NULL) THEN

    result := geocode_address(IN_ADDY);
  END IF;

  -- Next best is zipcode, if we've got it
  IF result IS NULL AND IN_ADDY.zip IS NOT NULL THEN
    result := geocode_zip(IN_ADDY);
  END IF;

  -- No zip code, try state/location, need both or we'll get too much stuffs.
  IF result IS NULL AND IN_ADDY.stateAbbrev IS NOT NULL AND IN_ADDY.location IS NOT NULL THEN
    result := geocode_location(IN_ADDY);
  END IF;

  IF result IS NULL THEN
    RETURN;
  END IF;

  ADDY.address := IN_ADDY.address;
  ADDY.internal := IN_ADDY.internal;

  LOOP
    FETCH result INTO rec;

    IF NOT FOUND THEN
        RETURN;
    END IF;

    ADDY.preDirAbbrev := rec.fedirp;
    ADDY.streetName := rec.fename;
    ADDY.streetTypeAbbrev := rec.fetype;
    ADDY.postDirAbbrev := rec.fedirs;
    ADDY.location := rec.place;
    ADDY.stateAbbrev := rec.state;
    ADDY.zip := rec.zip;
    ADDY.parsed := TRUE;

    GEOMOUT := rec.address_geom;
    RATING := rec.rating;

    RETURN NEXT;
  END LOOP;

  RETURN;

END;
$_$ LANGUAGE plpgsql;
