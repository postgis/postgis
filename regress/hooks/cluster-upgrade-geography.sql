CREATE EXTENSION postgis;

CREATE SCHEMA procsch;

CREATE TABLE procsch."MyTab" (
    "id" bigint,
    "loc" public.geography(Point, 4283)
);

INSERT INTO procsch."MyTab"
VALUES (1, 'SRID=4283;POINT(152.138672 -30.689888)'::geography);

SELECT count(*) FROM procsch."MyTab";
