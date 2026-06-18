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

-- https://trac.osgeo.org/postgis/ticket/6016
DO $$
DECLARE
  row RECORD;
BEGIN
  SELECT a, b INTO STRICT row
    FROM upgrade_test.domain_test
    ORDER BY ctid ASC
    LIMIT 1;

  IF row.a[1] != 1 OR row.a[2] != 2 THEN
    RAISE EXCEPTION 'topoelement storage was not rewritten during upgrade: %', row.a;
  END IF;

  IF row.b[1][1] != 2 OR row.b[1][2] != 3 THEN
    RAISE EXCEPTION 'topoelementarray storage was not rewritten during upgrade: %', row.b;
  END IF;
END
$$;

INSERT INTO upgrade_test.domain_default_test DEFAULT VALUES;
SELECT id, a[1], a[2], b[1][1], b[1][2]
  FROM upgrade_test.domain_default_test
  ORDER BY id;

INSERT INTO upgrade_test.domain_partition_test (id) VALUES (2);
INSERT INTO upgrade_test.domain_partition_test_1 (id) VALUES (3);
SELECT tableoid::regclass, id, a[1], a[2]
  FROM upgrade_test.domain_partition_test
  ORDER BY id;

INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

SELECT topology.DropTopology('upgrade_test');
SELECT topology.DropTopology('upgrade_test_copy');
