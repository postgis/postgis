-- Test index creation on null geometries
-- An index on more than 459 null geometries (with or without actual geometires)
--   used to fail on PostgreSQL 8.2.

CREATE TABLE "test" (
	"num" integer,
	"the_geom" geometry
);

INSERT INTO "test" ("num", "the_geom")
  SELECT i, NULL
  FROM generate_series(1, 500) i;

