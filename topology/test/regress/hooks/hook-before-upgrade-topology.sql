SELECT topology.createTopology('upgrade_test');

-- Create some TopoGeometry data
CREATE TABLE upgrade_test.feature(id serial primary key);
SELECT topology.AddTopoGeometryColumn('upgrade_test', 'upgrade_test', 'feature', 'tg', 'linear');
INSERT INTO upgrade_test.feature(tg) SELECT topology.toTopoGeom('LINESTRING(0 0, 10 0)', 'upgrade_test', 1);
CREATE INDEX ON upgrade_test.feature ( id(tg) );

-- Create some TopoGeometry data
CREATE TABLE upgrade_test.domain_test(a topology.topoelement, b topology.topoelementarray);
ALTER DOMAIN topology.topoelement
  ADD CONSTRAINT upgrade_test_topoelement_restored CHECK (VALUE IS NULL OR VALUE IS NOT NULL);
ALTER DOMAIN topology.topoelement
  ADD CONSTRAINT upgrade_test_topoelement_already_not_valid CHECK (VALUE IS NULL OR VALUE IS NOT NULL) NOT VALID;
ALTER DOMAIN topology.topoelementarray
  ADD CONSTRAINT upgrade_test_topoelementarray_restored CHECK (VALUE IS NULL OR VALUE IS NOT NULL);
ALTER DOMAIN topology.topoelement SET DEFAULT '{25,26}'::topology.topoelement;
-- Simulate upgrade sources older than the canonical topoelementarray
-- type_range constraint. The repair helper must backfill it with the same
-- NOT VALID policy used for restored constraints when storage is skipped.
ALTER DOMAIN topology.topoelementarray DROP CONSTRAINT type_range;
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

DO $$
BEGIN
  IF EXISTS (SELECT 1 FROM pg_catalog.pg_event_trigger WHERE evtname = 'trg_autovac_disable') THEN
    ALTER EVENT TRIGGER trg_autovac_disable DISABLE;
  END IF;
END
$$;
CREATE TABLE upgrade_test.domain_partition_test (
  id integer,
  a topology.topoelement DEFAULT '{9,10}'::topology.topoelement
) PARTITION BY RANGE (id);
DO $$
BEGIN
  IF EXISTS (SELECT 1 FROM pg_catalog.pg_event_trigger WHERE evtname = 'trg_autovac_disable') THEN
    ALTER EVENT TRIGGER trg_autovac_disable ENABLE;
  END IF;
END
$$;
CREATE TABLE upgrade_test.domain_partition_test_1
  PARTITION OF upgrade_test.domain_partition_test FOR VALUES FROM (0) TO (10);
ALTER TABLE upgrade_test.domain_partition_test_1
  ALTER COLUMN a SET DEFAULT '{13,14}'::topology.topoelement;
INSERT INTO upgrade_test.domain_partition_test VALUES (
  1,
  '{11,12}'::topology.topoelement
);

CREATE TABLE upgrade_test.domain_matview_source (
  a topology.topoelement,
  b topology.topoelementarray
);
INSERT INTO upgrade_test.domain_matview_source VALUES (
  '{19,20}'::topology.topoelement,
  '{{21,22}}'::topology.topoelementarray
);
CREATE MATERIALIZED VIEW upgrade_test.domain_matview_test AS
  SELECT a, b FROM upgrade_test.domain_matview_source;

CREATE TABLE upgrade_test.domain_view_source (
  a topology.topoelement
);
INSERT INTO upgrade_test.domain_view_source VALUES (
  '{23,24}'::topology.topoelement
);
CREATE VIEW upgrade_test.domain_view_test AS
  SELECT a FROM upgrade_test.domain_view_source;

CREATE TABLE upgrade_test.domain_whole_row_view_source (
  a topology.topoelement
);
INSERT INTO upgrade_test.domain_whole_row_view_source VALUES (
  '{31,32}'::topology.topoelement
);
CREATE VIEW upgrade_test.domain_whole_row_view_test AS
  SELECT s FROM upgrade_test.domain_whole_row_view_source AS s;

