SELECT count(*) FROM raster_outdb_template;
SELECT DISTINCT ST_BandFileSize(rast) FROM raster_outdb_template;
SELECT DISTINCT ST_BandFileTimestamp(rast) FROM raster_outdb_template;
SET postgis.enable_outdb_rasters = true;
SELECT DISTINCT ST_BandFileSize(rast) FROM raster_outdb_template;
SELECT DISTINCT ST_BandFileTimestamp(rast) != 0 FROM raster_outdb_template;