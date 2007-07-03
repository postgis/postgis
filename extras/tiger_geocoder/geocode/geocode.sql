CREATE OR REPLACE FUNCTION geocode(
    input VARCHAR,
    OUT NORM_ADDY VARCHAR,
    OUT GEOMOUT GEOMETRY,
    OUT RATING INTEGER
) RETURNS SETOF RECORD
AS $_$
DECLARE
  parsed norm_addy;
  result REFCURSOR;
  rec RECORD;
BEGIN

  IF input IS NULL THEN
    RETURN;
  END IF;

  -- Pass the input string into the address normalizer
  parsed := normalize_address(input);
  IF NOT parsed.parsed THEN
    RETURN;
  END IF;

  -- Go for the full monty if we've got enough info
  IF parsed.address IS NOT NULL AND
      parsed.streetName IS NOT NULL AND
      (parsed.zip IS NOT NULL OR parsed.stateAbbrev IS NOT NULL) THEN

    result := geocode_address(parsed);
  END IF;

  -- Next best is zipcode, if we've got it
  IF result IS NULL AND parsed.zip IS NOT NULL THEN
    result := geocode_zip(parsed);
  END IF;

  -- No zip code, try state/location, need both or we'll get too much stuffs.
  IF result IS NULL AND parsed.stateAbbrev IS NOT NULL AND parsed.location IS NOT NULL THEN
    result := geocode_location(parsed);
  END IF;

  IF result IS NULL THEN
    RETURN;
  END IF;

  ans := false;
  LOOP
    FETCH result INTO rec;

    IF NOT FOUND THEN
        RETURN;
    END IF;

    NORM_ADDY := cull_null(parsed.address::text)
              || CASE WHEN rec.fedirp IS NOT NULL THEN ' ' ELSE '' END
              || cull_null(rec.fedirp)
              || CASE WHEN rec.fename IS NOT NULL THEN ' ' ELSE '' END
              || cull_null(rec.fename)
              || CASE WHEN rec.fetype IS NOT NULL THEN ' ' ELSE '' END
              || cull_null(rec.fetype)
              || CASE WHEN rec.fedirs IS NOT NULL THEN ' ' ELSE '' END
              || cull_null(rec.fedirs)
              || CASE WHEN
                   parsed.address IS NOT NULL OR
                   rec.fename IS NOT NULL
                   THEN ', ' ELSE '' END
              || cull_null(parsed.internal)
              || CASE WHEN parsed.internal IS NOT NULL THEN ', ' ELSE '' END
              || cull_null(rec.place)
              || CASE WHEN rec.place IS NOT NULL THEN ', ' ELSE '' END
              || cull_null(rec.state)
              || CASE WHEN rec.state IS NOT NULL THEN ' ' ELSE '' END
              || cull_null(lpad(rec.zip,5,'0'));

    GEOMOUT := rec.address_geom;
    RATING := rec.rating;

    RETURN NEXT;
    END IF;
  END LOOP;

  RETURN;

END;
$_$ LANGUAGE plpgsql;
