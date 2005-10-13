-- This script adds a the_geom column to the feature tables
-- created by load_topology.sql and stores there the SFS Geometry
-- derived by the TopoGeometry column

ALTER TABLE features.city_streets ADD the_geom geometry;
UPDATE features.city_streets set the_geom = topology.Geometry(feature); 

ALTER TABLE features.traffic_signs ADD the_geom geometry;
UPDATE features.traffic_signs set the_geom = topology.Geometry(feature); 

ALTER TABLE features.land_parcels ADD the_geom geometry;
UPDATE features.land_parcels set the_geom = topology.Geometry(feature); 

ALTER TABLE features.big_parcels ADD the_geom geometry;
UPDATE features.big_parcels set the_geom = topology.Geometry(feature); 

ALTER TABLE features.big_signs ADD the_geom geometry;
UPDATE features.big_signs set the_geom = topology.Geometry(feature); 
