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
  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelement'::regtype
    AND conname = 'upgrade_test_topoelement_restored'
    AND NOT convalidated
  ) THEN
    RAISE EXCEPTION 'topoelement domain constraint was not restored unvalidated during incomplete upgrade repair';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelement'::regtype
    AND conname = 'upgrade_test_topoelement_restored'
    AND pg_catalog.obj_description(oid, 'pg_constraint')
      LIKE '%postgis-topology-domain-constraint-not-valid-by-repair-306%'
  ) THEN
    RAISE EXCEPTION 'repair-demoted topoelement domain constraint was not marked for complete retry revalidation';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelement'::regtype
    AND conname = 'upgrade_test_topoelement_already_not_valid'
    AND NOT convalidated
  ) THEN
    RAISE EXCEPTION 'pre-existing unvalidated topoelement domain constraint was not restored during incomplete upgrade repair';
  END IF;

  IF EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelement'::regtype
    AND conname = 'upgrade_test_topoelement_already_not_valid'
    AND pg_catalog.obj_description(oid, 'pg_constraint')
      LIKE '%postgis-topology-domain-constraint-not-valid-by-repair-306%'
  ) THEN
    RAISE EXCEPTION 'user-created NOT VALID topoelement domain constraint was incorrectly marked for revalidation';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelement'::regtype
    AND conname = 'upgrade_test_topoelement_array_grandfather'
    AND NOT convalidated
  ) THEN
    RAISE EXCEPTION 'grandfathered topoelement domain array constraint was not preserved unvalidated';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelementarray'::regtype
    AND conname = 'upgrade_test_topoelementarray_restored'
    AND NOT convalidated
  ) THEN
    RAISE EXCEPTION 'topoelementarray domain constraint was not restored unvalidated during incomplete upgrade repair';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelementarray'::regtype
    AND conname = 'upgrade_test_topoelementarray_array_grandfather'
    AND NOT convalidated
  ) THEN
    RAISE EXCEPTION 'grandfathered topoelementarray domain array constraint was not preserved unvalidated';
  END IF;

  IF (
    SELECT count(*)
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelement'::regtype
    AND conname IN ('dimensions', 'type_range', 'lower_dimension')
    AND NOT convalidated
  ) != 3 THEN
    RAISE EXCEPTION 'canonical topoelement domain constraints were not preserved unvalidated during incomplete upgrade repair';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid = 'topology.topoelementarray'::regtype
    AND conname = 'type_range'
    AND NOT convalidated
  ) THEN
    RAISE EXCEPTION 'canonical topoelementarray domain constraint was not preserved unvalidated during incomplete upgrade repair';
  END IF;

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

INSERT INTO upgrade_test.domain_level_default_test DEFAULT VALUES;
SELECT id, a[1], a[2]
  FROM upgrade_test.domain_level_default_test
  ORDER BY id;

INSERT INTO upgrade_test.domain_array_test DEFAULT VALUES;
SELECT id, a[1] IS NULL, a[2][1], a[2][2], b[1] IS NULL, b[2][1][1], b[2][1][2]
  FROM upgrade_test.domain_array_test
  ORDER BY id;

INSERT INTO upgrade_test.domain_array_constraint_test DEFAULT VALUES;
SELECT id, a[1][1], a[1][2]
  FROM upgrade_test.domain_array_constraint_test
  WHERE id = 2;

DO $$
BEGIN
  IF COALESCE(
    pg_catalog.obj_description('topology.topoelement'::regtype::oid, 'pg_type'),
    ''
  ) LIKE '%postgis-topology-domain-storage-repaired-3.6.0%'
    OR COALESCE(
      pg_catalog.obj_description('topology.topoelement'::regtype::oid, 'pg_type'),
      ''
    ) LIKE '%postgis-topology-domain-storage-repaired-306%'
  THEN
    RAISE EXCEPTION 'topoelement repair marker was set despite skipped storage';
  END IF;

  IF COALESCE(
    pg_catalog.obj_description('topology.topoelementarray'::regtype::oid, 'pg_type'),
    ''
  ) LIKE '%postgis-topology-domain-storage-repaired-3.6.0%'
    OR COALESCE(
      pg_catalog.obj_description('topology.topoelementarray'::regtype::oid, 'pg_type'),
      ''
    ) LIKE '%postgis-topology-domain-storage-repaired-306%'
  THEN
    RAISE EXCEPTION 'topoelementarray repair marker was set despite skipped storage';
  END IF;
END
$$;

