-- Ensure prior runs do not leak topologies or noisy notices
SET client_min_messages TO NOTICE;
DO $$
BEGIN
  PERFORM topology.DropTopology('err_loc');
EXCEPTION WHEN OTHERS THEN
  -- Topology may not exist yet; ignore.
END;
$$ LANGUAGE plpgsql;

SELECT 'created', topology.CreateTopology('err_loc', 0, 0) > 0;

SELECT 'n1', topology.ST_AddIsoNode('err_loc', NULL::integer, ST_GeomFromText('POINT(0 0)', 0));
SELECT 'n2', topology.ST_AddIsoNode('err_loc', NULL::integer, ST_GeomFromText('POINT(10 0)', 0));
SELECT 'n3', topology.ST_AddIsoNode('err_loc', NULL::integer, ST_GeomFromText('POINT(5 -5)', 0));
SELECT 'n4', topology.ST_AddIsoNode('err_loc', NULL::integer, ST_GeomFromText('POINT(5 5)', 0));

DO $$
BEGIN
  PERFORM topology.ST_AddIsoNode('err_loc', NULL::integer, ST_GeomFromText('POINT(0 0)', 0));
EXCEPTION WHEN OTHERS THEN
  RAISE NOTICE 'duplicate node: %', regexp_replace(SQLERRM, E'^.*: ', '');
END;
$$ LANGUAGE plpgsql;

SELECT 'base_edge', topology.ST_AddEdgeModFace('err_loc', 1, 2, ST_GeomFromText('LINESTRING(0 0, 10 0)', 0));

DO $$
BEGIN
  PERFORM topology.ST_AddEdgeModFace('err_loc', 3, 4, ST_GeomFromText('LINESTRING(5 -5, 5 5)', 0));
EXCEPTION WHEN OTHERS THEN
  RAISE NOTICE 'crossing edge: %', regexp_replace(SQLERRM, E'^.*: ', '');
END;
$$ LANGUAGE plpgsql;

SET client_min_messages TO WARNING;
SELECT 'dropped', topology.DropTopology('err_loc') IS NOT NULL;
RESET client_min_messages;