CREATE TABLE upgrade_test.domain_policy_source (
  id integer,
  a topology.topoelement DEFAULT '{45,46}'::topology.topoelement
);
INSERT INTO upgrade_test.domain_policy_source VALUES (
  1,
  '{47,48}'::topology.topoelement
);
ALTER TABLE upgrade_test.domain_policy_source ENABLE ROW LEVEL SECURITY;
CREATE POLICY domain_policy_source_a_policy
  ON upgrade_test.domain_policy_source
  USING (a[1] > 0)
  WITH CHECK (a[1] > 0);

CREATE TABLE upgrade_test.domain_whole_row_policy_source (
  id integer,
  a topology.topoelement DEFAULT '{81,82}'::topology.topoelement
);
INSERT INTO upgrade_test.domain_whole_row_policy_source VALUES (
  1,
  '{83,84}'::topology.topoelement
);
ALTER TABLE upgrade_test.domain_whole_row_policy_source ENABLE ROW LEVEL SECURITY;
CREATE POLICY domain_whole_row_policy_source_policy
  ON upgrade_test.domain_whole_row_policy_source
  USING (domain_whole_row_policy_source IS NOT NULL)
  WITH CHECK (domain_whole_row_policy_source IS NOT NULL);

CREATE TABLE upgrade_test.domain_trigger_source (
  id integer,
  a topology.topoelement DEFAULT '{49,50}'::topology.topoelement
);
INSERT INTO upgrade_test.domain_trigger_source VALUES (
  1,
  '{51,52}'::topology.topoelement
);
CREATE FUNCTION upgrade_test.domain_trigger_source_guard()
RETURNS trigger
LANGUAGE plpgsql
AS $upgrade$
BEGIN
  RETURN NEW;
END;
$upgrade$;
CREATE TRIGGER domain_trigger_source_a_trigger
  BEFORE UPDATE OF a ON upgrade_test.domain_trigger_source
  FOR EACH ROW EXECUTE FUNCTION upgrade_test.domain_trigger_source_guard();

CREATE TABLE upgrade_test.domain_whole_row_trigger_source (
  id integer,
  a topology.topoelement DEFAULT '{89,90}'::topology.topoelement
);
INSERT INTO upgrade_test.domain_whole_row_trigger_source VALUES (
  1,
  '{91,92}'::topology.topoelement
);
CREATE TRIGGER domain_whole_row_trigger_source_trigger
  BEFORE UPDATE ON upgrade_test.domain_whole_row_trigger_source
  FOR EACH ROW
  WHEN (OLD IS NOT NULL)
  EXECUTE FUNCTION upgrade_test.domain_trigger_source_guard();

CREATE TABLE upgrade_test.domain_function_source (
  id integer,
  a topology.topoelement DEFAULT '{61,62}'::topology.topoelement
);
INSERT INTO upgrade_test.domain_function_source VALUES (
  1,
  '{63,64}'::topology.topoelement
);
CREATE FUNCTION upgrade_test.domain_function_source_read()
RETURNS integer
LANGUAGE SQL
BEGIN ATOMIC
  SELECT a[1]
    FROM upgrade_test.domain_function_source
    LIMIT 1;
END;

DO $$
BEGIN
  IF current_setting('server_version_num')::integer >= 150000 THEN
    EXECUTE 'CREATE TABLE upgrade_test.domain_publication_source (
      id integer,
      a topology.topoelement DEFAULT ''{53,54}''::topology.topoelement
    )';
    INSERT INTO upgrade_test.domain_publication_source VALUES (
      1,
      '{55,56}'::topology.topoelement
    );
    EXECUTE 'CREATE PUBLICATION upgrade_test_domain_publication
      FOR TABLE upgrade_test.domain_publication_source (a)';
  END IF;
END
$$;

