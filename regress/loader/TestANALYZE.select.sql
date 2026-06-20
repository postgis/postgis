-- Expecting the table to have been analyzed
SELECT COUNT(stanumbers1)
FROM pg_statistic
WHERE starelid = 'public.loadedshp'::regclass AND stanumbers1 IS NOT NULL;
SELECT COUNT(*)
FROM pg_indexes
WHERE schemaname = 'public'
  AND tablename = 'loadedshp'
  AND indexdef LIKE 'CREATE INDEX % ON public.loadedshp USING gist (the_geom)';
