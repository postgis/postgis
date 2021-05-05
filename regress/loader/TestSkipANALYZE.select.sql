-- Checks if the table pgreg.loadedshp was analyzed by
-- querying the respective PostgreSQL system catalog
SELECT COUNT(stanumbers1)
FROM pg_statistic
WHERE starelid = 'pgreg.loadedshp'::regclass AND stanumbers1 IS NOT NULL;
