-- This function determines the levenshtein distance irespective of case.
CREATE OR REPLACE FUNCTION levenshtein_ignore_case(VARCHAR, VARCHAR) RETURNS INTEGER
AS $_$
  SELECT levenshtein(COALESCE(upper($1),''), COALESCE(upper($2),''));
$_$ LANGUAGE sql IMMUTABLE;