DO $$
BEGIN
  IF (SELECT count(*) FROM upgrade_test.domain_view_test) != 1 THEN
    RAISE EXCEPTION 'dependent topology domain view was not preserved during upgrade';
  END IF;

  IF (SELECT count(*) FROM upgrade_test.domain_whole_row_view_test) != 1 THEN
    RAISE EXCEPTION 'dependent topology domain whole-row view was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_policy
    WHERE polrelid = 'upgrade_test.domain_policy_source'::regclass
    AND polname = 'domain_policy_source_a_policy'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain row-level security policy was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_policy
    WHERE polrelid = 'upgrade_test.domain_whole_row_policy_source'::regclass
    AND polname = 'domain_whole_row_policy_source_policy'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain whole-row row-level security policy was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_trigger
    WHERE tgrelid = 'upgrade_test.domain_trigger_source'::regclass
    AND tgname = 'domain_trigger_source_a_trigger'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain trigger was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_trigger
    WHERE tgrelid = 'upgrade_test.domain_whole_row_trigger_source'::regclass
    AND tgname = 'domain_whole_row_trigger_source_trigger'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain whole-row trigger was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_proc
    WHERE pronamespace = 'upgrade_test'::regnamespace
    AND proname = 'domain_function_source_read'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain SQL function was not preserved during upgrade';
  END IF;

  IF current_setting('server_version_num')::integer >= 150000
    AND NOT EXISTS (
      SELECT 1
      FROM pg_catalog.pg_publication AS p
      JOIN pg_catalog.pg_publication_rel AS pr
        ON pr.prpubid = p.oid
      WHERE p.pubname = 'upgrade_test_domain_publication'
      AND pr.prrelid = 'upgrade_test.domain_publication_source'::regclass
    )
  THEN
    RAISE EXCEPTION 'dependent topology domain publication relation was not preserved during upgrade';
  END IF;

  IF (SELECT count(*) FROM upgrade_test.domain_blocked_partition_child_view) != 1 THEN
    RAISE EXCEPTION 'dependent topology domain partition-child view was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE conrelid = 'upgrade_test.domain_array_constraint_test'::regclass
    AND conname = 'domain_array_constraint_check'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain array constraint was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_class
    WHERE oid = 'upgrade_test.domain_array_constraint_expr_idx'::regclass
  ) THEN
    RAISE EXCEPTION 'dependent topology domain array expression index was not preserved during upgrade';
  END IF;

  IF EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE contypid IN (
      'topology.topoelement'::regtype,
      'topology.topoelementarray'::regtype
    )
    AND conname IN ('dimensions', 'type_range', 'lower_dimension')
    AND convalidated
  ) THEN
    RAISE EXCEPTION 'topology domain constraints with array-column dependencies were restored as validated';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_trigger
    WHERE tgrelid = 'upgrade_test.domain_trigger_child_child'::regclass
    AND tgname = 'domain_trigger_child_a_trigger'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain partition-child trigger was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_proc
    WHERE pronamespace = 'upgrade_test'::regnamespace
    AND proname = 'domain_function_child_read'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain partition-child SQL function was not preserved during upgrade';
  END IF;

  IF current_setting('server_version_num')::integer >= 150000
    AND NOT EXISTS (
      SELECT 1
      FROM pg_catalog.pg_publication AS p
      JOIN pg_catalog.pg_publication_rel AS pr
        ON pr.prpubid = p.oid
      WHERE p.pubname = 'upgrade_test_domain_child_publication'
      AND pr.prrelid = 'upgrade_test.domain_publication_child_child'::regclass
    )
  THEN
    RAISE EXCEPTION 'dependent topology domain partition-child publication relation was not preserved during upgrade';
  END IF;

  IF (SELECT count(*) FROM upgrade_test.domain_blocked_inherit_grandchild_view) != 1 THEN
    RAISE EXCEPTION 'dependent topology domain inherited grandchild view was not preserved during upgrade';
  END IF;
END
$$;

DO $$
DECLARE
  nested_value topology.topoelement;
  default_expr text;
  row_carrier_value topology.topoelement;
