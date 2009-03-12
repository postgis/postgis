CREATE OR REPLACE FUNCTION geocode_address_countysub_fuzzy(
    result REFCURSOR,
    parsed NORM_ADDY
) RETURNS REFCURSOR
AS $_$
DECLARE
  result REFCURSOR;
  tempString VARCHAR;
  tempInt INTEGER;
BEGIN
  -- The first step is to determine what weve been given, and if its enough.
  IF parsed.location IS NULL THEN
    -- location is manditory.  This is the location geocoder after all.
    RETURN NULL;
  END IF;

  -- Check to see if the road name can be matched.
  IF parsed.stateAbbrev IS NOT NULL THEN
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads
        WHERE soundex(parsed.location) = soundex(tiger_geocode_roads.cousub)
        AND soundex(parsed.streetName) = soundex(tiger_geocode_roads.fename)
        AND parsed.stateAbbrev = tiger_geocode_roads.state;
  ELSE
    SELECT INTO tempInt count(*) FROM tiger_geocode_roads
        WHERE soundex(parsed.location) = soundex(tiger_geocode_roads.cousub)
        AND soundex(parsed.streetName) = soundex(tiger_geocode_roads.fename);
  END IF;

  IF tempInt = 0 THEN
    RETURN NULL;
  END IF;

  -- The road name matches, now we check to see if the addresses match
  IF parsed.stateAbbrev IS NOT NULL THEN
    SELECT INTO tempInt count(*)
    FROM (
      SELECT *, rate_attributes(parsed.preDirAbbrev, tiger_geocode_roads.fedirp,
        parsed.streetName, tiger_geocode_roads.fename, parsed.streetTypeAbbrev,
        tiger_geocode_roads.fetype, parsed.postDirAbbrev,
        tiger_geocode_roads.fedirs) as rating
      FROM tiger_geocode_roads
      WHERE soundex(parsed.location) = soundex(tiger_geocode_roads.cousub)
        AND soundex(parsed.streetName) = soundex(tiger_geocode_roads.fename)
        AND parsed.stateAbbrev = tiger_geocode_roads.state
        ) AS subquery, roads_local
    WHERE includes_address(parsed.address, roads_local.fraddl, roads_local.toaddl,
        roads_local.fraddr, roads_local.toaddr)
      AND subquery.tlid = roads_local.tlid;
  ELSE
    SELECT INTO tempInt count(*)
    FROM (
      SELECT *, rate_attributes(parsed.preDirAbbrev, tiger_geocode_roads.fedirp,
        parsed.streetName, tiger_geocode_roads.fename, parsed.streetTypeAbbrev,
        tiger_geocode_roads.fetype, parsed.postDirAbbrev,
        tiger_geocode_roads.fedirs) as rating
      FROM tiger_geocode_roads
      WHERE soundex(parsed.location) = soundex(tiger_geocode_roads.cousub)
        AND soundex(parsed.streetName) = soundex(tiger_geocode_roads.fename)
        ) AS subquery, roads_local
    WHERE includes_address(parsed.address, roads_local.fraddl, roads_local.toaddl,
        roads_local.fraddr, roads_local.toaddr)
      AND subquery.tlid = roads_local.tlid;
  END IF;

  IF tempInt = 0 THEN
    RETURN NULL;
  END IF;

  IF parsed.stateAbbrev IS NOT NULL THEN
    OPEN result FOR
    SELECT
        roads_local.fedirp as fedirp,
        roads_local.fename as fename,
        roads_local.fetype as fetype,
        roads_local.fedirs as fedirs,
        CASE WHEN (parsed.address % 2) = roads_local.fraddl
          OR (parsed.address % 2) = roads_local.toaddl
          THEN coalesce(pl.name,zipl.city,csl.name,col.name) ELSE coalesce(pr.name,zipr.city,csr.name,cor.name) END as place,
        CASE WHEN (parsed.address % 2) = roads_local.fraddl
          OR (parsed.address % 2) = roads_local.toaddl
          THEN sl.abbrev ELSE sr.abbrev END as state,
        CASE WHEN (parsed.address % 2) = roads_local.fraddl
          OR (parsed.address % 2) = roads_local.toaddl
          THEN zipl ELSE zipr END as zip,
        interpolate_from_address(parsed.address, roads_local.fraddl,
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr,
            roads_local.geom) as address_geom,
        subquery.rating as rating
    FROM (
      SELECT *, rate_attributes(parsed.preDirAbbrev, tiger_geocode_roads.fedirp,
        parsed.streetName, tiger_geocode_roads.fename, parsed.streetTypeAbbrev,
        tiger_geocode_roads.fetype, parsed.postDirAbbrev,
        tiger_geocode_roads.fedirs, parsed.location,
        tiger_geocode_roads.cousub) as rating
      FROM tiger_geocode_roads
      WHERE soundex(parsed.location) = soundex(tiger_geocode_roads.cousub)
        AND soundex(parsed.streetName) = soundex(tiger_geocode_roads.fename)
        AND parsed.stateAbbrev = tiger_geocode_roads.state
        ) AS subquery
        JOIN roads_local ON (subquery.tlid = roads_local.tlid)
        JOIN state_lookup sl ON (roads_local.statel = sl.st_code)
        JOIN state_lookup sr ON (roads_local.stater = sr.st_code)
        LEFT JOIN place_lookup pl ON (roads_local.statel = pl.st_code AND roads_local.placel = pl.pl_code)
        LEFT JOIN place_lookup pr ON (roads_local.stater = pr.st_code AND roads_local.placer = pr.pl_code)
        LEFT JOIN county_lookup col ON (roads_local.statel = col.st_code AND roads_local.countyl = col.co_code)
        LEFT JOIN county_lookup cor ON (roads_local.stater = cor.st_code AND roads_local.countyr = cor.co_code)
        LEFT JOIN countysub_lookup csl ON (roads_local.statel = csl.st_code AND roads_local.countyl = csl.co_code AND roads_local.cousubl = csl.cs_code)
        LEFT JOIN countysub_lookup csr ON (roads_local.stater = csr.st_code AND roads_local.countyr = csr.co_code AND roads_local.cousubr = csr.cs_code)
        LEFT JOIN zip_lookup_base zipl ON (roads_local.zipl = zipl.zip)
        LEFT JOIN zip_lookup_base zipr ON (roads_local.zipr = zipr.zip)
    WHERE includes_address(parsed.address, roads_local.fraddl, roads_local.toaddl,
        roads_local.fraddr, roads_local.toaddr)
    ORDER BY subquery.rating;
    RETURN result;
  ELSE
    OPEN result FOR
    SELECT
        roads_local.fedirp as fedirp,
        roads_local.fename as fename,
        roads_local.fetype as fetype,
        roads_local.fedirs as fedirs,
        CASE WHEN (parsed.address % 2) = roads_local.fraddl
          OR (parsed.address % 2) = roads_local.toaddl
          THEN coalesce(pl.name,zipl.city,csl.name,col.name) ELSE coalesce(pr.name,zipr.city,csr.name,cor.name) END as place,
        CASE WHEN (parsed.address % 2) = roads_local.fraddl
          OR (parsed.address % 2) = roads_local.toaddl
          THEN sl.abbrev ELSE sr.abbrev END as state,
        CASE WHEN (parsed.address % 2) = roads_local.fraddl
          OR (parsed.address % 2) = roads_local.toaddl
          THEN zipl ELSE zipr END as zip,
        interpolate_from_address(parsed.address, roads_local.fraddl,
            roads_local.toaddl, roads_local.fraddr, roads_local.toaddr,
            roads_local.geom) as address_geom,
        subquery.rating as rating
    FROM (
      SELECT *, rate_attributes(parsed.preDirAbbrev, tiger_geocode_roads.fedirp,
        parsed.streetName, tiger_geocode_roads.fename, parsed.streetTypeAbbrev,
        tiger_geocode_roads.fetype, parsed.postDirAbbrev,
        tiger_geocode_roads.fedirs, parsed.location,
        tiger_geocode_roads.cousub) as rating
      FROM tiger_geocode_roads
      WHERE soundex(parsed.location) = soundex(tiger_geocode_roads.cousub)
        AND soundex(parsed.streetName) = soundex(tiger_geocode_roads.fename)
        ) AS subquery
        JOIN roads_local ON (subquery.tlid = roads_local.tlid)
        JOIN state_lookup sl ON (roads_local.statel = sl.st_code)
        JOIN state_lookup sr ON (roads_local.stater = sr.st_code)
        LEFT JOIN place_lookup pl ON (roads_local.statel = pl.st_code AND roads_local.placel = pl.pl_code)
        LEFT JOIN place_lookup pr ON (roads_local.stater = pr.st_code AND roads_local.placer = pr.pl_code)
        LEFT JOIN county_lookup col ON (roads_local.statel = col.st_code AND roads_local.countyl = col.co_code)
        LEFT JOIN county_lookup cor ON (roads_local.stater = cor.st_code AND roads_local.countyr = cor.co_code)
        LEFT JOIN countysub_lookup csl ON (roads_local.statel = csl.st_code AND roads_local.countyl = csl.co_code AND roads_local.cousubl = csl.cs_code)
        LEFT JOIN countysub_lookup csr ON (roads_local.stater = csr.st_code AND roads_local.countyr = csr.co_code AND roads_local.cousubr = csr.cs_code)
        LEFT JOIN zip_lookup_base zipl ON (roads_local.zipl = zipl.zip)
        LEFT JOIN zip_lookup_base zipr ON (roads_local.zipr = zipr.zip)
    WHERE includes_address(parsed.address, roads_local.fraddl, roads_local.toaddl,
        roads_local.fraddr, roads_local.toaddr)
    ORDER BY subquery.rating;
    RETURN result;
  END IF;
END;
$_$ LANGUAGE plpgsql;