CREATE TABLE upgrade_test.domain_level_default_test (
  id integer GENERATED ALWAYS AS IDENTITY,
  a topology.topoelement
);

CREATE TABLE upgrade_test.domain_array_test (
  id integer GENERATED ALWAYS AS IDENTITY,
  a topology.topoelement[] DEFAULT ARRAY[NULL::topology.topoelement, '{75,76}'::topology.topoelement]::topology.topoelement[],
  b topology.topoelementarray[] DEFAULT ARRAY[NULL::topology.topoelementarray, '{{77,78}}'::topology.topoelementarray]::topology.topoelementarray[]
);
INSERT INTO upgrade_test.domain_array_test (a, b) VALUES (
  ARRAY[NULL::topology.topoelement, '{67,68}'::topology.topoelement]::topology.topoelement[],
  ARRAY[NULL::topology.topoelementarray, '{{69,70}}'::topology.topoelementarray]::topology.topoelementarray[]
);
ALTER DOMAIN topology.topoelement
  ADD CONSTRAINT upgrade_test_topoelement_array_grandfather CHECK (VALUE IS NULL OR VALUE[1] != 67) NOT VALID;
ALTER DOMAIN topology.topoelementarray
  ADD CONSTRAINT upgrade_test_topoelementarray_array_grandfather CHECK (VALUE IS NULL OR VALUE[1][1] != 69) NOT VALID;

CREATE TABLE upgrade_test.domain_array_constraint_test (
  id integer GENERATED ALWAYS AS IDENTITY,
  a topology.topoelement[] DEFAULT ARRAY['{93,94}'::topology.topoelement]::topology.topoelement[],
  CONSTRAINT domain_array_constraint_check CHECK ((a[1])[1] > 0)
);
CREATE INDEX domain_array_constraint_expr_idx
  ON upgrade_test.domain_array_constraint_test (((a[1])[2]));
INSERT INTO upgrade_test.domain_array_constraint_test (a) VALUES (
  ARRAY['{95,96}'::topology.topoelement]::topology.topoelement[]
);

CREATE DOMAIN upgrade_test.nested_topoelement AS topology.topoelement;
CREATE TYPE upgrade_test.composite_topoelement AS (
  a topology.topoelement
);
CREATE TABLE upgrade_test.domain_nested_type_test (
  id integer GENERATED ALWAYS AS IDENTITY,
  a upgrade_test.nested_topoelement,
  b upgrade_test.composite_topoelement
);
INSERT INTO upgrade_test.domain_nested_type_test (a, b) VALUES (
  '{85,86}'::topology.topoelement::upgrade_test.nested_topoelement,
  ROW('{87,88}'::topology.topoelement)::upgrade_test.composite_topoelement
);

CREATE TABLE upgrade_test.domain_rule_source (
  id integer,
  a topology.topoelement
);
CREATE TABLE upgrade_test.domain_rule_log (
  a topology.topoelement
);
CREATE RULE domain_rule_source_insert AS
  ON INSERT TO upgrade_test.domain_rule_source
  DO ALSO INSERT INTO upgrade_test.domain_rule_log VALUES (NEW.a);
INSERT INTO upgrade_test.domain_rule_source VALUES (
  1,
  '{27,28}'::topology.topoelement
);

CREATE TABLE upgrade_test.domain_external_rule_source (
  id integer,
  a topology.topoelement
);
CREATE TABLE upgrade_test.domain_external_rule_target (
  id integer
);
CREATE RULE domain_external_rule_target_insert AS
  ON INSERT TO upgrade_test.domain_external_rule_target
  DO ALSO INSERT INTO upgrade_test.domain_rule_log
    SELECT a
    FROM upgrade_test.domain_external_rule_source
    WHERE id = NEW.id;
INSERT INTO upgrade_test.domain_external_rule_source VALUES (
  2,
  '{29,30}'::topology.topoelement
);