BEGIN
  IF (SELECT count(*) FROM upgrade_test.domain_nested_type_test) != 1 THEN
    RAISE EXCEPTION 'nested topology domain storage fixture was not preserved during upgrade';
  END IF;

  INSERT INTO upgrade_test.domain_nested_type_test DEFAULT VALUES;

  SELECT a::topology.topoelement INTO STRICT nested_value
    FROM upgrade_test.domain_nested_type_test
    WHERE id = 2;

  IF nested_value[1] != 89 OR nested_value[2] != 90 THEN
    RAISE EXCEPTION 'rewritten nested topology domain default did not preserve value: %',
      nested_value;
  END IF;

  SELECT pg_catalog.pg_get_expr(t.typdefaultbin, 0)
    INTO STRICT default_expr
    FROM pg_catalog.pg_type AS t
    WHERE t.oid = 'upgrade_test.nested_topoelement'::regtype;

  IF default_expr IS NULL OR default_expr NOT LIKE '%::text)::upgrade_test.nested_topoelement%' THEN
    RAISE EXCEPTION 'nested topology domain default was not rewritten during upgrade: %',
      default_expr;
  END IF;

  CREATE TEMP TABLE domain_nested_default_probe (
    a upgrade_test.nested_topoelement
  ) ON COMMIT DROP;
  INSERT INTO domain_nested_default_probe DEFAULT VALUES;
  SELECT a::topology.topoelement INTO STRICT nested_value
    FROM domain_nested_default_probe;

  IF nested_value[1] != 91 OR nested_value[2] != 92 THEN
    RAISE EXCEPTION 'rewritten nested topology domain-level default did not preserve value: %',
      nested_value;
  END IF;

  SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid)
    INTO STRICT default_expr
    FROM pg_catalog.pg_attribute AS a
    JOIN pg_catalog.pg_attrdef AS d
      ON d.adrelid = a.attrelid
      AND d.adnum = a.attnum
    WHERE a.attrelid = 'upgrade_test.domain_row_carrier_test'::regclass
    AND a.attname = 'r';

  IF default_expr IS NULL OR default_expr LIKE '%::text)::upgrade_test.domain_row_carrier_source%' THEN
    RAISE EXCEPTION 'table row-type topology carrier default used unsafe whole-row text rewrite: %',
      default_expr;
  END IF;

  INSERT INTO upgrade_test.domain_row_carrier_test DEFAULT VALUES;
  SELECT (r).a INTO STRICT row_carrier_value
    FROM upgrade_test.domain_row_carrier_test
    WHERE id = 2;

  IF row_carrier_value[1] != 105 OR row_carrier_value[2] != 106 THEN
    RAISE EXCEPTION 'rewritten table row-type topology carrier default did not preserve value: %',
      row_carrier_value;
  END IF;
END
$$;

DO $$
BEGIN
  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_rewrite
    WHERE ev_class = 'upgrade_test.domain_rule_source'::regclass
    AND rulename = 'domain_rule_source_insert'
  ) THEN
    RAISE EXCEPTION 'dependent topology domain rule was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_rewrite
    WHERE ev_class = 'upgrade_test.domain_external_rule_target'::regclass
    AND rulename = 'domain_external_rule_target_insert'
  ) THEN
    RAISE EXCEPTION 'external dependent topology domain rule was not preserved during upgrade';
  END IF;
END
$$;

DO $$
DECLARE
  default_value topology.topoelement[];
  default_element topology.topoelement;
BEGIN
  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_constraint
    WHERE conrelid = 'upgrade_test.domain_array_inherit_constraint_child'::regclass
    AND conname = 'domain_array_inherit_constraint_check'
  ) THEN
    RAISE EXCEPTION 'inherited child topology domain array constraint was not preserved during upgrade';
  END IF;

  IF NOT EXISTS (
    SELECT 1
    FROM pg_catalog.pg_class
    WHERE oid = 'upgrade_test.domain_array_inherit_constraint_expr_idx'::regclass
  ) THEN
    RAISE EXCEPTION 'inherited child topology domain array expression index was not preserved during upgrade';
  END IF;

  INSERT INTO upgrade_test.domain_array_inherit_constraint_parent(id) VALUES (2);

  SELECT a INTO STRICT default_value
    FROM ONLY upgrade_test.domain_array_inherit_constraint_parent
    WHERE id = 2;
  default_element := default_value[1]::topology.topoelement;

  IF default_element[1] != 101 OR default_element[2] != 102 THEN
    RAISE EXCEPTION 'rewritten inherited topology domain array default did not preserve value: %',
      default_value;
  END IF;
END
$$;

DO $$
DECLARE
  row RECORD;
  expected RECORD;
  dependent_generated_value topology.topoelement;
  cross_dependent_generated_value topology.topoelementarray;
  nested_dependent_generated_value topology.topoelement;
