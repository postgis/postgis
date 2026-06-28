-- helper function to determine if street type
-- should be put before or after the street name
-- note in streettype lookup this is misnamed as is_hw
-- because I originally thought only highways had that behavior
-- it applies to foreign influenced roads like Camino (for road)
CREATE OR REPLACE FUNCTION tiger.is_pretype(text) RETURNS boolean AS
$$
    SELECT EXISTS(SELECT name FROM tiger.street_type_lookup WHERE upper(name) = upper($1) AND is_hw );
$$
LANGUAGE sql IMMUTABLE STRICT; /** I know this should be stable but it's practically immutable :) **/

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

  result := COALESCE(input.address_alphanumeric, tiger.cull_null(input.address::text))
         || COALESCE(' ' || input.preDirAbbrev, '')
         || CASE WHEN tiger.is_pretype(input.streetTypeAbbrev) THEN ' ' || input.streetTypeAbbrev  ELSE '' END
         || COALESCE(' ' || input.streetName, '')
         || CASE WHEN NOT tiger.is_pretype(input.streetTypeAbbrev) THEN ' ' || input.streetTypeAbbrev  ELSE '' END
         || COALESCE(' ' || input.postDirAbbrev, '')
         || CASE WHEN
              input.address IS NOT NULL OR
              input.streetName IS NOT NULL
              THEN ', ' ELSE '' END
         || tiger.cull_null(input.internal)
         || CASE WHEN input.internal IS NOT NULL THEN ', ' ELSE '' END
         || tiger.cull_null(input.location)
         || CASE WHEN input.location IS NOT NULL THEN ', ' ELSE '' END
         || COALESCE(input.stateAbbrev || ' ' , '')
         || tiger.cull_null(input.zip) || COALESCE('-' || input.zip4, '');

  RETURN trim(result);

END;
$_$ LANGUAGE plpgsql IMMUTABLE;
