set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_large_topology.sql
\i ../load_large_features.sql

SELECT lid, tid, GetTopoGeomElements ('large_city_data', lid, tid)
FROM (
  SELECT DISTINCT layer_id as lid, topogeo_id as tid
  FROM large_city_data.relation
) as f
order by 1, 2, 3;

SELECT lid, tid, 'ARY', GetTopoGeomElementArray ('large_city_data', lid, tid)
FROM (
    SELECT DISTINCT layer_id as lid, topogeo_id as tid
    FROM large_city_data.relation
) as f
order by 1, 2;

-- See http://trac.osgeo.org/postgis/ticket/2060
SELECT 't2060', feature_name, GetTopoGeomElementArray (feature)
FROM large_features.land_parcels
ORDER BY feature_name;

-- clean up
SELECT topology.DropTopology ('large_city_data');
DROP SCHEMA large_features CASCADE;
