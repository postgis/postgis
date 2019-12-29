DO $unpackage_if_needed$
BEGIN
	IF EXISTS (
		select t.typname from pg_depend d, pg_extension e, pg_type t where
		e.extname = 'postgis' and d.refclassid =
		'pg_catalog.pg_extension'::pg_catalog.regclass and d.refobjid = e.oid
		and d.classid = 'pg_type'::regclass and d.objid = t.oid
		and t.typname = 'raster'
	) THEN

		RAISE WARNING 'unpackaging raster';

		EXECUTE $unpackage$
		-- UNPACKAGE_CODE --
		$unpackage$;

		RAISE WARNING 'PostGIS Raster functionality has been unpackaged'
		USING HINT = 'type `SELECT postgis_extensions_upgrade();` to finish the upgrade.'
						' After upgrading, if you want to drop raster, run: DROP EXTENSION postgis_raster;';
	END IF;
END
$unpackage_if_needed$ LANGUAGE 'plpgsql';

