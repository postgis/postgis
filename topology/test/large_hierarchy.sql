--
-- Define some hierarchical layers
--

--
-- Parcels
--

CREATE TABLE large_features.big_parcels (
	feature_name varchar primary key, fid bigserial
) ;

SELECT NULL FROM topology.AddTopoGeometryColumn('large_city_data', 'large_features.big_parcels',
  'feature', 2001, 'POLYGON',
	1101 -- the land_parcles
);

SELECT NULL FROM AddGeometryColumn('large_features','big_parcels','the_geom',-1,'MULTIPOLYGON',2);

INSERT INTO large_features.big_parcels(feature_name, feature) VALUES ('P1P2', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_parcels'),
    '{{1101000000001,1101},{1101000000002,1101}}')); -- P1 and P2

INSERT INTO large_features.big_parcels(feature_name, feature) VALUES ('P3P4', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_parcels'),
    '{{1101000000003,1101},{1101000000004,1101}}')); -- P3 and P4

INSERT INTO large_features.big_parcels(feature_name, feature) VALUES ('F3F6', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_parcels'),
    (SELECT topoelementarray_agg(ARRAY[id(feature), 1101])
     FROM large_features.land_parcels
     WHERE feature_name in ('F3','F6'))
  ));

--
-- Streets
--

CREATE TABLE large_features.big_streets (
	feature_name varchar primary key, fid bigserial
) ;

SELECT NULL FROM topology.AddTopoGeometryColumn('large_city_data', 'large_features.big_streets',
  'feature', 2003, 'LINE',
	1103 -- the city_streets layer id
);

INSERT INTO large_features.big_streets(feature_name, feature)VALUES ('R1R2', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    2, -- Topology geometry type (lineal)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_streets'),
    (SELECT topoelementarray_agg(ARRAY[id(feature), 1103])
     FROM large_features.city_streets
     WHERE feature_name in ('R1','R2')) -- R1 and R2
  ));

INSERT INTO large_features.big_streets(feature_name, feature) VALUES ('R4', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    2, -- Topology geometry type (lineal)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_streets'),
    (SELECT topoelementarray_agg(ARRAY[id(feature), 1103])
     FROM large_features.city_streets
     WHERE feature_name in ('R4'))
  ));

--
-- Signs
--

CREATE TABLE large_features.big_signs (
	feature_name varchar primary key, fid bigserial
) ;

SELECT NULL FROM topology.AddTopoGeometryColumn('large_city_data', 'large_features.big_signs',
  'feature', 2002, 'POINT',
	1102 -- the traffic_signs
);

SELECT NULL FROM AddGeometryColumn('large_features','big_signs','the_geom',0,'MULTIPOINT',2);

INSERT INTO large_features.big_signs(feature_name, feature) VALUES ('S1S2', -- Feature name
  topology.CreateTopoGeom(
    'large_city_data', -- Topology name
    1, -- Topology geometry type (point/multipoint)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_signs'),
    '{{1102000000002,1102},{1102000000003,1102}}')); -- S1 and S2

