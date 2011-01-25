CREATE OR REPLACE FUNCTION geocode_get_point(VARCHAR) RETURNS GEOMETRY
AS $_$
DECLARE
  ans RECORD;
BEGIN
  ans := geocode(NULL, $1);

  RETURN centroid(ans.geom);
END;
$_$ LANGUAGE plpgsql;
