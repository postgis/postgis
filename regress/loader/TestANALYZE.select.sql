-- Expecting the table to have been analyzed
SELECT COUNT(stanumbers1)
FROM pg_statistic
WHERE starelid = 'public.loadedshp'::regclass AND stanumbers1 IS NOT NULL;
