DO $$
BEGIN
  IF (SELECT array_agg(id ORDER BY id) FROM restore_app.layer)
     IS DISTINCT FROM ARRAY[1, 2] THEN
    RAISE EXCEPTION 'Hard upgrade did not restore the rows in restore_app.layer';
  END IF;
END;
$$;
DROP TABLE restore_app.layer;
DROP SCHEMA restore_app;