DO $$
BEGIN
  IF EXISTS (SELECT 1 FROM pg_catalog.pg_event_trigger WHERE evtname = 'trg_autovac_disable') THEN
    ALTER EVENT TRIGGER trg_autovac_disable DISABLE;
  END IF;
END
$$;
CREATE TABLE upgrade_test.domain_blocked_partition_parent (
  id integer,
  a topology.topoelement
) PARTITION BY RANGE (id);
DO $$
BEGIN
  IF EXISTS (SELECT 1 FROM pg_catalog.pg_event_trigger WHERE evtname = 'trg_autovac_disable') THEN
    ALTER EVENT TRIGGER trg_autovac_disable ENABLE;
  END IF;
END
$$;
CREATE TABLE upgrade_test.domain_blocked_partition_child
  PARTITION OF upgrade_test.domain_blocked_partition_parent FOR VALUES FROM (0) TO (10);
INSERT INTO upgrade_test.domain_blocked_partition_parent VALUES (
  1,
  '{33,34}'::topology.topoelement
);
CREATE VIEW upgrade_test.domain_blocked_partition_child_view AS
  SELECT c FROM upgrade_test.domain_blocked_partition_child AS c;

CREATE TABLE upgrade_test.domain_trigger_child_parent (
  id integer,
  a topology.topoelement
) PARTITION BY RANGE (id);
CREATE TABLE upgrade_test.domain_trigger_child_child
  PARTITION OF upgrade_test.domain_trigger_child_parent FOR VALUES FROM (0) TO (10);
INSERT INTO upgrade_test.domain_trigger_child_parent VALUES (
  1,
  '{57,58}'::topology.topoelement
);
CREATE TRIGGER domain_trigger_child_a_trigger
  BEFORE UPDATE OF a ON upgrade_test.domain_trigger_child_child
  FOR EACH ROW EXECUTE FUNCTION upgrade_test.domain_trigger_source_guard();

CREATE TABLE upgrade_test.domain_function_child_parent (
  id integer,
  a topology.topoelement
) PARTITION BY RANGE (id);
CREATE TABLE upgrade_test.domain_function_child_child
  PARTITION OF upgrade_test.domain_function_child_parent FOR VALUES FROM (0) TO (10);
INSERT INTO upgrade_test.domain_function_child_parent VALUES (
  1,
  '{65,66}'::topology.topoelement
);
CREATE FUNCTION upgrade_test.domain_function_child_read()
RETURNS integer
LANGUAGE SQL
BEGIN ATOMIC
  SELECT a[1]
    FROM upgrade_test.domain_function_child_child
    LIMIT 1;
END;

DO $$
BEGIN
  IF current_setting('server_version_num')::integer >= 150000 THEN
    EXECUTE 'CREATE TABLE upgrade_test.domain_publication_child_parent (
      id integer,
      a topology.topoelement
    ) PARTITION BY RANGE (id)';
    EXECUTE 'CREATE TABLE upgrade_test.domain_publication_child_child
      PARTITION OF upgrade_test.domain_publication_child_parent FOR VALUES FROM (0) TO (10)';
    INSERT INTO upgrade_test.domain_publication_child_parent VALUES (
      1,
      '{59,60}'::topology.topoelement
    );
    EXECUTE 'CREATE PUBLICATION upgrade_test_domain_child_publication
      FOR TABLE upgrade_test.domain_publication_child_child (a)';
  END IF;
END
$$;

CREATE TABLE upgrade_test.domain_blocked_inherit_parent (
  a topology.topoelement
);
CREATE TABLE upgrade_test.domain_blocked_inherit_child ()
  INHERITS (upgrade_test.domain_blocked_inherit_parent);
CREATE TABLE upgrade_test.domain_blocked_inherit_grandchild ()
  INHERITS (upgrade_test.domain_blocked_inherit_child);
INSERT INTO upgrade_test.domain_blocked_inherit_grandchild VALUES (
  '{35,36}'::topology.topoelement
);
CREATE VIEW upgrade_test.domain_blocked_inherit_grandchild_view AS
  SELECT g FROM upgrade_test.domain_blocked_inherit_grandchild AS g;

