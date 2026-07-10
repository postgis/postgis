SET client_min_messages TO warning;
-- Test for table without explicit schema
CREATE TABLE res1 AS SELECT
  :schema ST_AddBand(
    :schema ST_MakeEmptyRaster(10, 10, x, y, 1, -1, 0, 0, 0)
    , 1, '8BUI', 0, 0
  ) r
FROM generate_series(-170, 160, 10) x,
     generate_series(80, -70, -10) y;
SELECT :schema addrasterconstraints('res1', 'r');

SELECT :schema ST_CreateOverview('res1', 'r', 2)::text = 'o_2_res1';

SELECT :schema ST_CreateOverview('res1', 'r', 4)::text = 'o_4_res1';

SELECT :schema ST_CreateOverview('res1', 'r', 8)::text = 'o_8_res1';

SELECT :schema ST_CreateOverview('res1', 'r', 16)::text = 'o_16_res1';

SELECT r_table_name tab, r_raster_column c, srid s,
 scale_x sx, scale_y sy,
 blocksize_x w, blocksize_y h, same_alignment a,
 -- regular_blocking (why not regular?)
 --extent::box2d e,
 :schema st_covers(extent:: :schema box2d, 'BOX(-170 -80,170 80)':: :schema box2d) ec,
 :schema st_xmin(extent:: :schema box2d) = -170 as eix,
 :schema st_ymax(extent:: :schema box2d) = 80 as eiy,
 (:schema st_xmax(extent:: :schema box2d) - 170) <= scale_x as eox,
 --(st_xmax(extent::box2d) - 170) eoxd,
 abs(:schema st_ymin(extent:: :schema box2d) + 80) <= abs(scale_y) as eoy
 --,abs(st_ymin(extent::box2d) + 80) eoyd
 FROM :schema raster_columns
WHERE r_table_name like '%res1'
ORDER BY scale_x, r_table_name;

SELECT o_table_name, o_raster_column,
       r_table_name, r_raster_column, overview_factor
FROM :schema raster_overviews
WHERE r_table_name = 'res1'
ORDER BY overview_factor;

SET search_path TO '';
SELECT :schema _overview_constraint(
  (SELECT r:: :schema raster FROM public.o_2_res1 LIMIT 1),
  2, 'public', 'res1', 'r'
);
RESET search_path;

CREATE TEMP TABLE pg_class (dummy integer);
CREATE TEMP TABLE pg_namespace (dummy integer);
CREATE TEMP TABLE pg_attribute (dummy integer);
SET search_path TO pg_temp, public;
SELECT :schema _overview_constraint(
  (SELECT r:: :schema raster FROM public.o_2_res1 LIMIT 1),
  2, 'public', 'res1', 'r'
);
RESET search_path;
DROP TABLE pg_class;
DROP TABLE pg_namespace;
DROP TABLE pg_attribute;

CREATE SCHEMA rt_shadow;
CREATE DOMAIN rt_shadow."char" AS text;
SET search_path TO rt_shadow, public;
SELECT :schema _overview_constraint(
  (SELECT r:: :schema raster FROM public.o_2_res1 LIMIT 1),
  2, 'public', 'res1', 'r'
);
RESET search_path;
DROP SCHEMA rt_shadow CASCADE;

CREATE SCHEMA rt_spoof;
CREATE DOMAIN rt_spoof.raster AS text;
CREATE FUNCTION rt_spoof.postgis_raster_lib_version()
  RETURNS text LANGUAGE sql IMMUTABLE AS $$ SELECT 'spoof'::text $$;
SET search_path TO rt_spoof, public;
SELECT :schema _overview_constraint(
  (SELECT r:: :schema raster FROM public.o_2_res1 LIMIT 1),
  2, 'public', 'res1', 'r'
);
RESET search_path;
DROP SCHEMA rt_spoof CASCADE;

SELECT 'count',
(SELECT count(*) r1 from res1),
(SELECT count(*) r2 from o_2_res1),
(SELECT count(*) r4 from o_4_res1),
(SELECT count(*) r8 from o_8_res1),
(SELECT count(*) r16 from o_16_res1)
;

-- End of overview test on table without explicit schema

DROP TABLE o_16_res1;
DROP TABLE o_8_res1;
DROP TABLE o_4_res1;
DROP TABLE o_2_res1;
-- Keep the source table

