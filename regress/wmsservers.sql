SELECT 'Starting up MapServer/Geoserver tests...';
-- Set up the data table
SELECT 'Setting up the data table...';
create table wmstest as select lon * 100 + lat as id, st_setsrid(st_buffer(st_makepoint(lon, lat),1.0),4326) as pt
from (select lon, generate_series(-80,80, 5) as lat from (select generate_series(-175, 175, 5) as lon) as sq1) as sq2;
insert into geometry_columns (f_table_catalog, f_table_schema, f_table_name, f_geometry_column, coord_dimension, srid, type) values ('', 'public','wmstest','pt',2,4326,'POLYGON');
alter table wmstest add primary key ( id );
create index wmstest_geomidx on wmstest using gist ( pt );

-- Geoserver 2.0 NG tests
SELECT 'Running Geoserver 2.0 NG tests...';
-- Run a Geoserver 2.0 NG metadata query
SELECT TYPE FROM GEOMETRY_COLUMNS WHERE F_TABLE_SCHEMA = 'public' AND F_TABLE_NAME = 'wmstest' AND F_GEOMETRY_COLUMN = 'pt';
SELECT SRID FROM GEOMETRY_COLUMNS WHERE F_TABLE_SCHEMA = 'public' AND F_TABLE_NAME = 'wmstest' AND F_GEOMETRY_COLUMN = 'pt';
-- Run a Geoserver 2.0 NG WMS query
SELECT "id",encode(asBinary(force_2d("pt"),'XDR'),'base64') as "pt" FROM "public"."wmstest" WHERE "pt" && GeomFromText('POLYGON ((-6.58216065979069 -0.7685569763184591, -6.58216065979069 0.911225433349509, -3.050569931030911 0.911225433349509, -3.050569931030911 -0.7685569763184591, -6.58216065979069 -0.7685569763184591))', 4326);
-- Run a Geoserver 2.0 NG KML query
SELECT count(*) FROM "public"."wmstest" WHERE "pt" && GeomFromText('POLYGON ((-1.504017942347938 24.0332272532341, -1.504017942347938 25.99364254836741, 1.736833353559741 25.99364254836741, 1.736833353559741 24.0332272532341, -1.504017942347938 24.0332272532341))', 4326);
SELECT "id",encode(asBinary(force_2d("pt"),'XDR'),'base64') as "pt" FROM "public"."wmstest" WHERE "pt" && GeomFromText('POLYGON ((-1.504017942347938 24.0332272532341, -1.504017942347938 25.99364254836741, 1.736833353559741 25.99364254836741, 1.736833353559741 24.0332272532341, -1.504017942347938 24.0332272532341))', 4326);
SELECT "id",encode(asBinary(force_2d("pt"),'XDR'),'base64') as "pt" FROM "public"."wmstest" WHERE "pt" && GeomFromText('POLYGON ((-1.507182836191598 24.031312785172446, -1.507182836191598 25.995557016429064, 1.7399982474034008 25.995557016429064, 1.7399982474034008 24.031312785172446, -1.507182836191598 24.031312785172446))', 4326);

-- MapServer 5.4 tests
select attname from pg_attribute, pg_constraint, pg_class where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = 'wmstest' and pg_table_is_visible(pg_class.oid) and pg_constraint.conkey[2] is null;
select "id",encode(AsBinary(force_collection(force_2d("pt")),'NDR'),'base64') as geom,"id" from wmstest where pt && GeomFromText('POLYGON((-98.5 32,-98.5 39,-91.5 39,-91.5 32,-98.5 32))',find_srid('','wmstest','pt'));

-- MapServer 5.6 tests
select * from wmstest where false limit 0;
select attname from pg_attribute, pg_constraint, pg_class where pg_constraint.conrelid = pg_class.oid and pg_class.oid = pg_attribute.attrelid and pg_constraint.contype = 'p' and pg_constraint.conkey[1] = pg_attribute.attnum and pg_class.relname = 'wmstest' and pg_table_is_visible(pg_class.oid) and pg_constraint.conkey[2] is null;
select "id",encode(AsBinary(force_collection(force_2d("pt")),'NDR'),'hex') as geom,"id" from wmstest where pt && GeomFromText('POLYGON((-98.5 32,-98.5 39,-91.5 39,-91.5 32,-98.5 32))',find_srid('','wmstest','pt'));

-- Drop the data table
SELECT 'Removing the data table...';
drop table wmstest;
delete from geometry_columns where f_table_name = 'wmstest' and f_table_schema = 'public';
SELECT 'Done.';


