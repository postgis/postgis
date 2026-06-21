SET CLIENT_ENCODING to UTF8;
SELECT column_name
FROM information_schema.columns
WHERE table_name = 'loadedshp'
  AND column_name IN ('fid', 'gid', '__gid')
ORDER BY ordinal_position;
