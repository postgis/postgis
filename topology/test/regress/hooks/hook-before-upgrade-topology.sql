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

CREATE TABLE upgrade_test.domain_default_test (
  id integer GENERATED ALWAYS AS IDENTITY,
  a topology.topoelement DEFAULT '{5,6}'::topology.topoelement,
  b topology.topoelementarray DEFAULT '{{7,8}}'::topology.topoelementarray
);
INSERT INTO upgrade_test.domain_default_test (a, b) VALUES (
  '{3,4}'::topology.topoelement,
  '{{4,5}}'::topology.topoelementarray
);

CREATE TABLE upgrade_test.domain_partition_test (
  id integer,
  a topology.topoelement DEFAULT '{9,10}'::topology.topoelement
) PARTITION BY RANGE (id);
CREATE TABLE upgrade_test.domain_partition_test_1
  PARTITION OF upgrade_test.domain_partition_test FOR VALUES FROM (0) TO (10);
ALTER TABLE upgrade_test.domain_partition_test_1
  ALTER COLUMN a SET DEFAULT '{13,14}'::topology.topoelement;
INSERT INTO upgrade_test.domain_partition_test VALUES (
  1,
  '{11,12}'::topology.topoelement
);

-- Simulate databases already touched by the broken 3.6 upgrade path:
-- their domain catalogs can name bigint array bases while old rows still
-- carry integer-array storage bytes that must be rewritten.
UPDATE pg_catalog.pg_type
  SET typbasetype = 'bigint[]'::regtype::oid
  WHERE typnamespace = 'topology'::regnamespace
  AND typname = 'topoelement';
UPDATE pg_catalog.pg_type
  SET typbasetype = 'bigint[]'::regtype::oid
  WHERE typnamespace = 'topology'::regnamespace
  AND typname = 'topoelementarray';
