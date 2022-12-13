set client_min_messages to WARNING;

\i :top_builddir/topology/test/load_topology.sql
\i ../load_features.sql

ALTER TABLE features.land_parcels ADD geom geometry;
UPDATE features.land_parcels SET geom = feature::geometry;

ALTER TABLE features.city_streets ADD geom geometry;
UPDATE features.city_streets SET geom = feature::geometry;

ALTER TABLE features.traffic_signs ADD geom geometry;
UPDATE features.traffic_signs SET geom = feature::geometry;


CREATE FUNCTION features.check_changed_features()
RETURNS TABLE(typ text, nam text, exp text, obt text, symdiff text) AS
$$

  SELECT 'areal', feature_name,
    ST_AsText(geom),
    ST_AsText(feature::geometry),
    ST_AsText(ST_SymDifference(feature::geometry, geom))
    FROM features.land_parcels
    WHERE NOT ST_Equals(feature::geometry, geom)

  UNION ALL

  SELECT 'lineal', feature_name,
    ST_AsText(geom),
    ST_AsText(feature::geometry),
    ST_AsText(ST_SymDifference(feature::geometry, geom))
    FROM features.city_streets
    WHERE NOT ST_Equals(feature::geometry, geom)

  UNION ALL

  SELECT 'puntual', feature_name,
    ST_AsText(geom),
    ST_AsText(feature::geometry),
    ST_AsText(ST_SymDifference(feature::geometry, geom))
    FROM features.traffic_signs
    WHERE NOT ST_Equals(feature::geometry, geom);

$$ LANGUAGE 'sql';

-- Error calls

SELECT 'e1', topology.RemoveUnusedPrimitives('non-existent'::text);
SELECT 'e2', topology.RemoveUnusedPrimitives(NULL::text);

-- Valid calls

-- T0 -- bbox limited

--set client_min_messages to DEBUG;
SELECT 't0', 'clean', topology.RemoveUnusedPrimitives('city_data',
  ST_MakeEnvelope(8,5,10,23)
);
SELECT 't0', 'changed', * FROM features.check_changed_features();
SELECT 't0', 'invalidity', * FROM topology.ValidateTopology('city_data');

-- T1

--set client_min_messages to DEBUG;
SELECT 't1', 'clean', topology.RemoveUnusedPrimitives('city_data');
SELECT 't1', 'changed', * FROM features.check_changed_features();
SELECT 't1', 'invalidity', * FROM topology.ValidateTopology('city_data');


-- T2

WITH deleted AS (
  DELETE FROM features.city_streets WHERE feature_name IN ( 'R1' )
  RETURNING feature
), clear AS (
  SELECT clearTopoGeom(feature) FROM deleted
) SELECT NULL FROM clear;

--set client_min_messages to DEBUG;
SELECT 't2', 'clean', topology.RemoveUnusedPrimitives('city_data');
SELECT 't2', 'changed', * FROM features.check_changed_features();
SELECT 't2', 'invalidity', * FROM topology.ValidateTopology('city_data');

-- T3

WITH deleted AS (
  DELETE FROM features.traffic_signs WHERE feature_name IN ( 'S3' )
  RETURNING feature
), clear AS (
  SELECT clearTopoGeom(feature) FROM deleted
) SELECT NULL FROM clear;

--set client_min_messages to DEBUG;
SELECT 't3', 'clean', topology.RemoveUnusedPrimitives('city_data');
SELECT 't3', 'changed', * FROM features.check_changed_features();
SELECT 't3', 'invalidity', * FROM topology.ValidateTopology('city_data');

-- T4 -- test removal of isolated node

WITH deleted AS (
  DELETE FROM features.traffic_signs WHERE feature_name IN ( 'S4' )
  RETURNING feature
), clear AS (
  SELECT clearTopoGeom(feature) FROM deleted
) SELECT NULL FROM clear;

--set client_min_messages to DEBUG;
SELECT 't4', 'clean', topology.RemoveUnusedPrimitives('city_data');
SELECT 't4', 'changed', * FROM features.check_changed_features();
SELECT 't4', 'invalidity', * FROM topology.ValidateTopology('city_data');

-- T5 -- test removal of isolated edge leaving a used and an unused endpoint

