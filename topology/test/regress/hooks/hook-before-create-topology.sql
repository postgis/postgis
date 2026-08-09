DO $body$
BEGIN
    EXECUTE pg_catalog.format(
        'ALTER DATABASE %I SET search_path = "$user", public',
        pg_catalog.current_database()
    );
END
$body$;

CREATE SCHEMA topology;

CREATE TABLE public.addtosearchpath_overload_marker (
    schema_name text NOT NULL
);

CREATE FUNCTION topology.AddToSearchPath(text)
RETURNS text
AS $body$
BEGIN
    INSERT INTO public.addtosearchpath_overload_marker VALUES ($1);
    RETURN $1;
END
$body$
LANGUAGE plpgsql;
