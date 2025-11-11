SELECT * FROM topology.layer;
\d upgrade_test.feature
-- https://trac.osgeo.org/postgis/ticket/5983
SELECT topology.FixCorruptTopoGeometryColumn(schema_name, table_name, feature_column)
    FROM topology.layer;

\d upgrade_test.feature

-- See https://trac.osgeo.org/postgis/ticket/5102
SELECT topology.CopyTopology('upgrade_test', 'upgrade_test_copy');
INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

SELECT * FROM topology.layer;

INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

SELECT topology.DropTopology('upgrade_test');
SELECT topology.DropTopology('upgrade_test_copy');

