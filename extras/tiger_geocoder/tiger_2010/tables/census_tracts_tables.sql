CREATE TABLE tract
(
  gid serial NOT NULL,
  statefp varchar(2),
  countyfp varchar(3),
  tractce varchar(6),
  geoid varchar(11) PRIMARY KEY,
  name character varying(7),
  namelsad character varying(20),
  mtfcc character varying(5),
  funcstat character varying(1),
  aland double precision,
  awater double precision,
  intptlat character varying(11),
  intptlon character varying(12),
  geom geometry,
  CONSTRAINT enforce_dims_geom CHECK (st_ndims(geom) = 2),
  CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geom) = 'MULTIPOLYGON'::text OR geom IS NULL),
  CONSTRAINT enforce_srid_geom CHECK (st_srid(geom) = 4269)
);
COMMENT ON TABLE tract IS 'census tracts';

CREATE TABLE tabblock
(
  gid serial NOT NULL,
  statefp varchar(2),
  countyfp character varying(3),
  tractce character varying(6),
  blockce character varying(4),
  geoid character varying(15) PRIMARY KEY,
  name character varying(10),
  mtfcc character varying(5),
  urcharacter varying(1),
  uace character varying(5),
  funcstat character varying(1),
  aland double precision,
  awater double precision,
  intptlat character varying(11),
  intptlon character varying(12),
  geom geometry,
  CONSTRAINT enforce_dims_geom CHECK (st_ndims(geom) = 2),
  CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geom) = 'MULTIPOLYGON'::text OR geom IS NULL),
  CONSTRAINT enforce_srid_geom CHECK (st_srid(geom) = 4269)
);
COMMENT ON TABLE tabblock IS 'census blocks';

CREATE TABLE bg
(
  gid serial NOT NULL,
  statefp character varying(2),
  countyfp character varying(3),
  tractce character varying(6),
  blkgrpce character varying(1),
  geoid character varying(12) PRIMARY KEY,
  namelsad character varying(13),
  mtfcc character varying(5),
  funcstat character varying(1),
  aland double precision,
  awater double precision,
  intptlat character varying(11),
  intptlon character varying(12),
  geom geometry,
  CONSTRAINT enforce_dims_geom CHECK (st_ndims(geom) = 2),
  CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geom) = 'MULTIPOLYGON'::text OR geom IS NULL),
  CONSTRAINT enforce_srid_geom CHECK (st_srid(geom) = 4269)
);
COMMENT ON TABLE bg IS 'block groups';