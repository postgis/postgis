\set VERBOSITY terse
set client_min_messages to ERROR;

\i ../load_topology-4326.sql

-- Save max node id
select 'node'::text as what, max(node_id) INTO city_data.limits FROM city_data.node;
INSERT INTO city_data.limits select 'edge'::text as what, max(edge_id) FROM city_data.edge;
SELECT 'max',* from city_data.limits;

-- Check changes since last saving, save more
-- {
CREATE OR REPLACE FUNCTION check_changes(lbl text)
RETURNS TABLE (o text)
AS $$
DECLARE
  rec RECORD;
  sql text;
BEGIN
  -- Check effect on nodes
  sql :=  'SELECT $1 || ''|N|'' ||
        COALESCE(n.containing_face::text,'''') || ''|'' ||
        ST_AsText(n.geom, 2)::text as xx
  	FROM city_data.node n WHERE n.node_id > (
    		SELECT max FROM city_data.limits WHERE what = ''node''::text )
  		ORDER BY n.geom';

  FOR rec IN EXECUTE sql USING ( lbl )
  LOOP
    o := rec.xx;
    RETURN NEXT;
  END LOOP;

  -- Check effect on edges
  sql := 'WITH node_limits AS ( SELECT max FROM city_data.limits WHERE what = ''node''::text ),
       edge_limits AS ( SELECT max FROM city_data.limits WHERE what = ''edge''::text )
  SELECT $1 || ''|E|'' || ST_AsText(e.geom, 2)::text AS xx ' ||
   ' FROM city_data.edge e, node_limits nl, edge_limits el
   WHERE e.start_node > nl.max
      OR e.end_node > nl.max
      OR e.edge_id > el.max
  ORDER BY e.geom;';

  FOR rec IN EXECUTE sql USING ( lbl )
  LOOP
    o := rec.xx;
    RETURN NEXT;
  END LOOP;

  UPDATE city_data.limits SET max = (SELECT max(n.node_id) FROM city_data.node n) WHERE what = 'node';
  UPDATE city_data.limits SET max = (SELECT max(e.edge_id) FROM city_data.edge e) WHERE what = 'edge';

END;
$$ LANGUAGE 'plpgsql';
-- }

-- Invalid calls
SELECT 'invalid', TopoGeo_addLineString('city_data', 'SRID=4326;MULTILINESTRING((36 26, 38 30))');
SELECT 'invalid', TopoGeo_addLineString('city_data', 'SRID=4326;POINT(36 26)');
SELECT 'invalid', TopoGeo_addLineString('invalid', 'SRID=4326;LINESTRING(36 26, 0 0)');

-- Isolated edge in universal face
SELECT 'iso_uni', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(36 26, 38 30)');
SELECT check_changes('iso_uni');

-- Isolated edge in face 5
SELECT 'iso_f5', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(37 20, 43 19, 41 16)');
SELECT check_changes('iso_f5');

-- Existing isolated edge
SELECT 'iso_ex', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(36 26, 38 30)');
SELECT check_changes('iso_ex');

-- Existing isolated edge within tolerance
SELECT 'iso_ex_tol', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(36 27, 38 31)', 2);
SELECT check_changes('iso_ex_tol');

-- Existing non-isolated edge
SELECT 'noniso_ex', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(35 6, 35 14)');
SELECT check_changes('noniso_ex');

-- Existing non-isolated edge within tolerance
SELECT 'noniso_ex_tol', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(35 7, 35 13)', 2);
SELECT check_changes('noniso_ex_tol');

-- Fully contained
SELECT 'contained', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(35 8, 35 12)');
SELECT check_changes('contained');

-- Overlapping
SELECT 'overlap', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(45 22, 49 22)') ORDER BY 2;
SELECT check_changes('overlap');

-- Crossing
-- TODO: Geos 3.8+ gives different results, so just returning the count of edge instead
--       strk fix as you please leter
SELECT 'cross', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(49 18, 44 17)') ORDER BY 2;
SELECT check_changes('cross');

-- Snapping (and splitting a face)
SELECT 'snap', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(18 22.2, 22.5 22.2, 21.2 20.5)', 1) ORDER BY 2;
SELECT check_changes('snap');
SELECT 'snap_again', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(18 22.2, 22.5 22.2, 21.2 20.5)', 1) ORDER BY 2;
SELECT check_changes('snap_again');

-- A mix of crossing and overlapping, splitting another face
-- TODO: Geos 3.8+ gives different results, so just returning the count of edges instead
--       strk fix as you please leter
--SELECT 'crossover', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(9 18, 9 20, 21 10, 21 7)') ORDER BY 2;
SELECT 'crossover', COUNT(*) FROM TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(9 18, 9 20, 21 10, 21 7)') AS t;
SELECT check_changes('crossover');

-- TODO: Geos 3.8+ gives different results, so just returning the count of edges instead
--       strk fix as you please leter
--SELECT 'crossover_again', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(9 18, 9 20, 21 10, 21 7)') ORDER BY 2;
SELECT 'crossover_again', COUNT(*) FROM TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(9 18, 9 20, 21 10, 21 7)') AS t;
SELECT check_changes('crossover_again');

-- Fully containing
SELECT 'contains', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(14 34, 13 35, 10 35, 9 35, 7 36)') ORDER BY 2;
-- answers different between 3.8 and older geos so disabling output of ids and geometry
SELECT check_changes('contains');

-- Crossing a node
SELECT 'nodecross', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(18 37, 22 37)') ORDER BY 2;
SELECT check_changes('nodecross');

-- Existing isolated edge with 2 segments
SELECT 'iso_ex_2segs', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(37 20, 43 19, 41 16)');
SELECT check_changes('iso_ex_2segs');

-- See http://trac.osgeo.org/postgis/attachment/ticket/1613

SELECT '#1613.1', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(556267.562954 144887.066638, 556267 144887.4)') ORDER BY 2;
SELECT check_changes('#1613.1');
SELECT '#1613.2', TopoGeo_addLineString('city_data', 'SRID=4326;LINESTRING(556250 144887, 556267 144887.07, 556310.04 144887)') ORDER BY 2;
SELECT check_changes('#1613.2');

-- Consistency check
SELECT * FROM ValidateTopology('city_data');

-- See http://trac.osgeo.org/postgis/ticket/1631

-- clean all up first
DELETE FROM city_data.edge_data;
DELETE FROM city_data.node;
DELETE FROM city_data.face where face_id > 0;

SELECT '#1631.1', TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(556267.56295432 144887.06663814,556267.566 144888)'
) ORDER BY 2;
SELECT check_changes('#1631.1');
SELECT '#1631.2', TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(556254.67 144886.62, 556267.66 144887.07)'
) ORDER BY 2;
SELECT check_changes('#1631.2');

-- Consistency check
SELECT * FROM ValidateTopology('city_data');

-- See http://trac.osgeo.org/postgis/ticket/1641

-- clean all up first
DELETE FROM city_data.edge_data; DELETE FROM city_data.node;
DELETE FROM city_data.face where face_id > 0;

SELECT '#1641.1', TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(-0.223586 0.474301, 0.142550 0.406124)'
) ORDER BY 2;
SELECT check_changes('#1641.1');
-- Use a tolerance
SELECT '#1641.2', TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(0.095989 0.113619, -0.064646 0.470149)'
  , 1e-16
) ORDER BY 2;
SELECT check_changes('#1641.2');

-- Consistency check
SELECT * FROM ValidateTopology('city_data');

-- Now w/out explicit tolerance (will use local min)
-- clean all up first
DELETE FROM city_data.edge_data; DELETE FROM city_data.node;
DELETE FROM city_data.face where face_id > 0;

SELECT '#1641.3', TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(-0.223586 0.474301, 0.142550 0.406124)'
) ORDER BY 2;
SELECT check_changes('#1641.3');
SELECT '#1641.4', TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(0.095989 0.113619, -0.064646 0.470149)'
) ORDER BY 2;
SELECT check_changes('#1641.4');

-- Consistency check
SELECT * FROM ValidateTopology('city_data');

-- See http://trac.osgeo.org/postgis/ticket/1650

DELETE FROM city_data.edge_data; DELETE FROM city_data.node;
DELETE FROM city_data.face where face_id > 0;

SELECT '#1650.1' UNION ALL
SELECT '#1650.2' || TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(0 0, 0 1)'
, 2)::text;
SELECT check_changes('#1650.2');

SELECT '#1650.3', TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(-1 0, 10 0)'
, 2) ORDER BY 2;
SELECT check_changes('#1650.3');

-- Consistency check
SELECT * FROM ValidateTopology('city_data');

-- Test snapping of line over a node
-- See http://trac.osgeo.org/postgis/ticket/1654

DELETE FROM city_data.edge_data; DELETE FROM city_data.node;
DELETE FROM city_data.face where face_id > 0;

SELECT '#1654.1', 'N', ST_AddIsoNode('city_data', 0, 'SRID=4326;POINT(0 0)');
SELECT check_changes('#1654.1');
SELECT '#1654.2', TopoGeo_addLineString('city_data',
  'SRID=4326;LINESTRING(-10 1, 10 1)'
, 2) ORDER BY 2;
SELECT check_changes('#1654.2');

-- Consistency check
SELECT * FROM ValidateTopology('city_data');

-- Test snapping of new edge endpoints
-- See http://trac.osgeo.org/postgis/ticket/1706

DELETE FROM city_data.edge_data; DELETE FROM city_data.node;
DELETE FROM city_data.face where face_id > 0;

SELECT '#1706.1', 'E', TopoGeo_AddLineString('city_data',
 'SRID=4326;LINESTRING(20 10, 10 10, 9 12, 10 20)');
SELECT check_changes('#1706.1');

SELECT '#1706.2', count(*) FROM TopoGeo_addLineString('city_data',
 'SRID=4326;LINESTRING(10 0, 10 10, 15 10, 20 10)' , 4);
SELECT check_changes('#1706.2');

-- Consistency check
SELECT * FROM ValidateTopology('city_data');

-- Test noding after snap
-- See http://trac.osgeo.org/postgis/ticket/1714

DELETE FROM city_data.edge_data; DELETE FROM city_data.node;
DELETE FROM city_data.face where face_id > 0;

SELECT '#1714.1', 'N', AddNode('city_data', 'SRID=4326;POINT(10 0)', false, true);
SELECT check_changes('#1714.1');

SELECT '#1714.2', 'E*', TopoGeo_addLineString('city_data',
 'SRID=4326;LINESTRING(10 0, 0 20, 0 0, 10 0)'
, 12) ORDER BY 3;
SELECT check_changes('#1714.2');

-- Consistency check
SELECT * FROM ValidateTopology('city_data');

-- Cleanups
DROP FUNCTION check_changes(text);
SELECT DropTopology('city_data');

-- See http://trac.osgeo.org/postgis/ticket/3280
SELECT 't3280.start', topology.CreateTopology('bug3280') > 0;
SELECT 't3280', 'L1' || topology.TopoGeo_AddLinestring('bug3280',
 '010200000002000000EC51B89E320F3841333333B3A9C8524114AE47611D0F384114AE47B17DC85241'
 ::geometry);
SELECT 't3280', 'L2' || topology.TopoGeo_AddLinestring('bug3280',
 '010200000003000000EC51B89E320F3841333333B3A9C852415649EE1F280F384164E065F493C85241A4703D8A230F38410AD7A37094C85241'
 ::geometry);
SELECT 't3280', 'L1b' || l
 FROM (SELECT * FROM bug3280.edge where edge_id = 1) AS e
    CROSS JOIN LATERAL topology.TopoGeo_AddLinestring('bug3280', geom) AS l
 ORDER BY 2;
SELECT 't3280.end', topology.DropTopology('bug3280');

-- See http://trac.osgeo.org/postgis/ticket/3380
SELECT 't3380.start', CreateTopology( 'bug3380', 0, 0.01) > 0;
SELECT 't3380.L1', TopoGeo_AddLinestring('bug3380', '
LINESTRING(
1612829.90652844007126987 4841274.48807844985276461,
1612830.1566380700096488 4841287.23833953030407429,
1612883.15799825009889901 4841277.73794914968311787)
', 0);
SELECT 't3380.L2', TopoGeo_AddLinestring('bug3380', '
LINESTRING(
1612790.88055733009241521 4841286.88526585046201944,
1612830.15823523001745343 4841287.12674008030444384,
1612829.98813172010704875 4841274.56198261026293039)
', 0);
SELECT 't3380.L3', TopoGeo_AddLinestring('bug3380', '
 LINESTRING(
1612830.15823523 4841287.12674008,
1612881.64990281 4841274.56198261)
', 0);
SELECT 't3380.end', DropTopology( 'bug3380' );

-- See http://trac.osgeo.org/postgis/ticket/3402

SELECT 't3402.start', CreateTopology('bug3402') > 1;
SELECT 't3402.L1', TopoGeo_addLinestring('bug3402',
'010200000003000000C1AABC2B192739418E7DE0E6AB9652411F85EB5119283941F6285CEF2D9652411F85EB5128283941F6285CCF2C965241'
, 0);
SELECT 't3402.L2', TopoGeo_addLinestring('bug3402',
'010200000003000000BCAABC2B192739418F7DE0E6AB96524185EB51382828394115AE47D12C96524187EB51382828394115AE47D12C965241'
, 0);
SELECT 't3402.end', DropTopology('bug3402');

-- See http://trac.osgeo.org/postgis/ticket/3412
SELECT 't3412.start', CreateTopology('bug3412', 0, 0.001) > 0;
SELECT 't3412.L1', TopoGeo_AddLinestring('bug3412',
'LINESTRING(
599671.37 4889664.32,
599665.52 4889680.18,
599671.37 4889683.4,
599671.37 4889781.87
)'
::geometry, 0);

-- TODO: answers different on 3.8 from older geos so revised test
/**SELECT 't3412.L2', TopoGeo_AddLinestring('bug3412',
'0102000000020000003AB42BBFEE4C22410010C5A997A6524167BB5DBDEE4C224117FE3DA85FA75241'
::geometry, 0);**/
SELECT 't3412.L2', COUNT(*)
FROM TopoGeo_AddLinestring('bug3412',
'0102000000020000003AB42BBFEE4C22410010C5A997A6524167BB5DBDEE4C224117FE3DA85FA75241'
::geometry, 0);
SELECT 't3412.end', DropTopology('bug3412');

-- See http://trac.osgeo.org/postgis/ticket/3711
SELECT 't3371.start', topology.CreateTopology('bug3711', 0, 0, true) > 1;
SELECT 't3371.L1', topology.TopoGeo_AddLineString('bug3711',
'LINESTRING (618369 4833784 0.88, 618370 4833784 1.93, 618370 4833780 1.90)'
::geometry, 0);
SELECT 't3371.L2', count(*) FROM topology.TopoGeo_AddLineString( 'bug3711',
'LINESTRING (618370 4833780 1.92, 618370 4833784 1.90, 618371 4833780 1.93)'
::geometry, 0);
SELECT 't3371.end', topology.DropTopology('bug3711');

-- See http://trac.osgeo.org/postgis/ticket/3838
SELECT 't3838.start', topology.CreateTopology('bug3838') > 1;
SELECT 't3838.L1', topology.TopoGeo_addLinestring('bug3838',
'LINESTRING(
622617.12 6554996.14,
622612.06 6554996.7,
622609.17 6554995.51,
622606.83 6554996.14,
622598.73 6554996.23,
622591.53 6554995.96)'
::geometry , 1);
-- TODO: answers in geos 3.8 different from older geos
-- So just doing count instead of full test
/** SELECT 't3838.L2', topology.TopoGeo_addLinestring('bug3838',
'LINESTRING(622608 6554988, 622596 6554984)'
::geometry , 10);**/
SELECT 't3838.L2', COUNT(*)
  FROM topology.TopoGeo_addLinestring('bug3838',
'LINESTRING(622608 6554988, 622596 6554984)'
::geometry , 10);
SELECT 't3838.end', topology.DropTopology('bug3838');

-- See https://trac.osgeo.org/postgis/ticket/1855
-- Simplified case 1
SELECT 't1855_1.start', topology.CreateTopology('bug1855') > 0;
SELECT 't1855_1.0', topology.TopoGeo_addLinestring('bug1855',
  'LINESTRING(0 0, 10 0, 0 1)', 2);
SELECT 't1855_1.end', topology.DropTopology('bug1855');
-- Simplified case 2
SELECT 't1855_2.start', topology.CreateTopology('bug1855') > 0;
SELECT 't1855_2.0', topology.topogeo_AddLinestring('bug1855',
  'LINESTRING(0 0, 0 100)');
SELECT 't1855_2.1', count(*) FROM topology.topogeo_AddLinestring('bug1855',
  'LINESTRING(10 51, -100 50, 10 49)', 2);
SELECT 't1855_2.end', topology.DropTopology('bug1855');

-- See https://trac.osgeo.org/postgis/ticket/4757
SELECT 't4757.start', topology.CreateTopology('bug4757') > 0;
SELECT 't4757.0', topology.TopoGeo_addPoint('bug4757', 'POINT(0 0)');
SELECT 't4757.1', topology.TopoGeo_addLinestring('bug4757',
  'LINESTRING(0 -0.1,1 0,1 1,0 1,0 -0.1)', 1);
SELECT 't4757.end', topology.DropTopology('bug4757');

-- See https://trac.osgeo.org/postgis/ticket/t4758
select 't4758.start', topology.CreateTopology ('t4758', 0, 1e-06) > 0;
select 't4758.0', topology.TopoGeo_addLinestring('t4758',
  'LINESTRING(11.38327215  60.4081942, 11.3826176   60.4089484)');
select 't4758.1', topology.TopoGeo_addLinestring('t4758',
  'LINESTRING( 11.3832721  60.408194249999994, 11.38327215 60.4081942)');
SELECT 't4758.2', t
FROM topology.TopoGeo_addLinestring('t4758',
  'LINESTRING( 11.38330505 60.408239599999995, 11.3832721  60.408194249999994)') AS t
ORDER BY t;
SELECT 't4758.end', topology.DropTopology('t4758');
