---------------------------------------------------------------------
-- ST_SummaryStatsAgg AGGREGATE
-- Compute summary statistics for an aggregation of raster.
--
-- Exemple
-- SELECT (aws).count, 
--        (aws).sum, 
--        (aws).mean, 
--        (aws).min, 
--        (aws).max
-- FROM (SELECT ST_SummaryStatsAgg(gv) aws
--       FROM (SELECT ST_Clip(rt.rast, gt.geom) gv
--             FROM rasttable rt, geomtable gt
--             WHERE ST_Intersects(rt.rast, gt.geom)
--            ) foo
--       GROUP BY gt.id
--      ) foo2
---------------------------------------------------------------------

-- DROP TYPE summarystatsstate CASCADE;
CREATE TYPE summarystatsstate AS (
    count int,
    sum double precision,
    min double precision, 
    max double precision
);

---------------------------------------------------------------------
-- raster_summarystatsstate
-- State function used by the ST_SummaryStatsAgg aggregate
CREATE OR REPLACE FUNCTION raster_summarystatsstate(sss summarystatsstate, rast raster, nband int DEFAULT 1, exclude_nodata_value boolean DEFAULT TRUE, sample_percent double precision DEFAULT 1)
    RETURNS summarystatsstate 
    AS $$
    DECLARE
        newstats summarystats;
        ret summarystatsstate;
    BEGIN
        IF rast IS NULL THEN
            RETURN sss;
        END IF;
        newstats := _ST_SummaryStats(rast, nband, exclude_nodata_value, sample_percent);
        IF $1 IS NULL THEN
            ret := (newstats.count, 
                    newstats.sum, 
                    newstats.min, 
                    newstats.max)::summarystatsstate;
        ELSE
            ret := (sss.count + newstats.count, 
                    sss.sum + newstats.sum, 
                    least(sss.min, newstats.min), 
                    greatest(sss.max, newstats.max))::summarystatsstate;
        END IF;
RAISE NOTICE 'min=% ',ret.min;            
        RETURN ret;
    END;
    $$
    LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION raster_summarystatsstate(sss summarystatsstate, rast raster)
    RETURNS summarystatsstate 
    AS $$
        SELECT raster_summarystatsstate($1, $2, 1, true, 1);
    $$ LANGUAGE 'SQL';

---------------------------------------------------------------------
-- raster_summarystatsfinal
-- Final function used by the ST_SummaryStatsAgg aggregate 
CREATE OR REPLACE FUNCTION raster_summarystatsfinal(sss summarystatsstate)
    RETURNS summarystats 
    AS $$
    DECLARE
        ret summarystats;
    BEGIN
        ret := (($1).count,
                ($1).sum,
                ($1).sum / ($1).count,
                null,
                ($1).min,
                ($1).max
               )::summarystats;
        RETURN ret;
    END;
    $$
    LANGUAGE 'plpgsql';

---------------------------------------------------------------------
-- ST_SummaryStatsAgg AGGREGATE
---------------------------------------------------------------------
CREATE AGGREGATE ST_SummaryStatsAgg(raster, int, boolean, double precision) (
  SFUNC=raster_summarystatsstate,
  STYPE=summarystatsstate,
  FINALFUNC=raster_summarystatsfinal
);

CREATE AGGREGATE ST_SummaryStatsAgg(raster) (
  SFUNC=raster_summarystatsstate,
  STYPE=summarystatsstate,
  FINALFUNC=raster_summarystatsfinal
);

-- Test
CREATE OR REPLACE FUNCTION ST_TestRaster(h integer, w integer, val float8) 
    RETURNS raster AS 
    $$
    DECLARE
    BEGIN
        RETURN ST_AddBand(ST_MakeEmptyRaster(h, w, 0, 0, 1, 1, 0, 0, -1), '32BF', val, -1);
    END;
    $$
    LANGUAGE 'plpgsql';

SELECT id,
       (sss).count, 
       (sss).sum, 
       (sss).mean, 
       (sss).stddev, 
       (sss).min, 
       (sss).max
FROM (SELECT ST_SummaryStatsAgg(rast) as sss, id
      FROM (SELECT 1 id, ST_TestRaster(2, 2, 2) rast
            UNION ALL
            SELECT 1 id, ST_TestRaster(2, 2, 4) rast
            UNION ALL
            SELECT 2 id, ST_TestRaster(2, 2, 4) rast
           ) foo
      GROUP BY id
     ) foo2

