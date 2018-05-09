SET postgis.enable_outdb_rasters = false;
SELECT count(*) FROM raster_outdb_template;
SELECT DISTINCT ST_BandFileSize(rast) FROM raster_outdb_template;
SELECT DISTINCT ST_BandFileTimestamp(rast) FROM raster_outdb_template;
SET postgis.enable_outdb_rasters = true;
SELECT DISTINCT ST_BandFileSize(rast) FROM raster_outdb_template;
SELECT DISTINCT ST_BandFileTimestamp(rast) != 0 FROM raster_outdb_template;

-- error cases
SELECT ST_BandFileSize(''::raster);
SELECT ST_BandFileTimestamp(''::raster);
SELECT ST_BandFileSize(rast,-1) FROM raster_outdb_template WHERE rid = 1;
SELECT ST_BandFileTimestamp(rast,-1) FROM raster_outdb_template WHERE rid = 1;
SELECT ST_BandFileSize(rast,10) FROM raster_outdb_template WHERE rid = 1;
SELECT ST_BandFileTimestamp(rast,10) FROM raster_outdb_template WHERE rid = 1;
-- valid raster, but file does not exist
SELECT ST_BandFileSize('0100000100000000000000F03F000000000000F0BF0000000000000000000000000000000000000000000000000000000000000000000000005A003200840000DEADBEEF00'::raster);
SELECT ST_BandFileTimestamp('0100000100000000000000F03F000000000000F0BF0000000000000000000000000000000000000000000000000000000000000000000000005A003200840000DEADBEEF00'::raster);