-- Drop lineal feature R3 turning isolated edge (25) unused
-- Add a puntual feature TS1 using one (21) of the two unlinked
-- nodes (21 and 22)
WITH deleted AS (
  DELETE FROM features.city_streets WHERE feature_name IN ( 'R3' )
  RETURNING feature
),
start_point AS (
  SELECT ST_StartPoint(feature::geometry) sp, feature
  FROM deleted
),
clear AS (
  SELECT sp, clearTopoGeom(feature)
  FROM start_point
)
INSERT INTO features.traffic_signs ( feature_name, feature )
SELECT
  'TS1',
  toTopoGeom(
    sp,
    'city_data',
    layer_id(findLayer('features', 'traffic_signs', 'feature')),
    0
  )
FROM clear;
UPDATE features.traffic_signs SET geom = feature::geometry
WHERE feature_name = 'TS1';


--set client_min_messages to DEBUG;
SELECT 't5', 'clean', topology.RemoveUnusedPrimitives('city_data');
SELECT 't5', 'changed', * FROM features.check_changed_features();
SELECT 't5', 'invalidity', * FROM topology.ValidateTopology('city_data');

-- T6 -- test removal in presence of node incident to 2 closed edges
--       and 2 non-closed edges

-- Add closed edge starting and ending on the node used by S1

CREATE FUNCTION features.triangle(point geometry, offx float8, offy float8)
RETURNS geometry AS $xx$
  WITH point AS (
    SELECT ST_X($1) x, ST_Y($1) y
  )
  SELECT ST_MakeLine(p order by id) l FROM (
    SELECT
       s,
       ST_MakePoint(
        x + CASE WHEN s > 1 AND s < 4 THEN $2 ELSE 0 END,
        y + CASE WHEN s > 2 AND s < 4 THEN $3 ELSE 0 END
      )
    FROM
      point,
      generate_series(1,4) s
  ) foo(id,p)
$xx$ LANGUAGE 'sql';

WITH node AS (
  SELECT ST_GeometryN(feature::geometry, 1) g
  FROM features.traffic_signs WHERE feature_name = 'S1'
),
triangle_right AS (
  SELECT TopoGeo_addLineString( 'city_data',
    features.triangle(g, 4, 4)
  ) FROM node
),
triangle_left AS (
  SELECT TopoGeo_addLineString( 'city_data',
    features.triangle(g, -4, 4)
  ) FROM node
)
SELECT NULL FROM triangle_right UNION
SELECT NULL FROM triangle_left;


-- Drop puntual feature S1 turning node 14 unused
WITH deleted AS (
  DELETE FROM features.traffic_signs
  WHERE feature_name = 'S1'
  RETURNING feature
),
clear AS (
  SELECT clearTopoGeom(feature)
  FROM deleted
)
SELECT NULL FROM clear;

--set client_min_messages to DEBUG;
SELECT 't6', 'clean', topology.RemoveUnusedPrimitives('city_data');
SELECT 't6', 'changed', * FROM features.check_changed_features();
SELECT 't6', 'invalidity', * FROM topology.ValidateTopology('city_data');


-- See https://trac.osgeo.org/postgis/ticket/5289
SELECT NULL FROM (
  SELECT toTopoGeom(
    ST_MakeLine(
      ST_EndPoint( ST_GeometryN(feature,1) ),
      ST_StartPoint( ST_GeometryN(feature,1) )
    ),
    feature
  ) FROM features.city_streets WHERE feature_name = 'R2'
) foo;
UPDATE features.city_streets SET geom = feature::geometry
WHERE feature_name = 'R2';
SELECT '#5289', 'clean', topology.RemoveUnusedPrimitives('city_data');
SELECT '#5289', 'changed', * FROM features.check_changed_features();
SELECT '#5289', 'invalidity', * FROM topology.ValidateTopology('city_data');

--
-- See https://trac.osgeo.org/postgis/ticket/5303
--

-- Extend lineal feature R4 to include usage of edge 2
-- so that node X becomes "unused" (if it wasn't for
-- being required as endpoint of closed edge 2) while
-- edges themselves would still be both used by R4 feature.
UPDATE features.city_streets
SET feature = TopoGeom_addElement(feature, ARRAY[2, 2])
WHERE feature_name = 'R4';

SELECT '#5303', 'clean', topology.RemoveUnusedPrimitives('city_data');
SELECT '#5303', 'changed', * FROM features.check_changed_features();
SELECT '#5303', 'invalidity', * FROM topology.ValidateTopology('city_data');


--
-- Cleanup
--

SELECT NULL FROM DropTopology('city_data');
DROP SCHEMA features CASCADE;

