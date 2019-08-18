insert into spatial_ref_sys(srid,srtext) values (1,'fake["srs"],text');
CREATE TABLE c ( id integer NOT NULL, geom geometry(Point, 1));
INSERT INTO c VALUES(1, NULL);
INSERT INTO c VALUES(2, NULL);
INSERT INTO c VALUES(3, 'SRID=1;POINT(1 1)');
INSERT INTO c VALUES(4, NULL);
