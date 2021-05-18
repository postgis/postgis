CREATE TABLE points AS SELECT * FROM (VALUES ('SRID=4326;POINT(-124.9921 49.6851)'::geometry), ('SRID=4326;POINT(-119.4032 50.0305)'::geometry), ('SRID=4326;POINT(-122.799 49.1671)'::geometry), ('SRID=4326;POINT(-122.3379 49.0597)'::geometry), ('SRID=4326;POINT(-123.1264 49.2671)'::geometry), ('SRID=4326;POINT(-122.7132 49.0519)'::geometry), ('SRID=4326;POINT(-124.3475 49.3042)'::geometry), ('SRID=4326;POINT(-119.389 49.8891)'::geometry), ('SRID=4326;POINT(-123.126 49.281)'::geometry), ('SRID=4326;POINT(-122.6606 49.1134)'::geometry), ('SRID=4326;POINT(-124.3233 49.312)'::geometry), ('SRID=4326;POINT(-124.0478 49.2397)'::geometry), ('SRID=4326;POINT(-119.2683 50.266)'::geometry), ('SRID=4326;POINT(-121.9705 49.081)'::geometry), ('SRID=4326;POINT(-123.8854 49.482)'::geometry), ('SRID=4326;POINT(-123.1528 49.77)'::geometry), ('SRID=4326;POINT(-120.8051 50.488)'::geometry), ('SRID=4326;POINT(-122.6403 49.1652)'::geometry), ('SRID=4326;POINT(-122.7717 49.2433)'::geometry), ('SRID=4326;POINT(-121.9587 49.1661)'::geometry))
p(g);

CREATE OR REPLACE FUNCTION get_closest(p geometry(POINT)) RETURNS geometry(POINT) AS $$
    SELECT g FROM points ORDER BY st_transform(p, 3005) <-> st_transform(points.g, 3005) LIMIT 1
$$
STABLE
LANGUAGE SQL;

SELECT 4890 AS id, ST_AsText(ST_SnapToGrid(ST_Centroid(ST_Collect(get_closest(g) )), 0.0001)) FROM points ;
DROP TABLE points;
DROP FUNCTION get_closest(geometry);
