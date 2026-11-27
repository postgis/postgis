-- EMPTY and NaN geometries in multi-page SP-GiST indexes.
--
-- For every operator and query geometry the ids found through the index
-- must be the ids found by a sequential scan. The tables are big enough
-- that the indexes have inner pages. Output:
--   index | operator | query | index result vs seq scan | rows found

CREATE OR REPLACE FUNCTION idx_vs_seq(tbl text, op text, q text, idxname text)
RETURNS text LANGUAGE plpgsql AS
$$
DECLARE
	sql text := format('SELECT coalesce(array_agg(id ORDER BY id), ARRAY[]::int[]) FROM %I WHERE g %s %L::geometry', tbl, op, q);
	plan text;
	used_idx boolean := false;
	via_idx int[];
	via_seq int[];
BEGIN
	PERFORM set_config('enable_seqscan', 'off', true);
	PERFORM set_config('enable_bitmapscan', 'off', true);
	FOR plan IN EXECUTE 'EXPLAIN ' || sql LOOP
		IF position(idxname IN plan) > 0 THEN used_idx := true; END IF;
	END LOOP;
	EXECUTE sql INTO via_idx;
	PERFORM set_config('enable_seqscan', 'on', true);
	PERFORM set_config('enable_indexscan', 'off', true);
	EXECUTE sql INTO via_seq;
	PERFORM set_config('enable_indexscan', 'on', true);
	RETURN format('%s|%s|%s', CASE WHEN NOT used_idx THEN 'NO INDEX SCAN'
	                               WHEN via_idx = via_seq THEN 'same'
	                               ELSE 'DIFFERENT' END,
	              coalesce(array_length(via_idx, 1), 0), coalesce(array_length(via_seq, 1), 0));
END;
$$;

-- Same, for a real query and an EMPTY query in one scan
CREATE OR REPLACE FUNCTION idx_vs_seq2(tbl text, op text, idxname text)
RETURNS text LANGUAGE plpgsql AS
$$
DECLARE
	sql text := format('SELECT count(*) FROM %I WHERE g %s %L::geometry AND g %s %L::geometry', tbl, op,
	                   'POLYGON((0 0, 0 50, 50 50, 50 0, 0 0))', op, 'POINT EMPTY');
	via_idx bigint;
	via_seq bigint;
BEGIN
	PERFORM set_config('enable_seqscan', 'off', true);
	PERFORM set_config('enable_bitmapscan', 'off', true);
	EXECUTE sql INTO via_idx;
	PERFORM set_config('enable_seqscan', 'on', true);
	PERFORM set_config('enable_indexscan', 'off', true);
	EXECUTE sql INTO via_seq;
	PERFORM set_config('enable_indexscan', 'on', true);
	RETURN format('%s|%s|%s', CASE WHEN via_idx = via_seq THEN 'same' ELSE 'DIFFERENT' END, via_idx, via_seq);
END;
$$;

-- 3600 points on a grid, 300 EMPTY and 180 NaN rows (both, X only, Y only) mixed in
CREATE TABLE tbl_empty_idx AS
SELECT i AS id,
	CASE WHEN i % 12 = 0 THEN 'POINT EMPTY'::geometry
	     WHEN i % 60 = 7 THEN ST_MakePoint('NaN', 'NaN')
	     WHEN i % 60 = 11 THEN ST_MakePoint(i % 60, 'NaN')
	     WHEN i % 60 = 13 THEN ST_MakePoint('NaN', i / 60)
	     ELSE ST_MakePoint(i % 60, i / 60, i % 7) END AS g
FROM generate_series(1, 3600) i;
ANALYZE tbl_empty_idx;

-- Only EMPTY rows, and a mix of EMPTY with few real rows
CREATE TABLE tbl_empty_only AS
SELECT i AS id, CASE WHEN i % 500 = 1 THEN ST_MakePoint(i, i, i) ELSE 'POINT EMPTY'::geometry END AS g
FROM generate_series(1, 2000) i;
ANALYZE tbl_empty_only;

CREATE TEMP TABLE queries (qid int, q text);
INSERT INTO queries VALUES
	(1, 'POLYGON((10 10, 10 20, 30 20, 30 10, 10 10))'),
	(2, 'POLYGON((-100 -100, -100 100, 100 100, 100 -100, -100 -100))'),
	(3, 'POLYGON((1000 1000, 1000 1001, 1001 1001, 1001 1000, 1000 1000))'),
	(4, 'POINT(15 15)'),
	(5, 'POINT EMPTY'),
	(6, 'POINT(NaN NaN)'),
	(7, 'POINT(15 NaN)'),
	(8, 'POINT(NaN 15)');

-- SP-GiST 2D, all operators
CREATE INDEX tbl_empty_idx_spg2d ON tbl_empty_idx USING spgist (g spgist_geometry_ops_2d);
CREATE INDEX tbl_empty_only_spg2d ON tbl_empty_only USING spgist (g spgist_geometry_ops_2d);

SELECT 'spgist 2d', op, qid, idx_vs_seq('tbl_empty_idx', op, q, 'tbl_empty_idx_spg2d')
FROM (VALUES ('&&'), ('@'), ('~'), ('~='), ('<<'), ('>>'), ('<<|'), ('|>>'), ('&<'), ('&>'), ('&<|'), ('|&>')) o(op),
     queries
ORDER BY op, qid;

SELECT 'spgist 2d only empty', op, qid, idx_vs_seq('tbl_empty_only', op, q, 'tbl_empty_only_spg2d')
FROM (VALUES ('&&'), ('@'), ('~'), ('~=')) o(op), queries
ORDER BY op, qid;

