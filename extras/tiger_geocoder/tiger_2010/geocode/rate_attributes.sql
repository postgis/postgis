--$Id$
-- rate_attributes(dirpA, dirpB, streetNameA, streetNameB, streetTypeA,
-- streetTypeB, dirsA, dirsB, locationA, locationB)
-- Rates the street based on the given attributes.  The locations must be
-- non-null.  The other eight values are handled by the other rate_attributes
-- function, so it's requirements must also be met.
-- changed: 2010-10-18 Regina Obe - all references to verbose to var_verbose since causes compile errors in 9.0
CREATE OR REPLACE FUNCTION rate_attributes(VARCHAR, VARCHAR, VARCHAR, VARCHAR,
    VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS INTEGER
AS $_$
DECLARE
  result INTEGER := 0;
  locationWeight INTEGER := 14;
  var_verbose BOOLEAN := FALSE;
BEGIN
  IF $9 IS NOT NULL AND $10 IS NOT NULL THEN
    result := levenshtein_ignore_case($9, $10);
  ELSE
    IF var_verbose THEN
      RAISE NOTICE 'rate_attributes() - Location names cannot be null!';
    END IF;
    RETURN NULL;
  END IF;
  result := result + rate_attributes($1, $2, $3, $4, $5, $6, $7, $8);
  RETURN result;
END;
$_$ LANGUAGE plpgsql;

-- rate_attributes(dirpA, dirpB, streetNameA, streetNameB, streetTypeA,
-- streetTypeB, dirsA, dirsB)
-- Rates the street based on the given attributes.  Only streetNames are
-- required.  If any others are null (either A or B) they are treated as
-- empty strings.
CREATE OR REPLACE FUNCTION rate_attributes(VARCHAR, VARCHAR, VARCHAR, VARCHAR,
    VARCHAR, VARCHAR, VARCHAR, VARCHAR) RETURNS INTEGER
AS $_$
DECLARE
  result INTEGER := 0;
  directionWeight INTEGER := 2;
  nameWeight INTEGER := 10;
  typeWeight INTEGER := 5;
  var_verbose BOOLEAN := FALSE;
BEGIN
  result := result + levenshtein_ignore_case(cull_null($1), cull_null($2)) *
      directionWeight;
  IF $3 IS NOT NULL AND $4 IS NOT NULL THEN
    result := result + levenshtein_ignore_case($3, $4) * nameWeight;
  ELSE
    IF var_verbose THEN
      RAISE NOTICE 'rate_attributes() - Street names cannot be null!';
    END IF;
    RETURN NULL;
  END IF;
  result := result + levenshtein_ignore_case(cull_null($5), cull_null($6)) *
      typeWeight;
  result := result + levenshtein_ignore_case(cull_null($7), cull_null($7)) *
      directionWeight;
  return result;
END;
$_$ LANGUAGE plpgsql STABLE COST 10;