BEGIN
  IF current_setting('server_version_num')::integer >= 170000 THEN
    CREATE TEMP TABLE domain_generated_storage_probe (
      a topology.topoelement,
      b topology.topoelementarray
    ) ON COMMIT DROP;
    INSERT INTO domain_generated_storage_probe VALUES (
      ARRAY[15::bigint, 16::bigint]::topology.topoelement,
      ARRAY[ARRAY[17::bigint, 18::bigint]]::topology.topoelementarray
    );

    SELECT a, b INTO STRICT row
      FROM upgrade_test.domain_generated_test;

    SELECT pg_column_size(a) AS a_size, pg_column_size(b) AS b_size
      INTO STRICT expected
      FROM domain_generated_storage_probe;

    IF row.a[1] != 15 OR row.a[2] != 16 THEN
      RAISE EXCEPTION 'generated topoelement storage was not rewritten during upgrade: %', row.a;
    END IF;

    IF pg_column_size(row.a) != expected.a_size THEN
      RAISE EXCEPTION 'generated topoelement storage size was not rewritten during upgrade: %', row.a;
    END IF;

    IF row.b[1][1] != 17 OR row.b[1][2] != 18 THEN
      RAISE EXCEPTION 'generated topoelementarray storage was not rewritten during upgrade: %', row.b;
    END IF;

    IF pg_column_size(row.b) != expected.b_size THEN
      RAISE EXCEPTION 'generated topoelementarray storage size was not rewritten during upgrade: %', row.b;
    END IF;

    SELECT g INTO STRICT dependent_generated_value
      FROM upgrade_test.domain_generated_source_dependency_test;

    IF dependent_generated_value[1] != 39 OR dependent_generated_value[2] != 40 THEN
      RAISE EXCEPTION 'generated topology domain source dependency was not preserved during upgrade: %',
        dependent_generated_value;
    END IF;

    SELECT g INTO STRICT cross_dependent_generated_value
      FROM upgrade_test.domain_generated_cross_dependency_test;

    IF cross_dependent_generated_value[1][1] != 43
      OR cross_dependent_generated_value[1][2] != 44
    THEN
      RAISE EXCEPTION 'generated topology domain cross dependency was not preserved during upgrade: %',
        cross_dependent_generated_value;
    END IF;

    SELECT g INTO STRICT nested_dependent_generated_value
      FROM upgrade_test.domain_generated_nested_source_dependency_test;

    IF nested_dependent_generated_value[1] != 97
      OR nested_dependent_generated_value[2] != 98
    THEN
      RAISE EXCEPTION 'generated nested topology domain source dependency was not preserved during upgrade: %',
        nested_dependent_generated_value;
    END IF;
  END IF;
END
$$;

DO $$
DECLARE
  default_expr text;
