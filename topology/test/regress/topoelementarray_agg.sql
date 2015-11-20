set client_min_messages to WARNING;

SELECT 1, topology.TopoElementArray_agg('{1,3}'::topology.TopoElement);

SELECT 2, topology.TopoElementArray_agg(e) from (
  VALUES ('{4,1}'::TopoElement),
         ('{5,2}')
) as foo(e);

-- See https://trac.osgeo.org/postgis/ticket/3372
SELECT 3, topology.TopoElementArray_agg(e) from (
  SELECT '{4,1}'::TopoElement WHERE false
) as foo(e);
