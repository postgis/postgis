set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology.sql
\i ../load_features.sql

SELECT 'street R1 signs', (tg).layer_id, (tg).id, (tg).type
FROM topology.GetRelatedTopoGeom('city_data', 3, 1, 2) AS tg
ORDER BY (tg).id;

SELECT 'street R2 signs', (tg).layer_id, (tg).id, (tg).type
FROM topology.GetRelatedTopoGeom('city_data', 3, 2, 2) AS tg
ORDER BY (tg).id;

SELECT 'street R4 signs count', count(*)
FROM topology.GetRelatedTopoGeom('city_data', 3, 4, 2) AS tg;

SELECT 'sign S1 streets', (tg).layer_id, (tg).id, (tg).type
FROM topology.GetRelatedTopoGeom(
  (SELECT feature FROM features.traffic_signs WHERE feature_name = 'S1'),
  3
) AS tg
ORDER BY (tg).id;

SELECT 'parcel P5 streets', (tg).layer_id, (tg).id, (tg).type
FROM topology.GetRelatedTopoGeom(
  (SELECT feature FROM features.land_parcels WHERE feature_name = 'P5'),
  3
) AS tg
ORDER BY (tg).id;

SELECT topology.DropTopology('city_data');
DROP SCHEMA features CASCADE;
