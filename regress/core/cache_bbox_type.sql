-- A type named geometry is not necessarily PostGIS geometry.
CREATE DOMAIN pg_temp.geometry AS bytea;
CREATE TEMP TABLE cache_bbox_type_test (g pg_temp.geometry);
CREATE TRIGGER cache_bbox_type_test_trigger BEFORE INSERT ON cache_bbox_type_test
  FOR EACH ROW EXECUTE FUNCTION postgis_cache_bbox('g');

DO $$
BEGIN
  INSERT INTO cache_bbox_type_test VALUES (decode('000000000200000000000010', 'hex'));
  RAISE EXCEPTION 'cache_bbox accepted a non-geometry type';
EXCEPTION WHEN OTHERS THEN
  IF SQLERRM NOT LIKE 'trigger % requested to apply to a non-geometry field (g)' THEN
    RAISE;
  END IF;
  RAISE NOTICE 'cache_bbox rejects a non-geometry type';
END;
$$;
