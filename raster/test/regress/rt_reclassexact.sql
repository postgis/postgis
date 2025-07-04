DROP TABLE IF EXISTS reclassexact;
CREATE TABLE reclassexact (
	id integer,
	rast raster
);

INSERT INTO reclassexact (id, rast)
SELECT 1, ST_SetValues(
    ST_AddBand(
      ST_MakeEmptyRaster(
        2,    -- width in pixels
        2,    -- height in pixels
        0,    -- upper-left x-coordinate
        0,    -- upper-left y-coordinate
        1,    -- pixel size in x-direction
        -1,   -- pixel size in y-direction (negative for north-up)
        0,    -- skew in x-direction
        0,    -- skew in y-direction
        4326  -- SRID (e.g., WGS 84)
      ),
      '32BUI'::text, -- pixel type (e.g., '32BF' for float, '8BUI' for unsigned 8-bit int)
      0.0,           -- initial value for the band (e.g., 0.0 or a no-data value)
      -99            -- nodatavalue
    ),
    1, -- band number (usually 1 for single-band rasters)
    1, -- x origin for setting values (usually 1)
    1, -- y origin for setting values (usually 1)
    ARRAY[
      ARRAY[1, 2],
      ARRAY[3, 4]
    ]::double precision[][] -- 2D array of values
  );

-- Reclass and resort input arrays
WITH rc AS (
  SELECT ST_ReclassExact(
    rast,                -- input raster
    ARRAY[4,3,2,1],      -- input map
    ARRAY[14,13,12,11],  -- output map
    1,                   -- band number to remap
    '32BUI'              -- output raster pixtype
    ) AS rast
  FROM reclassexact
  WHERE id = 1
  )
SELECT 'rce-1', (ST_DumpValues(rc.rast)).*
FROM rc;

-- Reclass to a new pixel type
WITH rc AS (
  SELECT ST_ReclassExact(
    rast,                -- input raster
    ARRAY[4,3,2,1],      -- input map
    ARRAY[14,13,12,11],  -- output map
    1,                   -- band number to remap
    '32BF'               -- output raster pixtype
    ) AS rast
  FROM reclassexact
  WHERE id = 1
  )
SELECT 'rce-2', (ST_DumpValues(rc.rast)).*
FROM rc;

-- Mismatched array sizes
WITH rc AS (
  SELECT ST_ReclassExact(
    rast,                -- input raster
    ARRAY[4,3,2,1,0],      -- input map
    ARRAY[14,13,12,11],  -- output map
    1,                   -- band number to remap
    '32BF'               -- output raster pixtype
    ) AS rast
  FROM reclassexact
  WHERE id = 1
  )
SELECT 'err-1', (ST_DumpValues(rc.rast)).*
FROM rc;

-- Bogus pixel type
WITH rc AS (
  SELECT ST_ReclassExact(
    rast,                -- input raster
    ARRAY[4,3,2,1],      -- input map
    ARRAY[14,13,12,11],  -- output map
    1,                   -- band number to remap
    '32BFF'               -- output raster pixtype
    ) AS rast
  FROM reclassexact
  WHERE id = 1
  )
SELECT 'err-2', (ST_DumpValues(rc.rast)).*
FROM rc;

DROP TABLE IF EXISTS reclassexact;
