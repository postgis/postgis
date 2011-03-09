set client_min_messages to WARNING;

SELECT topology.CreateTopology('MiX') > 0;

-- Fails due to missing layer 1
SELECT topology.CreateTopoGeom(
    'MiX', -- Topology name
    3, -- Topology geometry type (polygon/multipolygon)
    1, -- TG_LAYER_ID for this topology (from topology.layer)
    '{{3,3},{6,3}}'); -- face_id:3 face_id:6

CREATE TABLE "MiX".poi (id int);
SELECT topology.AddTopoGeometryColumn('MiX', 'MiX', 'poi', 'feat', 'POINT');

-- A Layer of type 1 (POINT) cannot contain a TopoGeometry of type 2 (LINE)
SELECT topology.CreateTopoGeom( 'MiX', 2, 1, '{{12,2}}'); 
-- A Layer of type 1 (POINT) cannot contain a TopoGeometry of type 3 (POLY)
SELECT topology.CreateTopoGeom( 'MiX', 3, 1, '{{13,3}}'); 
-- A Layer of type 1 (POINT) cannot contain a TopoGeometry of type 4 (COLL.)
SELECT topology.CreateTopoGeom( 'MiX', 4, 1, '{{12,2}}'); 

-- Node 78 does not exist in topology MiX (trigger on "relation" table)
SELECT topology.CreateTopoGeom( 'MiX', 1, 1, '{{78,1}}'); 

SELECT 'n1',  topology.addNode('MiX', 'POINT(0 0)');

-- Success !
SELECT layer_id(tg), id(tg), type(tg) FROM (
 SELECT topology.CreateTopoGeom( 'MiX', 1, 1, '{{1,1}}') as tg
) foo; 

SELECT topology.DropTopology('MiX');
