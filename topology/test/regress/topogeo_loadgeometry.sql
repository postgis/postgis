SET client_min_messages TO WARNING;

-- Unexisted topology call
SELECT 'unexistent_topo', topology.TopoGeo_LoadGeometry('t', 'POINT(0 0)'::geometry);

SELECT NULL FROM topology.CreateTopology('t');

CREATE FUNCTION t.print_counts(lbl text) RETURNS TEXT
AS $$
  SELECT format('%s|%s nodes|%s edges|%s faces',
    lbl,
    ( SELECT count(*) FROM t.node ),
    ( SELECT count(*) FROM t.edge ),
    ( SELECT count(*) FROM t.face WHERE face_id != 0 )
  );
$$ LANGUAGE 'sql';

-- Null call
SELECT 'null', topology.TopoGeo_LoadGeometry('t', NULL::geometry);

-- Empty
SELECT 'empty', topology.TopoGeo_LoadGeometry('t', 'POINT EMPTY'::geometry);

--
--
--            o
--
SELECT topology.TopoGeo_LoadGeometry('t', 'POINT(0 0)');
SELECT t.print_counts('point1');

--
--                      o {5,5}
--
--            o {0,0}
--
SELECT topology.TopoGeo_LoadGeometry('t', 'MULTIPOINT((0 0),(5 5))');
SELECT t.print_counts('mpoint1');

--
--        {0,10}  {5,10}       {10,10}
--            o----------o-----------o
--                       |
--                       |
--                       |
--                       |
--                       |
--                       o {5,5}
--         {0,0}         |
--            o          |
--                       |
--                       |
--                       |
--                       o {5,-10}
--
-- mline1|6 nodes|4 edges|0 faces
--
SELECT topology.TopoGeo_LoadGeometry('t', 'MULTILINESTRING((5 -10,5 10),(0 10,10 10))');
SELECT t.print_counts('mline1');

--     {-10,20}                              {10,20}
--         .-------------------------------------.
--         |                                     |
--         |   .------.{0,16}                    |
--         |   |      |                          |
--         |   o------'                          |
--         |  {-5,15}                            |
--         |                                     |
--         |     {0,10}  {5,10}           {10,10}|
--         |          o---------o----------------o
--         |                    |                |
--         |                    |                |
--         |                    o {5,5}          |
--         |       {0,0}        |                |
--         |          o         |                |
--         |                    |                |
--         o--------------------o----------------'
--     {-10,-10}             {5,-10}          {10,-10}
--
--   mpoly1|8 nodes|8 edges|3 faces
--
SELECT topology.TopoGeo_LoadGeometry('t', 'MULTIPOLYGON(
  ((-10 -10,10 -10,10 20,-10 20,-10 -10)),
  ((-5 15,0 15,0 16,-5 16,-5 15))
)');
SELECT t.print_counts('mpoly1');


--     {-10,20}                              {10,20}
--         .-------------------------------------.
--         |                                     |
--         |   .------.{0,16}     {8,16} o-------o--------.
--         |   |      |                  |       |        |
--         |   o------'                  `-------o--------' {12,15}
--         |  {-5,15}                            |
--         |                                     |
--         |     {0,10}  {5,10}           {10,10}|
--         o----------o---------o----------------o
--         |                    |                |
--         |                    |                |
--         |                    o {5,5}          |
--         |       {0,0}        |                |
--         |          o         |        o       |
--         |                    |     {8,0}      |
--         o--------------------o----------------'
--     {-10,-10}             {5,-10}          {10,-10}
--
--   coll1|13 nodes|15 edges|6 faces
--
SELECT topology.TopoGeo_LoadGeometry('t', 'GEOMETRYCOLLECTION(
  POLYGON((8 16,12 16,12 15,8 16,8 16)),
  MULTIPOINT((8 0)),
  LINESTRING(-10 10,0 10)
)');
SELECT t.print_counts('coll1');

SELECT 'unexpected invalidity', * FROM topology.ValidateTopology('t');

SELECT NULL FROM topology.DropTopology('t');
