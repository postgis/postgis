\set VERBOSITY terse
set client_min_messages to WARNING;

select NULL FROM createtopology('tt', 4326);

-- layer 1 is PUNTUAL
CREATE TABLE tt.f_point(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_point', 'g', 'POINT');

-- layer 2 is HIERARCHICAL PUNTUAL
CREATE TABLE tt.f_hier_point(lbl text primary key);
SELECT NULL FROM AddTopoGeometryColumn('tt', 'tt', 'f_hier_point', 'g', 'POINT', 1);

-- populate base
INSERT INTO tt.f_point(lbl,g)
SELECT x::text||y::text, ToTopoGeom( ST_Point(x,y,4326), 'tt', l.layer_id )
FROM generate_series(50,60, 10) AS x CROSS JOIN generate_series(10,90, 20) AS y
    CROSS JOIN (SELECT l.layer_id
                    FROM topology.layer AS l WHERE l.table_name = 'f_point' AND l.schema_name = 'tt' AND l.feature_column = 'g'
               ) AS l;

-- populate hierarchy, cast
INSERT INTO  tt.f_hier_point(lbl, g)
SELECT left(f.lbl,2) AS lbl, CreateTopoGeom('tt',1, l.layer_id, TopoElementArray_agg(f.g::topology.topoelement ORDER BY f.lbl) )
FROM  tt.f_point AS f CROSS JOIN
    (SELECT l.layer_id FROM topology.layer AS l WHERE l.table_name = 'f_hier_point' AND l.schema_name = 'tt' AND l.feature_column = 'g' ) AS l
GROUP BY l.layer_id, left(f.lbl,2);

SELECT ST_AsText(g::geometry)
FROM tt.f_hier_point;

-- using FUNCTION

-- populate hierarchy
INSERT INTO  tt.f_hier_point(lbl, g)
SELECT 'all' AS lbl, CreateTopoGeom('tt',1, l.layer_id, TopoElementArray_agg(topology.topoelement(f.g) ORDER BY f.lbl) )
FROM  tt.f_point AS f CROSS JOIN
    (SELECT l.layer_id FROM topology.layer AS l WHERE l.table_name = 'f_hier_point' AND l.schema_name = 'tt' AND l.feature_column = 'g' ) AS l
GROUP BY l.layer_id;

SELECT ST_AsText(g::geometry)
FROM tt.f_hier_point WHERE lbl = 'all';

-- Cleanup
SELECT NULL FROM DropTopology('tt');
