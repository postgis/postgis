-- This function determines the levenshtein distance irespective of case.
CREATE OR REPLACE FUNCTION levenshtein_ignore_case(VARCHAR, VARCHAR) RETURNS INTEGER
AS $_$
DECLARE
  result INTEGER;
BEGIN
  result := levenshtein(upper($1), upper($2));
  RETURN result;
END
$_$ LANGUAGE plpgsql;
