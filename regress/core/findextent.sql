DROP SCHEMA IF EXISTS "schema""quote" CASCADE;
CREATE SCHEMA "schema""quote";
CREATE TABLE "schema""quote"."table""quote" ("geom""column" geometry);
INSERT INTO "schema""quote"."table""quote" VALUES ('POINT(1 2)'), ('POINT(3 4)');
SELECT ST_FindExtent('schema"quote', 'table"quote', 'geom"column')::text;
SET search_path = "schema""quote", public;
SELECT public.ST_FindExtent('table"quote', 'geom"column')::text;
DROP SCHEMA "schema""quote" CASCADE;
