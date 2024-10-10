DO $SQL$
DECLARE
	rec RECORD;
	sql TEXT;
	test_ext TEXT;
	from_version TEXT;
BEGIN

	-- TODO: test all extensions ?
	test_ext := 'postgis';

	FOR rec IN
		SELECT * FROM pg_available_extension_versions
		WHERE name = test_ext
		AND version NOT LIKE '%next'
		AND version NOT IN ( 'unpackaged', 'ANY')
		ORDER BY string_to_array(
			regexp_replace(version, '[^0-9.]', '', 'g'), '.'
		)::int[] DESC
	LOOP
		IF from_version IS NULL THEN
			from_version := rec.version;
			sql := format('CREATE EXTENSION %I VERSION %L', test_ext, from_version);
			EXECUTE sql;
		ELSE
			sql := format('ALTER EXTENSION %I UPDATE TO %L', test_ext, rec.version);
			BEGIN
				EXECUTE sql;
				-- expect error like Downgrade of postgis from version 3.4.0dev 3.3.0rc2-808-g6080076af to version 3.3.3dev is forbidden
			EXCEPTION WHEN OTHERS THEN
				IF SQLERRM LIKE 'Downgrade % is forbidden' THEN
					RAISE NOTICE 'PASS: %', SQLERRM;
				ELSE
					RAISE EXCEPTION 'FAIL: % to % downgrade was not handled: %',
						from_version, rec.version, SQLERRM;
				END IF;
			END;
		END IF;
	END LOOP;

	sql := format('DROP EXTENSION IF EXISTS %I', test_ext);
	EXECUTE sql;

END

$SQL$ LANGUAGE 'plpgsql';

