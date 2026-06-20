SELECT count(*) FROM loadedrast;
SELECT ST_Width(rast), ST_Height(rast), ST_NumBands(rast) FROM loadedrast WHERE rid = 1;
SELECT ST_Value(rast, 1, 1, 1) FROM loadedrast WHERE rid = 1;
SELECT COUNT(*)
FROM pg_indexes
WHERE schemaname = 'public'
  AND tablename = 'loadedrast'
  AND indexdef LIKE 'CREATE INDEX loadedrast_rast_gist ON public.loadedrast USING gist (st_convexhull(rast))';
