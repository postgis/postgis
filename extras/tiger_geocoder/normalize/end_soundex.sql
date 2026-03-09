-- Runs the soundex function on the last word in the string provided.
-- Words are allowed to be separated by space, comma, period, new-line
-- tab or form feed.
CREATE OR REPLACE FUNCTION end_soundex(VARCHAR) RETURNS VARCHAR
AS $$
  SELECT @extschema:fuzzystrmatch@.soundex(
      COALESCE(substring($1, E'[ ,.\n\t\f]([a-zA-Z0-9]*)$'), $1)
  );
$$ LANGUAGE sql IMMUTABLE;
