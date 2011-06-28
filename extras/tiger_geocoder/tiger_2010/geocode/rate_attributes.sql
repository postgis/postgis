--$Id$
-- rate_attributes(dirpA, dirpB, streetNameA, streetNameB, streetTypeA,
-- streetTypeB, dirsA, dirsB, locationA, locationB)
-- Rates the street based on the given attributes.  The locations must be
-- non-null.  The other eight values are handled by the other rate_attributes
-- function, so it's requirements must also be met.
-- changed: 2010-10-18 Regina Obe - all references to verbose to var_verbose since causes compile errors in 9.0
-- changed: 2011-06-25 revise to use real named args and fix direction rating typo
CREATE OR REPLACE FUNCTION rate_attributes(dirpA VARCHAR, dirpB VARCHAR, streetNameA VARCHAR, streetNameB VARCHAR,
    streetTypeA VARCHAR, streetTypeB VARCHAR, dirsA VARCHAR, dirsB VARCHAR,  locationA VARCHAR, locationB VARCHAR) RETURNS INTEGER
AS $_$
DECLARE
--$Id$
  result INTEGER := 0;
  locationWeight INTEGER := 14;
  var_verbose BOOLEAN := FALSE;
BEGIN
  IF locationA IS NOT NULL AND locationB IS NOT NULL THEN
    result := levenshtein_ignore_case(locationA, locationB);
  ELSE
    IF var_verbose THEN
      RAISE NOTICE 'rate_attributes() - Location names cannot be null!';
    END IF;
    RETURN NULL;
  END IF;
  result := result + rate_attributes($1, $2, $3, $4, $5, $6, $7, $8);
  RETURN result;
END;
$_$ LANGUAGE plpgsql IMMUTABLE;

-- rate_attributes(dirpA, dirpB, streetNameA, streetNameB, streetTypeA,
-- streetTypeB, dirsA, dirsB)
-- Rates the street based on the given attributes.  Only streetNames are
-- required.  If any others are null (either A or B) they are treated as
-- empty strings.
CREATE OR REPLACE FUNCTION rate_attributes(dirpA VARCHAR, dirpB VARCHAR, streetNameA VARCHAR, streetNameB VARCHAR,
    streetTypeA VARCHAR, streetTypeB VARCHAR, dirsA VARCHAR, dirsB VARCHAR) RETURNS INTEGER
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
  IF streetNameA IS NOT NULL AND streetNameB IS NOT NULL THEN
    -- We want to treat numeric streets that have numerics as equal 
    -- and not penalize if they are spelled different e.g. have ND instead of TH
    IF NOT numeric_streets_equal(streetNameA, streetNameB) THEN
        result := result + levenshtein_ignore_case($3, $4) * nameWeight;
    END IF;
  ELSE
    IF var_verbose THEN
      RAISE NOTICE 'rate_attributes() - Street names cannot be null!';
    END IF;
    RETURN NULL;
  END IF;
  result := result + levenshtein_ignore_case(cull_null(streetTypeA), cull_null(streetTypeB)) *
      typeWeight;
  result := result + levenshtein_ignore_case(cull_null(dirsA), cull_null(dirsB)) *
      directionWeight;
  return result;
END;
$_$ LANGUAGE plpgsql IMMUTABLE;
