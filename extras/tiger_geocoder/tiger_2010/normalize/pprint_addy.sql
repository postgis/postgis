CREATE OR REPLACE FUNCTION pprint_addy(
    input NORM_ADDY
) RETURNS VARCHAR
AS $_$
DECLARE
  result VARCHAR;
BEGIN
  IF NOT input.parsed THEN
    RETURN NULL;
  END IF;

  result := cull_null(input.address::text)
         || CASE WHEN input.preDirAbbrev IS NOT NULL THEN ' ' ELSE '' END
         || cull_null(input.preDirAbbrev)
         || CASE WHEN input.streetName IS NOT NULL THEN ' ' ELSE '' END
         || cull_null(input.streetName)
         || CASE WHEN input.streetTypeAbbrev IS NOT NULL THEN ' ' ELSE '' END
         || cull_null(input.streetTypeAbbrev)
         || CASE WHEN input.postDirAbbrev IS NOT NULL THEN ' ' ELSE '' END
         || cull_null(input.postDirAbbrev)
         || CASE WHEN
              input.address IS NOT NULL OR
              input.streetName IS NOT NULL
              THEN ', ' ELSE '' END
         || cull_null(input.internal)
         || CASE WHEN input.internal IS NOT NULL THEN ', ' ELSE '' END
         || cull_null(input.location)
         || CASE WHEN input.location IS NOT NULL THEN ', ' ELSE '' END
         || cull_null(input.stateAbbrev)
         || CASE WHEN input.stateAbbrev IS NOT NULL THEN ' ' ELSE '' END
         || cull_null(input.zip);

  RETURN trim(result);

END;
$_$ LANGUAGE plpgsql IMMUTABLE;
