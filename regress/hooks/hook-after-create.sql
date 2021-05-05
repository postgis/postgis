--
-- Disable autovacuum on the tables created as part of the test suite
-- to make sure autovacuum does not run VACUUM or ANALYZE on the tables
-- created by the test suite.
--
-- Some of the tests are designed with the assumption that ANALYZE will
-- only be run explicitly, and not from an external source like autovacuum.
--

CREATE OR REPLACE FUNCTION trg_test_disable_table_autovacuum()
RETURNS event_trigger
LANGUAGE plpgsql
AS $$
DECLARE
    obj record;
BEGIN
    FOR obj IN SELECT * FROM pg_event_trigger_ddl_commands() WHERE command_tag in ('CREATE TABLE','CREATE TABLE AS')
    LOOP
        IF obj.object_identity = 'public.spatial_ref_sys' THEN
            CONTINUE;
        END IF;
        EXECUTE format('ALTER TABLE %s SET (autovacuum_enabled = false);',obj.object_identity);
    END LOOP;
END;
$$;

CREATE EVENT TRIGGER trg_autovac_disable ON ddl_command_end
WHEN TAG IN ('CREATE TABLE','CREATE TABLE AS')
EXECUTE PROCEDURE trg_test_disable_table_autovacuum();
