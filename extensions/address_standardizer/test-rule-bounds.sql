\t on
\pset format unaligned
\set ECHO none
\set QUIET on

BEGIN;
\i :top_builddir/regress/00-regress-install/share/contrib/postgis/address_standardizer.sql
SET client_min_messages = warning;

CREATE TEMP TABLE issue010_lex (
  id integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  seq integer,
  word text,
  stdword text,
  token integer
);
CREATE TEMP TABLE issue010_gaz (LIKE issue010_lex INCLUDING ALL);

CREATE TEMP TABLE issue010_rules_126 (
  id integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  rule text NOT NULL
);
CREATE TEMP TABLE issue010_rules_127 (LIKE issue010_rules_126 INCLUDING ALL);
CREATE TEMP TABLE issue010_rules_128 (LIKE issue010_rules_126 INCLUDING ALL);
CREATE TEMP TABLE issue010_rules_129 (LIKE issue010_rules_126 INCLUDING ALL);

INSERT INTO issue010_rules_126(rule)
SELECT repeat('1 ', 61) || '-1 ' || repeat('5 ', 61) || '-1 2 7';
INSERT INTO issue010_rules_127(rule)
SELECT repeat('1 ', 61) || '-1 ' || repeat('5 ', 61) || '-1 2 7 0';
INSERT INTO issue010_rules_128(rule)
SELECT repeat('1 ', 62) || '-1 ' || repeat('5 ', 62) || '-1 2 7';
INSERT INTO issue010_rules_129(rule)
SELECT repeat('1 ', 62) || '-1 ' || repeat('5 ', 62) || '-1 2 7 305419896';

CREATE FUNCTION pg_temp.issue010_rule_result(rules_table text)
RETURNS text
LANGUAGE plpgsql
AS $$
BEGIN
  PERFORM public.standardize_address(
    'issue010_lex',
    'issue010_gaz',
    rules_table,
    '1 main',
    'issue010 rule bounds'
  );
  RETURN 'accepted';
EXCEPTION
  WHEN SQLSTATE 'XX000' THEN
    IF SQLERRM = format('CreateStd: failed to load ''%s'' for rules', rules_table) THEN
      RETURN 'controlled rejection';
    END IF;
    RAISE;
END;
$$;

\set QUIET off

SELECT 126 AS terms,
       pg_temp.issue010_rule_result('issue010_rules_126') AS result;
SELECT 127 AS terms,
       pg_temp.issue010_rule_result('issue010_rules_127')
         IN ('accepted', 'controlled rejection') AS safe_result;
SELECT 128 AS terms,
       pg_temp.issue010_rule_result('issue010_rules_128') AS result;
SELECT 129 AS terms,
       pg_temp.issue010_rule_result('issue010_rules_129') AS result;

\set QUIET on
ROLLBACK;
\set QUIET off

SELECT 'backend_alive' AS result;
