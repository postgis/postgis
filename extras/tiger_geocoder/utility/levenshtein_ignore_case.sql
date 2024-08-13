-- This function determines the levenshtein distance irrespective of case.
CREATE OR REPLACE FUNCTION levenshtein_ignore_case(VARCHAR, VARCHAR) RETURNS INTEGER
AS $_$
  SELECT @extschema:fuzzystrmatch@.levenshtein(COALESCE(upper($1),''), COALESCE(upper($2),''));
$_$ LANGUAGE sql IMMUTABLE;
