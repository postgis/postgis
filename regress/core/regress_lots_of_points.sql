-- Generate 50000 deterministic pseudorandom points

CREATE TABLE "test" (
	"num" bigint,
	"the_geom" geometry
);

INSERT INTO "test" ("num", "the_geom")
  SELECT path[1], ST_AsText(geom, 6)
  FROM (
    SELECT (ST_DumpPoints(ST_GeneratePoints(ST_MakeEnvelope(0, 0, 1000, 1000), 50000, 654321))).*
  ) f;

