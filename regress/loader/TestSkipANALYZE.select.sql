-- Checks if the table was analyzed by looking at the table statistics
-- in the pg system catalog
SELECT COUNT(stanumbers1)
FROM pg_statistic
WHERE starelid = 'loadedshp'::regclass AND stanumbers1 IS NOT NULL;
