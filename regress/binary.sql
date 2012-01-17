SET client_min_messages TO warning;
CREATE SCHEMA tm;

CREATE TABLE tm.types (id serial, g geometry);

INSERT INTO tm.types(g) values ('POINT EMPTY');
INSERT INTO tm.types(g) values ('LINESTRING EMPTY');
INSERT INTO tm.types(g) values ('POLYGON EMPTY');
INSERT INTO tm.types(g) values ('MULTIPOINT EMPTY');
INSERT INTO tm.types(g) values ('MULTILINESTRING EMPTY');
INSERT INTO tm.types(g) values ('MULTIPOLYGON EMPTY');
INSERT INTO tm.types(g) values ('GEOMETRYCOLLECTION EMPTY');
INSERT INTO tm.types(g) values ('CIRCULARSTRING EMPTY');
INSERT INTO tm.types(g) values ('COMPOUNDCURVE EMPTY');
INSERT INTO tm.types(g) values ('CURVEPOLYGON EMPTY');
INSERT INTO tm.types(g) values ('MULTICURVE EMPTY');
INSERT INTO tm.types(g) values ('MULTISURFACE EMPTY');
INSERT INTO tm.types(g) values ('POLYHEDRALSURFACE EMPTY');
INSERT INTO tm.types(g) values ('TRIANGLE EMPTY');
INSERT INTO tm.types(g) values ('TIN EMPTY');

-- all zm flags
INSERT INTO tm.types(g)
SELECT st_force_3dz(g) FROM tm.types WHERE id < 15 ORDER BY id;
INSERT INTO tm.types(g)
SELECT st_force_3dm(g) FROM tm.types WHERE id < 15 ORDER BY id;
INSERT INTO tm.types(g)
SELECT st_force_4d(g) FROM tm.types WHERE id < 15 ORDER BY id;

-- known srid
INSERT INTO tm.types(g)
SELECT st_setsrid(g,1) FROM tm.types ORDER BY id;

COPY ( SELECT g FROM tm.types ORDER BY id ) TO STDOUT WITH BINARY;

DROP SCHEMA tm CASCADE;
