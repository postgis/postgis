-- The long action options should still perform a normal load.
SELECT count(*) FROM loadedshp;

-- --create-index combined with --if-not-exists emits a stable named index.
-- PostgreSQL stores the resulting definition without IF NOT EXISTS.
SELECT COUNT(*)
FROM pg_indexes
WHERE schemaname = 'public'
  AND tablename = 'loadedshp'
  AND indexdef LIKE 'CREATE INDEX loadedshp_the_geom_gist ON public.loadedshp USING gist (the_geom)';
