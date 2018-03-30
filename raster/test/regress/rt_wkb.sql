SET postgis.gdal_enabled_drivers = 'GTiff';
SET postgis.enable_outdb_rasters = TRUE;

-----------------------------------------------------------------------
--- ST_AsWKB
-----------------------------------------------------------------------

WITH foo AS (
	SELECT
		rid,
		ST_AsWKB(rast, FALSE) AS outout,
		ST_AsWKB(rast, TRUE) AS outin
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE encode(outout, 'base64') = encode(outin, 'base64');

WITH foo AS (
	SELECT
		rid,
		rast::bytea AS outbytea,
		ST_AsWKB(rast, FALSE) AS outout
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE encode(outbytea, 'base64') != encode(outout, 'base64');

WITH foo AS (
	SELECT
		rid,
		rast::bytea AS outbytea,
		ST_AsWKB(rast, TRUE) AS outin
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE encode(outbytea, 'base64') = encode(outin, 'base64');

-----------------------------------------------------------------------
--- ST_AsBinary
-----------------------------------------------------------------------

WITH foo AS (
	SELECT
		rid,
		ST_AsBinary(rast, FALSE) AS outout,
		ST_AsBinary(rast, TRUE) AS outin
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE encode(outout, 'base64') = encode(outin, 'base64');

WITH foo AS (
	SELECT
		rid,
		rast::bytea AS outbytea,
		ST_AsBinary(rast, FALSE) AS outout
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE encode(outbytea, 'base64') != encode(outout, 'base64');

WITH foo AS (
	SELECT
		rid,
		rast::bytea AS outbytea,
		ST_AsBinary(rast, TRUE) AS outin
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE encode(outbytea, 'base64') = encode(outin, 'base64');

-----------------------------------------------------------------------
--- ST_AsHexWKB
-----------------------------------------------------------------------

WITH foo AS (
	SELECT
		rid,
		ST_AsHexWKB(rast, FALSE) AS outout,
		ST_AsHexWKB(rast, TRUE) AS outin
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE outout = outin;

WITH foo AS (
	SELECT
		rid,
		ST_AsWKB(rast, FALSE) AS outout,
		ST_AsHexWKB(rast, FALSE) AS outouthex
	FROM raster_outdb_template
)
SELECT
	rid,
	encode(outout, 'hex'),
	outouthex
FROM foo
WHERE upper(encode(outout, 'hex')) != upper(outouthex);

WITH foo AS (
	SELECT
		rid,
		ST_AsWKB(rast, TRUE) AS outin,
		ST_AsHexWKB(rast, TRUE) AS outinhex
	FROM raster_outdb_template
)
SELECT
	rid,
	encode(outin, 'hex'),
	outinhex
FROM foo
WHERE upper(encode(outin, 'hex')) != upper(outinhex);

-----------------------------------------------------------------------
--- ST_FromWKB
-----------------------------------------------------------------------

WITH foo AS (
	SELECT
		rid,
		ST_AsWKB(rast, FALSE) AS wkb
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE encode(ST_AsWKB(ST_RastFromWKB(wkb)), 'base64') != encode(wkb, 'base64');

WITH foo AS (
	SELECT
		rid,
		ST_AsWKB(rast, FALSE) AS wkbout,
		ST_AsWKB(rast, TRUE) AS wkbin
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE encode(ST_AsWKB(ST_RastFromWKB(wkbin)), 'base64') = encode(wkbout, 'base64');

-----------------------------------------------------------------------
--- ST_FromHexWKB
-----------------------------------------------------------------------
WITH foo AS (
	SELECT
		rid,
		ST_AsHexWKB(rast, FALSE) AS hexwkb
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE ST_AsHexWKB(ST_RastFromHexWKB(hexwkb)) != hexwkb;

WITH foo AS (
	SELECT
		rid,
		ST_AsHexWKB(rast, FALSE) AS hexwkbout,
		ST_AsHexWKB(rast, TRUE) AS hexwkbin
	FROM raster_outdb_template
)
SELECT
	rid
FROM foo
WHERE ST_AsHexWKB(ST_RastFromHexWKB(hexwkbin)) = hexwkbout;
