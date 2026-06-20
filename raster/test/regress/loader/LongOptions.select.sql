-- The long action options should still perform a normal raster load.
SELECT count(*) FROM loadedrast;
SELECT ST_Width(rast), ST_Height(rast), ST_NumBands(rast) FROM loadedrast WHERE rid = 1;
SELECT ST_Value(rast, 1, 1, 1) FROM loadedrast WHERE rid = 1;

-- --create-index combined with --if-not-exists emits a stable named index.
-- PostgreSQL stores the resulting definition without IF NOT EXISTS.
SELECT COUNT(*)
FROM pg_indexes
WHERE schemaname = 'public'
  AND tablename = 'loadedrast'
  AND indexdef LIKE 'CREATE INDEX loadedrast_rast_gist ON public.loadedrast USING gist (st_convexhull(rast))';
