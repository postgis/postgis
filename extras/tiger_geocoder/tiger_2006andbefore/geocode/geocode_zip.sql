CREATE OR REPLACE FUNCTION geocode_zip(
    parsed NORM_ADDY
) RETURNS REFCURSOR
AS $_$
DECLARE
  result REFCURSOR;
  tempString VARCHAR;
  tempInt INTEGER;
BEGIN
  -- Check to see if the road name can be matched.
  SELECT INTO tempInt count(*)
    FROM zip_lookup_base zip
    JOIN state_lookup sl on (zip.state = sl.name)
    JOIN zt99_d00 zl ON (zip.zip = zl.zcta::integer)
    WHERE zip = parsed.zip;

  IF tempInt = 0 THEN
    RETURN NULL;
  END IF;

  OPEN result FOR
  SELECT
      NULL::varchar(2) as fedirp,
      NULL::varchar(30) as fename,
      NULL::varchar(4) as fetype,
      NULL::varchar(2) as fedirs,
      coalesce(zip.city) as place,
      sl.abbrev as state,
      parsed.zip as zip,
      centroid(wkb_geometry) as address_geom,
      100::integer as rating
  FROM
    zip_lookup_base zip
    JOIN state_lookup sl on (zip.state = sl.name)
    JOIN zt99_d00 zl ON (zip.zip = zl.zcta::integer)
  WHERE
    zip.zip = parsed.zip;

  RETURN result;
END;
$_$ LANGUAGE plpgsql;
