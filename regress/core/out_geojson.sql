DROP TABLE IF EXISTS g;
CREATE TABLE g (
	g geometry,
	i integer,
	f real,
	t text,
	d date
);

INSERT INTO g (g, i, f, t, d)
VALUES ('POINT(42 42)', 1, 1.1, 'one', '2001-01-01');

INSERT INTO g (g, i, f, t, d)
VALUES ('LINESTRING(42 42, 45 45)', 2, 2.2, 'two', '2002-02-02');

INSERT INTO g (g, i, f, t, d)
VALUES ('POLYGON((42 42, 45 45, 45 42, 42 42))', 3, 3.3, 'three', '2003-03-03');

INSERT INTO g (g, i, f, t, d)
VALUES ('GEOMETRYCOLLECTION(POINT(42 42))', 4, 4.4, 'four', '2004-04-04');

INSERT INTO g VALUES ('POINT EMPTY', 5, 5.5, 'five', '2005-05-05');
INSERT INTO g VALUES (NULL, 6, 6.6, 'six', '2006-06-06');
INSERT INTO g VALUES ('GEOMETRYCOLLECTION(POINT EMPTY, POINT(1 2))', 7, 7.7, 'seven', '2007-07-07');

SELECT 'gj01', i, ST_AsGeoJSON(g) AS g1
	FROM g ORDER BY i;

SELECT 'gj02', i, ST_AsGeoJSON(g.*) AS gj2
	FROM g ORDER BY i;

SELECT 'gj03', i, to_json(g.*) AS rj3
	FROM g ORDER BY i;

SELECT 'gj04', i, to_jsonb(g.*) AS rj4
	FROM g ORDER BY i;

SELECT 'gj05', i, ST_AsGeoJSON(g.*, id_column => 'i') AS gj5
	FROM g ORDER BY i;

SELECT '4695', ST_ASGeoJSON(a.*) FROM
(
    SELECT 1 as v, ST_SetSRID(ST_Point(0,1),2227) as g
) a;

DROP TABLE g;
SELECT 'gj_named_bigint', ST_AsGeoJSON(q, 'geom') FROM (SELECT 1::bigint AS geom) AS q;
SELECT 'gj_named_null_text', ST_AsGeoJSON(q, 'geom') FROM (SELECT NULL::text AS geom) AS q;
SELECT ST_AsGeoJSON('POINT(1 2)'::geometry, 1000000000, 1);
SELECT ST_AsGeoJSON('LINESTRING(1.5 -0.0000001, 10000000000000000 2)'::geometry, 7, 1);
SELECT ST_AsGeoJSON('LINESTRING Z (1.5 -0.0000001 3.125, 10000000000000000 2 4)'::geometry, 7, 1);
SELECT case_name, ST_AsGeoJSON(q, id_column => cases.id_column)
FROM (SELECT 1 AS id, 'POINT(1 2)'::geometry AS geom) AS q
CROSS JOIN (VALUES ('gj_id_empty', ''), ('gj_id_named', 'id'), ('gj_id_null', NULL))
    AS cases(case_name, id_column)
ORDER BY case_name;

-- Installing the legacy C binding requires a superuser.
DO $$
BEGIN
  IF (SELECT rolsuper FROM pg_roles WHERE rolname = current_user) THEN
    EXECUTE format(
      'CREATE FUNCTION pg_temp.st_asgeojson_legacy(record, text, integer, boolean)
       RETURNS text AS %L, %L LANGUAGE C STRICT',
      (SELECT probin FROM pg_proc WHERE oid = 'st_asgeojson(record,text,integer,boolean,text)'::regprocedure),
      'ST_AsGeoJsonRow');

    IF EXISTS (
      SELECT FROM (SELECT 1 AS id, ST_Point(1, 2) AS geom, ST_Point(3, 4) AS other_geom) AS q
      CROSS JOIN (VALUES (''), ('other_geom')) AS cases(geom_column)
      WHERE pg_temp.st_asgeojson_legacy(q, geom_column, 9, false)
        IS DISTINCT FROM ST_AsGeoJSON(q, geom_column, 9, false, '')
    ) THEN
      RAISE EXCEPTION 'Legacy ST_AsGeoJSON must match the current signature without an ID column';
    END IF;
    DROP FUNCTION pg_temp.st_asgeojson_legacy(record, text, integer, boolean);
  END IF;
END
$$;
