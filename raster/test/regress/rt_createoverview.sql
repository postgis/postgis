SET client_min_messages TO warning;
-- Test for table without explicit schema
CREATE TABLE res1 AS SELECT
  ST_AddBand(
    ST_MakeEmptyRaster(10, 10, x, y, 1, -1, 0, 0, 0)
    , 1, '8BUI', 0, 0
  ) r
FROM generate_series(-170, 160, 10) x,
     generate_series(80, -70, -10) y;
SELECT addrasterconstraints('res1', 'r');

SELECT ST_CreateOverview('res1', 'r', 2)::text = 'o_2_res1';

SELECT ST_CreateOverview('res1', 'r', 4)::text = 'o_4_res1';

SELECT ST_CreateOverview('res1', 'r', 8)::text = 'o_8_res1';

SELECT ST_CreateOverview('res1', 'r', 16)::text = 'o_16_res1';

SELECT r_table_name tab, r_raster_column c, srid s,
 scale_x sx, scale_y sy,
 blocksize_x w, blocksize_y h, same_alignment a,
 -- regular_blocking (why not regular?)
 --extent::box2d e,
 st_covers(extent::box2d, 'BOX(-170 -80,170 80)'::box2d) ec,
 st_xmin(extent::box2d) = -170 as eix,
 st_ymax(extent::box2d) = 80 as eiy,
 (st_xmax(extent::box2d) - 170) <= scale_x as eox,
 --(st_xmax(extent::box2d) - 170) eoxd,
 abs(st_ymin(extent::box2d) + 80) <= abs(scale_y) as eoy
 --,abs(st_ymin(extent::box2d) + 80) eoyd
 FROM raster_columns
WHERE r_table_name like '%res1'
ORDER BY scale_x, r_table_name;

SELECT o_table_name, o_raster_column,
       r_table_name, r_raster_column, overview_factor
FROM raster_overviews
WHERE r_table_name = 'res1'
ORDER BY overview_factor;

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
-- offset the schema tableto distinguish it from original
-- let it be small to reduce time cost.
CREATE TABLE oschm.res1 AS SELECT
  ST_AddBand(
    ST_MakeEmptyRaster(10, 10, x, y, 1, -1, 0, 0, 0)
    , 1, '8BUI', 0, 0
  ) r
FROM generate_series(100, 270, 10) x,
     generate_series(140, -30, -10) y;
SELECT addrasterconstraints('oschm'::name,'res1'::name, 'r'::name);

-- add overview with explicit schema
SELECT ST_CreateOverview('oschm.res1', 'r', 8)::text = 'oschm.o_8_res1';

-- set search path to 'oschm' first
SET search_path to oschm,public;

-- create overview with schema in search_path
SELECT ST_CreateOverview('res1', 'r', 4)::text = 'o_4_res1';

-- Create overview for table in public schema with explict path
-- at same factor of schema table
SELECT ST_CreateOverview('public.res1', 'r', 8)::text = 'public.o_8_res1';

-- Reset the search_path
SET search_path to public;

-- Create public overview of public table with same name
-- and scale as schema table above using reset search path.
SELECT ST_CreateOverview('res1', 'r', 4)::text = 'o_4_res1';

-- Check scale and extent
-- Offset means that original raster overviews won't match
-- extent and values only mach on schema table not public one
SELECT r_table_schema, r_table_name tab, r_raster_column c,
 srid s, scale_x sx, scale_y sy,
 blocksize_x w, blocksize_y h, same_alignment a,
 -- regular_blocking (why not regular?)
 --extent::box2d e,
 st_covers(extent::box2d, 'BOX(100 -30,270 140)'::box2d) ec,
 st_xmin(extent::box2d) = 100 as eix,
 st_ymax(extent::box2d) = 140 as eiy,
 (st_xmax(extent::box2d) - 280) <= scale_x as eox,
 --(st_xmax(extent::box2d) - 170) eoxd,
 abs(st_ymin(extent::box2d) + 40) <= abs(scale_y) as eoy
 --,abs(st_ymin(extent::box2d) + 80) eoyd
 FROM raster_columns
WHERE r_table_name like '%res1'
ORDER BY r_table_schema, scale_x, r_table_name;

-- this test may be redundant - it's just confirming that the
-- overview factor is correct, which we do for the original table.
SELECT o_table_schema, o_table_name, o_raster_column,
       r_table_schema, r_table_name, r_raster_column, overview_factor
FROM raster_overviews
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

-- Reset the session environment
-- possibly a bit harsh, but we had to set the search_path
-- and need to reset it back to default.
DISCARD ALL;
