--
-- Define some hierarchical layers
--

--
-- Parcels
--

CREATE TABLE features.big_parcels (
	feature_name varchar primary key
);

SELECT topology.AddTopoGeometryColumn('city_data', 'features',
	'big_parcels', 'feature', 'POLYGON',
	1 -- the land_parcles
);

SELECT AddGeometryColumn('features','big_parcels','the_geom',-1,'MULTIPOLYGON',2);

INSERT INTO features.big_parcels VALUES ('P1P2', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_parcels'),
    '{{1,1},{2,1}}')); -- P1 and P2

INSERT INTO features.big_parcels VALUES ('P3P4', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_parcels'),
    '{{3,1},{4,1}}')); -- P3 and P4

INSERT INTO features.big_parcels VALUES ('F3F6', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_parcels'),
    '{{6,1},{7,1}}')); -- F3 and F6

SELECT feature_name,ST_AsText(topology.geometry(feature)) from features.big_parcels;

SELECT a.feature_name, b.feature_name
	FROM features.land_parcels a, features.big_parcels b
	WHERE topology.equals(a.feature, b.feature);

--
-- Signs
--

CREATE TABLE features.big_signs (
	feature_name varchar primary key
);

SELECT topology.AddTopoGeometryColumn('city_data', 'features',
	'big_signs', 'feature', 'POINT',
	2 -- the traffic_signs
);

SELECT AddGeometryColumn('features','big_signs','the_geom',-1,'MULTIPOINT',2);

INSERT INTO features.big_signs VALUES ('S1S2', -- Feature name
  topology.CreateTopoGeom(
    'city_data', -- Topology name
    1, -- Topology geometry type (point/multipoint)
    (SELECT layer_id FROM topology.layer WHERE table_name = 'big_signs'),
    '{{1,2},{2,2}}')); -- S1 and S2

SELECT feature_name, ST_AsText(topology.geometry(feature)) from features.big_signs;

SELECT a.feature_name, b.feature_name
	FROM features.traffic_signs a, features.big_signs b
	WHERE topology.equals(a.feature, b.feature);
