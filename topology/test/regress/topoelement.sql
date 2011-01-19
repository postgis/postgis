-- Good
SELECT '1','{101,1}'::topology.TopoElement;
SELECT '2','{101,2}'::topology.TopoElement;
SELECT '3','{101,3}'::topology.TopoElement;
-- Invalid: has 3 elements
SELECT '[0:2]={1,2,3}'::topology.TopoElement;
-- Invalid: type element is out of range (1:node, 2:edge, 3:face)
SELECT '{1,0}'::topology.TopoElement;
SELECT '{1,4}'::topology.TopoElement;