DO $$
BEGIN
  IF current_setting('server_version_num')::integer >= 170000 THEN
    CREATE TABLE upgrade_test.domain_generated_test (
      id integer,
      a topology.topoelement
        GENERATED ALWAYS AS (ARRAY[id, id + 1]::topology.topoelement) STORED,
      b topology.topoelementarray
        GENERATED ALWAYS AS (ARRAY[ARRAY[id + 2, id + 3]]::topology.topoelementarray) STORED
    );
    INSERT INTO upgrade_test.domain_generated_test(id) VALUES (15);

    CREATE TABLE upgrade_test.domain_generated_source_guard (
      fail boolean NOT NULL
    );
    INSERT INTO upgrade_test.domain_generated_source_guard VALUES (false);
    CREATE FUNCTION upgrade_test.domain_generated_source_value(value topology.topoelement)
      RETURNS topology.topoelement
      LANGUAGE plpgsql
      IMMUTABLE
    AS $upgrade$
    DECLARE
      fail boolean;
    BEGIN
      SELECT g.fail
        INTO STRICT fail
        FROM upgrade_test.domain_generated_source_guard AS g;
      IF fail THEN
        RAISE EXCEPTION 'generated source dependency was recomputed';
      END IF;
      RETURN ARRAY[value[1], value[2]]::topology.topoelement;
    END;
    $upgrade$;

    CREATE FUNCTION upgrade_test.domain_generated_source_array_value(value topology.topoelement)
    RETURNS topology.topoelementarray
    LANGUAGE 'plpgsql'
    IMMUTABLE
    AS $upgrade$
    DECLARE
      fail boolean;
    BEGIN
      SELECT g.fail
        INTO STRICT fail
        FROM upgrade_test.domain_generated_source_guard AS g;
      IF fail THEN
        RAISE EXCEPTION 'generated source dependency was recomputed';
      END IF;
      RETURN ARRAY[ARRAY[value[1], value[2]]]::topology.topoelementarray;
    END;
    $upgrade$;

    CREATE TABLE upgrade_test.domain_generated_source_dependency_test (
      id integer,
      a topology.topoelement,
      g topology.topoelement
        GENERATED ALWAYS AS (upgrade_test.domain_generated_source_value(a)) STORED
    );
    INSERT INTO upgrade_test.domain_generated_source_dependency_test(id, a)
      VALUES (1, '{39,40}'::topology.topoelement);

    CREATE TABLE upgrade_test.domain_generated_cross_dependency_test (
      id integer,
      a topology.topoelement,
      g topology.topoelementarray
        GENERATED ALWAYS AS (upgrade_test.domain_generated_source_array_value(a)) STORED
    );
    INSERT INTO upgrade_test.domain_generated_cross_dependency_test(id, a)
      VALUES (1, '{43,44}'::topology.topoelement);

    CREATE TABLE upgrade_test.domain_generated_nested_source_dependency_test (
      id integer,
      a upgrade_test.nested_topoelement,
      g topology.topoelement
        GENERATED ALWAYS AS (upgrade_test.domain_generated_source_value(a::topology.topoelement)) STORED
    );
    INSERT INTO upgrade_test.domain_generated_nested_source_dependency_test(id, a)
      VALUES (1, '{97,98}'::topology.topoelement::upgrade_test.nested_topoelement);
    UPDATE upgrade_test.domain_generated_source_guard SET fail = true;
  END IF;
END
$$;

CREATE TABLE upgrade_test.domain_generated_dependency_test (
  id integer,
  a topology.topoelement DEFAULT '{41,42}'::topology.topoelement,
  generated_id bigint GENERATED ALWAYS AS (a[1]) STORED
);
INSERT INTO upgrade_test.domain_generated_dependency_test(id, a)
  VALUES (1, '{37,38}'::topology.topoelement);

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
