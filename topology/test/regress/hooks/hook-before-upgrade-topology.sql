SELECT topology.createTopology('upgrade_test');

-- Create some TopoGeometry data
CREATE TABLE upgrade_test.feature(id serial primary key);
SELECT topology.AddTopoGeometryColumn('upgrade_test', 'upgrade_test', 'feature', 'tg', 'linear');
INSERT INTO upgrade_test.feature(tg) SELECT topology.toTopoGeom('LINESTRING(0 0, 10 0)', 'upgrade_test', 1);
CREATE INDEX ON upgrade_test.feature ( id(tg) );

-- Create some TopoGeometry data
CREATE TABLE upgrade_test.domain_test(a topology.topoelement, b topology.topoelementarray);
INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

