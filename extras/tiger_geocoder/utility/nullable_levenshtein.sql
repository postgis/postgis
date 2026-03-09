-- This function take two arguments.  The first is the "given string" and
-- must not be null.  The second argument is the "compare string" and may
-- or may not be null.  If the second string is null, the value returned is
-- 3, otherwise it is the levenshtein difference between the two.
-- Change 2010-10-18 Regina Obe - name verbose to var_verbose since get compile error in PostgreSQL 9.0
CREATE OR REPLACE FUNCTION nullable_levenshtein(VARCHAR, VARCHAR) RETURNS INTEGER
AS $$
  SELECT CASE
      WHEN $1 IS NULL THEN NULL
      WHEN $2 IS NOT NULL AND $2 != '' THEN @extschema@.levenshtein_ignore_case($1, $2)
      ELSE 3
  END;
$$ LANGUAGE sql IMMUTABLE COST 10;
