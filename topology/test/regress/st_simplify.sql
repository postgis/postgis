set client_min_messages to WARNING;

SELECT CreateTopology('tt') > 1;
CREATE TABLE tt.areas(id serial, g geometry);
INSERT INTO tt.areas(g) VALUES ('POLYGON((0 0,1 1,1 3,0 4,-2 3,-1 1,0 0))'),
                               ('POLYGON((0 0,1 1,1 3,2 3,2 0,0 0))');
SELECT 'L' || AddTopoGeometryColumn('tt', 'tt', 'areas', 'tg', 'polygon');
UPDATE tt.areas SET tg = toTopoGeom(g, 'tt', 1);

-- ensures this point won't be removed
SELECT 'N' || TopoGeo_addPoint('tt', 'POINT(1 3)');

SELECT 'S1',
  -- Point 1 3 is removed when simplifying the simple (unconstrained) geometry
  ST_Equals(ST_Simplify( g, 1), 'POLYGON((0 0,1 3,-2 3,0 0))'),
  ST_Equals(ST_Simplify(tg, 1), 'POLYGON((0 0,1 3,-2 3,0 0))')
FROM tt.areas WHERE id = 1;
SELECT 'S2',
  ST_Equals(ST_Simplify( g, 1), 'POLYGON((0 0,1 3,2 0,0 0))'),
  ST_Equals(ST_Simplify(tg, 1), 'POLYGON((0 0,1 3,2 0,0 0))')
FROM tt.areas WHERE id = 2;

SELECT DropTopology('tt') IS NULL;
