-- Returns the value passed, or an empty string if null.
-- This is used to concatinate values that may be null.
CREATE OR REPLACE FUNCTION cull_null(VARCHAR) RETURNS VARCHAR
AS $_$
BEGIN
    RETURN coalesce($1,'');
END;
$_$ LANGUAGE plpgsql;
