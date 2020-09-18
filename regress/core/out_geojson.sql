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

SELECT '4695', ST_ASGeoJSON(a.*) FROM
(
    SELECT 1 as v, ST_SetSRID(ST_Point(0,1),2227) as g
) a;

DROP TABLE g;