-- Rows added after the index was built
INSERT INTO tbl_empty_idx
SELECT 10000 + i, CASE WHEN i % 3 = 0 THEN 'POINT EMPTY'::geometry ELSE ST_MakePoint(i % 60, 70 + i / 60) END
FROM generate_series(1, 600) i;

SELECT 'spgist 2d insert', op, qid, idx_vs_seq('tbl_empty_idx', op, q, 'tbl_empty_idx_spg2d')
FROM (VALUES ('&&'), ('@'), ('~'), ('~=')) o(op), queries
ORDER BY op, qid;

DROP INDEX tbl_empty_idx_spg2d;
DROP INDEX tbl_empty_only_spg2d;

-- An EMPTY query next to a real one, on an index that is a single leaf
-- page, so that the leaf test sees both: nothing matches an EMPTY query.
CREATE TABLE tbl_empty_small AS
SELECT i AS id, ST_MakePoint(i, i) AS g FROM generate_series(1, 20) i;
CREATE INDEX tbl_empty_small_spgnd ON tbl_empty_small USING spgist (g spgist_geometry_ops_nd);
SELECT 'spgist nd two keys', op, idx_vs_seq2('tbl_empty_small', op, 'tbl_empty_small_spgnd')
FROM (VALUES ('&&&'), ('@@'), ('~~'), ('~~=')) o(op)
ORDER BY op;
DROP TABLE tbl_empty_small;

-- Keys of different dimensionality in one index: XY and XYZM rows
CREATE TABLE tbl_empty_mixed AS
SELECT i AS id, CASE WHEN i % 2 = 0 THEN ST_MakePoint(i % 100, i / 100)
                     ELSE ST_MakePoint(i % 100, i / 100, i % 7, i % 5) END AS g
FROM generate_series(1, 20000) i;
ANALYZE tbl_empty_mixed;
CREATE INDEX tbl_empty_mixed_spgnd ON tbl_empty_mixed USING spgist (g spgist_geometry_ops_nd);
INSERT INTO queries VALUES (9, 'LINESTRING(10 10, 20 20)'), (10, 'LINESTRING Z (10 10 1, 20 20 3)');
SELECT 'spgist nd mixed dims', op, qid, idx_vs_seq('tbl_empty_mixed', op, q, 'tbl_empty_mixed_spgnd')
FROM (VALUES ('&&&'), ('@@'), ('~~'), ('~~=')) o(op), queries WHERE qid IN (1, 2, 3, 4, 9, 10)
ORDER BY op, qid;
DROP TABLE tbl_empty_mixed;

DROP TABLE tbl_empty_idx;
DROP TABLE tbl_empty_only;
-- Mix EMPTY and NaN with genuine geometries at the BOX2DF limits.
-- The index must recheck entries that share the clamped key shape.
CREATE TABLE tbl_legacy_idx AS
SELECT i AS id,
	CASE WHEN i % 40 = 0 THEN 'POINT EMPTY'::geometry
	     WHEN i % 40 = 1 THEN ST_MakePoint('NaN', 'NaN')
	     WHEN i % 40 = 2 THEN ST_MakePoint('NaN', i % 50)
	     WHEN i % 40 = 3 THEN ST_MakePoint(i % 50, 'NaN')
	     WHEN i % 80 = 4 THEN ST_MakeEnvelope(-1e39, -1e39, 1e39, 1e39)
	     WHEN i % 80 = 5 THEN ST_MakeEnvelope(-1e39, 0, 30, 30)
	     WHEN i % 80 = 6 THEN ST_MakeEnvelope(0, 0, 1e39, 30)
	     WHEN i % 80 = 7 THEN ST_MakePoint('Infinity', i % 50)
	     WHEN i % 80 = 8 THEN ST_MakePoint(-1e39, -1e39)
	     WHEN i % 5 = 0 THEN ST_MakeEnvelope(i % 50, i % 47, i % 50 + 1 + i % 7, i % 47 + 1 + i % 5)
	     ELSE ST_MakePoint(i % 50, i % 47) END AS g
FROM generate_series(1, 8000) i;
ANALYZE tbl_legacy_idx;
CREATE INDEX tbl_legacy_idx_spg2d ON tbl_legacy_idx USING spgist (g spgist_geometry_ops_2d);

INSERT INTO queries VALUES
	(11, ST_MakeEnvelope(-1e39, -1e39, 1e39, 1e39)::text),
	(12, 'POLYGON((0 0, 0 5, 5 5, 5 0, 0 0))'),
	(13, ST_MakePoint('Infinity', 5)::text),
	-- the largest box that is not infinite: its float box is exactly the clamp value
	(14, ST_MakeEnvelope(-3.4028235e38, -3.4028235e38, 3.4028235e38, 3.4028235e38)::text);

SELECT 'spgist 2d clamped keys', op, qid, idx_vs_seq('tbl_legacy_idx', op, q, 'tbl_legacy_idx_spg2d')
FROM (VALUES ('&&'), ('@'), ('~'), ('~='), ('<<'), ('>>'), ('<<|'), ('|>>'), ('&<'), ('&>'), ('&<|'), ('|&>')) o(op),
     queries WHERE qid IN (1, 2, 4, 5, 6, 7, 8, 11, 12, 13, 14)
ORDER BY op, qid;

DROP TABLE tbl_legacy_idx;

DROP FUNCTION idx_vs_seq(text, text, text, text);
DROP FUNCTION idx_vs_seq2(text, text, text);
