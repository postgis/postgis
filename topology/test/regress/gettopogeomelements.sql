set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology.sql
\i ../load_features.sql

SELECT lid, tid, GetTopoGeomElements('city_data', lid, tid)
FROM (
 SELECT DISTINCT layer_id as lid, topogeo_id as tid
 FROM city_data.relation
) as f
order by 1, 2, 3;

SELECT lid, tid, 'ARY', GetTopoGeomElementArray('city_data', lid, tid)
FROM (
 SELECT DISTINCT layer_id as lid, topogeo_id as tid
 FROM city_data.relation
) as f
order by 1, 2;

-- See http://trac.osgeo.org/postgis/ticket/4882
SELECT 't4882-set', feature_name, elem
FROM features.city_streets,
LATERAL GetTopoGeomElements(feature, true) AS signed_elements(elem)
WHERE feature_name IN ('R1', 'R2')
ORDER BY feature_name, abs((elem)[1]);

SELECT 't4882-array', feature_name, GetTopoGeomElementArray(feature, true)
FROM features.city_streets
WHERE feature_name IN ('R1', 'R2')
ORDER BY feature_name;

SELECT 't4882-raw-set', lid, tid, elem
FROM (
 SELECT DISTINCT layer_id as lid, topogeo_id as tid
 FROM city_data.relation
 WHERE layer_id = 3 AND topogeo_id IN (1, 2)
) as f,
LATERAL GetTopoGeomElements('city_data', lid, tid, true) AS signed_elements(elem)
ORDER BY 2, 3, abs((elem)[1]);

SELECT 't4882-raw-array', lid, tid, GetTopoGeomElementArray('city_data', lid, tid, true)
FROM (
 SELECT DISTINCT layer_id as lid, topogeo_id as tid
 FROM city_data.relation
 WHERE layer_id = 3 AND topogeo_id IN (1, 2)
) as f
ORDER BY 2, 3;

-- See http://trac.osgeo.org/postgis/ticket/2060
SELECT 't2060', feature_name, GetTopoGeomElementArray(feature)
FROM features.land_parcels
ORDER BY feature_name;

-- clean up
SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
