-- Keep geometry_columns from binding an overload in a writable schema.
CREATE FUNCTION public.acldefault(pg_catalog.text, pg_catalog.oid)
RETURNS pg_catalog.aclitem[]
LANGUAGE plpgsql
AS $$
BEGIN
	RAISE EXCEPTION 'public.acldefault(text,oid) must not be called';
END;
$$;