BEGIN
  IF (SELECT generated_id FROM upgrade_test.domain_generated_dependency_test) != 37 THEN
    RAISE EXCEPTION 'generated topology domain dependency was not preserved during upgrade';
  END IF;

  SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid)
    INTO STRICT default_expr
    FROM pg_catalog.pg_attribute AS a
    JOIN pg_catalog.pg_attrdef AS d
      ON d.adrelid = a.attrelid
      AND d.adnum = a.attnum
    WHERE a.attrelid = 'upgrade_test.domain_generated_dependency_test'::regclass
    AND a.attname = 'a';

  IF default_expr NOT LIKE '%::text)::bigint[]%' THEN
    RAISE EXCEPTION 'skipped topology domain column default was not rewritten during upgrade: %',
      default_expr;
  END IF;

  INSERT INTO upgrade_test.domain_generated_dependency_test(id) VALUES (2);
  IF (
    SELECT generated_id
    FROM upgrade_test.domain_generated_dependency_test
    WHERE id = 2
  ) != 41 THEN
    RAISE EXCEPTION 'rewritten skipped topology domain default did not preserve generated dependency value';
  END IF;

  SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid)
    INTO STRICT default_expr
    FROM pg_catalog.pg_attribute AS a
    JOIN pg_catalog.pg_attrdef AS d
      ON d.adrelid = a.attrelid
      AND d.adnum = a.attnum
    WHERE a.attrelid = 'upgrade_test.domain_policy_source'::regclass
    AND a.attname = 'a';

  IF default_expr NOT LIKE '%::text)::bigint[]%' THEN
    RAISE EXCEPTION 'RLS-skipped topology domain column default was not rewritten during upgrade: %',
      default_expr;
  END IF;

  INSERT INTO upgrade_test.domain_policy_source(id) VALUES (2);
  IF (
    SELECT a[1]
    FROM upgrade_test.domain_policy_source
    WHERE id = 2
  ) != 45 THEN
    RAISE EXCEPTION 'rewritten RLS-skipped topology domain default did not preserve value';
  END IF;

  SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid)
    INTO STRICT default_expr
    FROM pg_catalog.pg_attribute AS a
    JOIN pg_catalog.pg_attrdef AS d
      ON d.adrelid = a.attrelid
      AND d.adnum = a.attnum
    WHERE a.attrelid = 'upgrade_test.domain_whole_row_policy_source'::regclass
    AND a.attname = 'a';

  IF default_expr NOT LIKE '%::text)::bigint[]%' THEN
    RAISE EXCEPTION 'whole-row RLS-skipped topology domain column default was not rewritten during upgrade: %',
      default_expr;
  END IF;

  INSERT INTO upgrade_test.domain_whole_row_policy_source(id) VALUES (2);
  IF (
    SELECT a[1]
    FROM upgrade_test.domain_whole_row_policy_source
    WHERE id = 2
  ) != 81 THEN
    RAISE EXCEPTION 'rewritten whole-row RLS-skipped topology domain default did not preserve value';
  END IF;

  SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid)
    INTO STRICT default_expr
    FROM pg_catalog.pg_attribute AS a
    JOIN pg_catalog.pg_attrdef AS d
      ON d.adrelid = a.attrelid
      AND d.adnum = a.attnum
    WHERE a.attrelid = 'upgrade_test.domain_trigger_source'::regclass
    AND a.attname = 'a';

  IF default_expr NOT LIKE '%::text)::bigint[]%' THEN
    RAISE EXCEPTION 'trigger-skipped topology domain column default was not rewritten during upgrade: %',
      default_expr;
  END IF;

  INSERT INTO upgrade_test.domain_trigger_source(id) VALUES (2);
  IF (
    SELECT a[1]
    FROM upgrade_test.domain_trigger_source
    WHERE id = 2
  ) != 49 THEN
    RAISE EXCEPTION 'rewritten trigger-skipped topology domain default did not preserve value';
  END IF;

  SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid)
    INTO STRICT default_expr
    FROM pg_catalog.pg_attribute AS a
    JOIN pg_catalog.pg_attrdef AS d
      ON d.adrelid = a.attrelid
      AND d.adnum = a.attnum
    WHERE a.attrelid = 'upgrade_test.domain_function_source'::regclass
    AND a.attname = 'a';

  IF default_expr NOT LIKE '%::text)::bigint[]%' THEN
    RAISE EXCEPTION 'function-skipped topology domain column default was not rewritten during upgrade: %',
      default_expr;
  END IF;

  INSERT INTO upgrade_test.domain_function_source(id) VALUES (2);
  IF (
    SELECT a[1]
    FROM upgrade_test.domain_function_source
    WHERE id = 2
  ) != 61 THEN
    RAISE EXCEPTION 'rewritten function-skipped topology domain default did not preserve value';
  END IF;

  IF current_setting('server_version_num')::integer >= 150000 THEN
    SELECT pg_catalog.pg_get_expr(d.adbin, d.adrelid)
      INTO STRICT default_expr
      FROM pg_catalog.pg_attribute AS a
      JOIN pg_catalog.pg_attrdef AS d
        ON d.adrelid = a.attrelid
        AND d.adnum = a.attnum
      WHERE a.attrelid = 'upgrade_test.domain_publication_source'::regclass
      AND a.attname = 'a';

    IF default_expr NOT LIKE '%::text)::bigint[]%' THEN
      RAISE EXCEPTION 'publication-skipped topology domain column default was not rewritten during upgrade: %',
        default_expr;
    END IF;

    INSERT INTO upgrade_test.domain_publication_source(id) VALUES (2);
    IF (
      SELECT a[1]
      FROM upgrade_test.domain_publication_source
      WHERE id = 2
    ) != 53 THEN
      RAISE EXCEPTION 'rewritten publication-skipped topology domain default did not preserve value';
    END IF;
  END IF;
END
$$;

INSERT INTO upgrade_test.domain_test values (
  '{1,2}'::topology.topoelement,
  '{{2,3}}'::topology.topoelementarray
);

SELECT topology.DropTopology('upgrade_test');
SELECT topology.DropTopology('upgrade_test_copy');
