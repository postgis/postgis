set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology.sql
\i :regdir/../topology/test/load_features.sql
\i :regdir/../topology/test/more_features.sql
\i :regdir/../topology/test/hierarchy.sql

SELECT DISTINCT 'ST_Srid(traffic_signs)',
	st_srid(feature) FROM features.traffic_signs;

SELECT DISTINCT 'ST_Srid(city_streets)',
	st_srid(feature) FROM features.city_streets;

SELECT DISTINCT 'ST_Srid(land_parcels)',
	st_srid(feature) FROM features.land_parcels;

SELECT DISTINCT 'ST_Srid(big_signs)',
	st_srid(feature) FROM features.big_signs;

SELECT DISTINCT 'ST_Srid(big_streets)',
	st_srid(feature) FROM features.big_streets;

SELECT DISTINCT 'ST_Srid(big_parcels)',
	st_srid(feature) FROM features.big_parcels;

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