-- Test overview with table in schema
--
CREATE SCHEMA oschm;
SELECT set_config(
  'search_path',
  COALESCE(NULLIF(pg_catalog.rtrim(:'schema', '.'), ''), 'public') || ',public',
  false
) IS NOT NULL;
-- offset the schema tableto distinguish it from original
-- let it be small to reduce time cost.
CREATE TABLE oschm.res1 AS SELECT
  :schema ST_AddBand(
    :schema ST_MakeEmptyRaster(10, 10, x, y, 1, -1, 0, 0, 0)
    , 1, '8BUI', 0, 0
  ) r
FROM generate_series(100, 270, 10) x,
     generate_series(140, -30, -10) y;
SELECT :schema addrasterconstraints('oschm'::name,'res1'::name, 'r'::name);

-- add overview with explicit schema
SELECT :schema ST_CreateOverview('oschm.res1', 'r', 8)::text = 'oschm.o_8_res1';

-- set search path to 'oschm' first
SELECT set_config(
  'search_path',
  'oschm,' || COALESCE(NULLIF(pg_catalog.rtrim(:'schema', '.'), ''), 'public') || ',public',
  false
) IS NOT NULL;

-- create overview with schema in search_path
SELECT :schema ST_CreateOverview('res1', 'r', 4)::text = 'o_4_res1';

-- Create overview for table in public schema with explicit path
-- at same factor of schema table
SELECT :schema ST_CreateOverview('public.res1', 'r', 8)::text = 'public.o_8_res1';

-- Reset the search_path
SELECT set_config(
  'search_path',
  COALESCE(NULLIF(pg_catalog.rtrim(:'schema', '.'), ''), 'public') || ',public',
  false
) IS NOT NULL;

-- Create public overview of public table with same name
-- and scale as schema table above using reset search path.
SELECT :schema ST_CreateOverview('res1', 'r', 4)::text = 'o_4_res1';

-- Check scale and extent
-- Offset means that original raster overviews won't match
-- extent and values only mach on schema table not public one
SELECT r_table_schema, r_table_name tab, r_raster_column c,
 srid s, scale_x sx, scale_y sy,
 blocksize_x w, blocksize_y h, same_alignment a,
 -- regular_blocking (why not regular?)
 --extent::box2d e,
 :schema st_covers(extent:: :schema box2d, 'BOX(100 -30,270 140)':: :schema box2d) ec,
 :schema st_xmin(extent:: :schema box2d) = 100 as eix,
 :schema st_ymax(extent:: :schema box2d) = 140 as eiy,
 (:schema st_xmax(extent:: :schema box2d) - 280) <= scale_x as eox,
 --(st_xmax(extent::box2d) - 170) eoxd,
 abs(:schema st_ymin(extent:: :schema box2d) + 40) <= abs(scale_y) as eoy
 --,abs(st_ymin(extent::box2d) + 80) eoyd
 FROM :schema raster_columns
WHERE r_table_name like '%res1'
ORDER BY r_table_schema, scale_x, r_table_name;

-- this test may be redundant - it's just confirming that the
-- overview factor is correct, which we do for the original table.
SELECT o_table_schema, o_table_name, o_raster_column,
       r_table_schema, r_table_name, r_raster_column, overview_factor
FROM :schema raster_overviews
WHERE r_table_name = 'res1'
AND   r_table_schema = 'oschm'
ORDER BY o_table_schema,overview_factor;
-- get the oschm table sizes
SELECT 'count',
(SELECT count(*) r1 from oschm.res1),
(SELECT count(*) r4 from oschm.o_4_res1),
(SELECT count(*) r8 from oschm.o_8_res1) ;

-- clean up scheam oschm
DROP TABLE oschm.o_8_res1;
DROP TABLE oschm.o_4_res1;
DROP TABLE oschm.res1;
DROP SCHEMA oschm;

-- Drop the tables for noschema test
DROP TABLE o_8_res1;
DROP TABLE o_4_res1;
DROP TABLE res1;

-- Overview creation should not require SELECT on the raster_columns view.
SET postgis_reg.schema TO :'schema';
DO $$
DECLARE
  can_manage_view boolean;
  can_manage_role boolean;
  had_public_select boolean;
  ext_schema text := COALESCE(NULLIF(pg_catalog.rtrim(current_setting('postgis_reg.schema'), '.'), ''), 'public');
  test_role name := format('rt_overview_acl_user_%s_%s', pg_backend_pid(), txid_current());
  test_schema name := format('rt_overview_acl_%s_%s', pg_backend_pid(), txid_current());
  created_role boolean := false;
  created_schema boolean := false;
