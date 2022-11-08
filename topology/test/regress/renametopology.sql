BEGIN;
SELECT NULL FROM topology.CreateTopology('topo');
SELECT 't1', topology.RenameTopology('topo', 'topo with space');
SELECT 't2', topology.RenameTopology('topo with space', 'topo with MixedCase');
SELECT 't2', topology.RenameTopology('topo with MixedCase', 'topo');
ROLLBACK;
