\set VERBOSITY terse
set client_min_messages to ERROR;

\i :top_builddir/topology/test/load_topology.sql

SELECT 't1', 'count', count(*) FROM topology.FindVertexSegmentPairsBelowDistance('city_data', 1);
SELECT 't2', seg_edge, segno, vert_edge, vertno, ST_AsText(vert_geom)
FROM topology.FindVertexSegmentPairsBelowDistance('city_data', 1.1)
ORDER BY 1,2,3,4;

SELECT NULL FROM topology.DropTopology('city_data');
