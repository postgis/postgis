CREATE TABLE upgrade_test(g1 geometry, g2 geography);
INSERT INTO upgrade_test(g1,g2) VALUES
('POINT(0 0)', 'LINESTRING(0 0, 1 1)'),
('POINT(1 0)', 'LINESTRING(0 1, 1 1)');

-- We know upgrading with an st_union() based view
-- fails unless you're on PostgreSQL 12, so we don't
-- even try that.
--
-- We could re-enable this test IF we fix the upgrade
-- in pre-12 versions. Refer to
-- https://trac.osgeo.org/postgis/ticket/4386
--
DO $BODY$
DECLARE
	vernum INT;
BEGIN
	show server_version_num INTO vernum;
	IF vernum >= 120000
	THEN
		RAISE DEBUG '12+ server (%)', vernum;
    CREATE VIEW upgrade_view_test AS
    SELECT ST_Union(g1) FROM upgrade_test;
	END IF;
END;
$BODY$ LANGUAGE 'plpgsql';
