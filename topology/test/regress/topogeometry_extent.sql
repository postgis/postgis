set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology.sql
\i :regdir/../topology/test/load_features.sql
\i :regdir/../topology/test/more_features.sql
\i :regdir/../topology/test/hierarchy.sql

SELECT 'traffic_signs',
	topology.ST_Extent(feature)::text
FROM features.traffic_signs;

SELECT 'city_streets',
	topology.ST_Extent(feature)::text
FROM features.city_streets;

SELECT 'land_parcels',
	topology.ST_Extent(feature)::text
FROM features.land_parcels;

SELECT 'big_signs',
	topology.ST_Extent(feature)::text
FROM features.big_signs;

SELECT 'big_streets',
	topology.ST_Extent(feature)::text
FROM features.big_streets;

SELECT 'big_parcels',
	topology.ST_Extent(feature)::text
FROM features.big_parcels;

SELECT 'empty_set',
	topology.ST_Extent(feature) IS NULL
FROM features.big_parcels
WHERE false;

SELECT 'empty_topogeom',
	topology.ST_Extent(
		topology.CreateTopoGeom(
			'city_data',
			3,
			(SELECT layer_id FROM topology.layer WHERE table_name = 'land_parcels')
		)
	) IS NULL;

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
