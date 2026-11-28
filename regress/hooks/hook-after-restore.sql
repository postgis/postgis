INSERT INTO public.topology (name) VALUES ('third');
DO $$
BEGIN
  IF (SELECT array_agg(id ORDER BY id) FROM restore_app.layer)
     IS DISTINCT FROM ARRAY[1, 2] THEN
    RAISE EXCEPTION 'Hard upgrade did not restore the rows in restore_app.layer';
  END IF;
  IF (SELECT array_agg(id ORDER BY id) FROM public.layer)
     IS DISTINCT FROM ARRAY[3, 4] THEN
    RAISE EXCEPTION 'Hard upgrade did not restore the rows in public.layer';
  END IF;
  IF NOT EXISTS (
    SELECT 1 FROM pg_catalog.pg_constraint
    WHERE conrelid = 'public.layer'::regclass AND contype = 'p'
  ) THEN
    RAISE EXCEPTION 'Hard upgrade did not restore the primary key in public.layer';
  END IF;
  IF (SELECT array_agg(name ORDER BY id) FROM public.topology)
     IS DISTINCT FROM ARRAY['first', 'second', 'third']
     OR (SELECT id FROM public.topology WHERE name = 'third') IS DISTINCT FROM 3 THEN
    RAISE EXCEPTION 'Hard upgrade did not restore rows and sequence state in public.topology';
  END IF;
  IF (SELECT array_agg(contype ORDER BY contype) FROM pg_catalog.pg_constraint
      WHERE conrelid = 'public.topology'::regclass AND contype IN ('p', 'u'))
     IS DISTINCT FROM ARRAY['p'::"char", 'u'::"char"] THEN
    RAISE EXCEPTION 'Hard upgrade did not restore the keys in public.topology';
  END IF;
END;
$$;
DROP TABLE public.topology;
DROP TABLE public.layer;
DROP TABLE restore_app.layer;
DROP SCHEMA restore_app;
