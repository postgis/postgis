-- See https://trac.osgeo.org/postgis/ticket/5633
DO $BODY$
BEGIN
	EXECUTE format(
		$$
			ALTER DATABASE %I
			SET standard_conforming_strings to off
		$$,
		current_database()
	);

	EXECUTE format(
		$$
			ALTER DATABASE %I
			SET escape_string_warning to on
		$$,
		current_database()
	);
END;
$BODY$ LANGUAGE 'plpgsql';