BEGIN
  SELECT pg_has_role(c.relowner, 'USAGE')
  INTO can_manage_view
  FROM pg_class c
  JOIN pg_namespace n ON n.oid = c.relnamespace
  WHERE n.nspname = ext_schema
    AND c.relname = 'raster_columns'
    AND c.relkind = 'v';

  SELECT rolsuper
  INTO can_manage_role
  FROM pg_roles
  WHERE rolname = current_user;

  IF NOT COALESCE(can_manage_view, false) OR NOT COALESCE(can_manage_role, false) THEN
    RETURN;
  END IF;

  SELECT EXISTS (
    SELECT 1
    FROM pg_class c
    JOIN pg_namespace n ON n.oid = c.relnamespace
    CROSS JOIN LATERAL aclexplode(COALESCE(c.relacl, acldefault('r', c.relowner))) a
    WHERE n.nspname = ext_schema
      AND c.relname = 'raster_columns'
      AND c.relkind = 'v'
      AND a.grantee = 0
      AND a.privilege_type = 'SELECT'
  )
  INTO had_public_select;

  -- Roles and schemas may outlive a failed regression on a non-disposable
  -- PostgreSQL instance, so use per-run names and only remove objects created
  -- by this block.
  EXECUTE format('CREATE ROLE %I', test_role);
  created_role := true;
  EXECUTE format('CREATE SCHEMA %I AUTHORIZATION %I', test_schema, test_role);
  created_schema := true;
  EXECUTE format('GRANT USAGE ON SCHEMA %I TO %I', ext_schema, test_role);
  EXECUTE format('REVOKE SELECT ON %I.raster_columns FROM PUBLIC', ext_schema);
  EXECUTE format('SET ROLE %I', test_role);
  PERFORM set_config('search_path', format('%I, %I, public', test_schema, ext_schema), true);

  CREATE TABLE res_acl AS SELECT
    ST_AddBand(
      ST_MakeEmptyRaster(10, 10, x, y, 1, -1, 0, 0, 0)
      , 1, '8BUI', 0, 0
    ) r
  FROM generate_series(0, 10, 10) x,
       generate_series(10, 0, -10) y;
  PERFORM addrasterconstraints('res_acl', 'r');

  CREATE TABLE o_2_res_acl AS SELECT r FROM res_acl LIMIT 1;
  PERFORM AddOverviewConstraints('o_2_res_acl', 'r', 'res_acl', 'r', 2);

  RESET ROLE;
  IF had_public_select THEN
    EXECUTE format('GRANT SELECT ON %I.raster_columns TO PUBLIC', ext_schema);
  ELSE
    EXECUTE format('REVOKE SELECT ON %I.raster_columns FROM PUBLIC', ext_schema);
  END IF;
  EXECUTE format('DROP SCHEMA %I CASCADE', test_schema);
  created_schema := false;
  EXECUTE format('REVOKE USAGE ON SCHEMA %I FROM %I', ext_schema, test_role);
  EXECUTE format('DROP ROLE %I', test_role);
  created_role := false;
EXCEPTION WHEN OTHERS THEN
  RESET ROLE;
  IF had_public_select IS NOT NULL THEN
    IF had_public_select THEN
      EXECUTE format('GRANT SELECT ON %I.raster_columns TO PUBLIC', ext_schema);
    ELSE
      EXECUTE format('REVOKE SELECT ON %I.raster_columns FROM PUBLIC', ext_schema);
    END IF;
  END IF;
  IF created_schema THEN
    EXECUTE format('DROP SCHEMA IF EXISTS %I CASCADE', test_schema);
  END IF;
  IF created_role THEN
    EXECUTE format('REVOKE USAGE ON SCHEMA %I FROM %I', ext_schema, test_role);
    EXECUTE format('DROP ROLE IF EXISTS %I', test_role);
  END IF;
  RAISE;
END;
$$;

-- Reset the session environment
-- possibly a bit harsh, but we had to set the search_path
-- and need to reset it back to default.
DISCARD ALL;
