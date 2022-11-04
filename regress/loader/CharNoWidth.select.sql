SET CLIENT_ENCODING to UTF8;
SELECT * FROM loadedshp;
SELECT column_name, character_maximum_length FROM information_schema.columns WHERE table_name = 'loadedshp' ORDER BY column_name;
