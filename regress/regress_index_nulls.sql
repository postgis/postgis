\i regress_lots_of_nulls.sql


CREATE INDEX "test_geom_idx" ON "test" using gist (the_geom);

DROP TABLE "test";
