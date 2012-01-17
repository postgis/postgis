SET client_min_messages TO warning;
CREATE SCHEMA tm;

CREATE TABLE tm.geoms (id serial, g geometry);

INSERT INTO tm.geoms(g) values ('POINT EMPTY');
INSERT INTO tm.geoms(g) values ('LINESTRING EMPTY');
INSERT INTO tm.geoms(g) values ('POLYGON EMPTY');
INSERT INTO tm.geoms(g) values ('MULTIPOINT EMPTY');
INSERT INTO tm.geoms(g) values ('MULTILINESTRING EMPTY');
INSERT INTO tm.geoms(g) values ('MULTIPOLYGON EMPTY');
INSERT INTO tm.geoms(g) values ('GEOMETRYCOLLECTION EMPTY');
INSERT INTO tm.geoms(g) values ('CIRCULARSTRING EMPTY');
INSERT INTO tm.geoms(g) values ('COMPOUNDCURVE EMPTY');
INSERT INTO tm.geoms(g) values ('CURVEPOLYGON EMPTY');
INSERT INTO tm.geoms(g) values ('MULTICURVE EMPTY');
INSERT INTO tm.geoms(g) values ('MULTISURFACE EMPTY');
INSERT INTO tm.geoms(g) values ('POLYHEDRALSURFACE EMPTY');
INSERT INTO tm.geoms(g) values ('TRIANGLE EMPTY');
INSERT INTO tm.geoms(g) values ('TIN EMPTY');

-- all zm flags
INSERT INTO tm.geoms(g)
SELECT st_force_3dz(g) FROM tm.geoms WHERE id < 15 ORDER BY id;
INSERT INTO tm.geoms(g)
SELECT st_force_3dm(g) FROM tm.geoms WHERE id < 15 ORDER BY id;
INSERT INTO tm.geoms(g)
SELECT st_force_4d(g) FROM tm.geoms WHERE id < 15 ORDER BY id;

-- known srid
INSERT INTO tm.geoms(g)
SELECT st_setsrid(g,4326) FROM tm.geoms ORDER BY id;

COPY ( SELECT g FROM tm.geoms ORDER BY id ) TO STDOUT WITH BINARY;

CREATE TABLE tm.geogs AS SELECT id,g::geography FROM tm.geoms
WHERE geometrytype(g) NOT LIKE '%CURVE%'
  AND geometrytype(g) NOT LIKE '%CIRCULAR%'
  AND geometrytype(g) NOT LIKE '%SURFACE%'
  AND geometrytype(g) NOT LIKE 'TRIANGLE%'
  AND geometrytype(g) NOT LIKE 'TIN%'
;

COPY ( SELECT g FROM tm.geogs ORDER BY id ) TO STDOUT WITH BINARY;

DROP SCHEMA tm CASCADE;
