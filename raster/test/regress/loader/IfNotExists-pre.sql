DROP TABLE IF EXISTS loadedrast;
CREATE TABLE loadedrast ("rid" serial PRIMARY KEY, rast raster);
CREATE INDEX loadedrast_rast_gist ON loadedrast USING gist (st_convexhull(rast));
