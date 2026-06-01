-- postgis_hex9 1.0.0 → 1.1.0 upgrade
--
-- Breaking changes:
--   * h9_grid argument order changed from (layer, bounds) to
--     (bounds, layer, [densify]). PostGIS convention is geometry-first;
--     this aligns h9_grid with ST_Within, ST_Intersects, etc.
--   * h9_grid output gains a third column `centroid` (POINT, SRID 4326) —
--     the cell's geographic centre, already computed by the BFS.
--   * h9_cell gains an optional `densify` arg (default 0).
--     densify > 0 produces a 6·3^densify + 1-point ring per cell, useful
--     for rendering large hexes (layer ≤ 3) without straight-line chord
--     artefacts. The 1.0.0 two-arg form continues to work via SQL DEFAULT 0.
--
-- Run via:
--   ALTER EXTENSION postgis_hex9 UPDATE TO '1.1.0';

\echo Use "ALTER EXTENSION postgis_hex9 UPDATE TO '1.1.0'" to load this file. \quit

-- Drop the old signatures. CASCADE is intentionally NOT used; if a user
-- has views/indexes depending on them, the upgrade aborts and they must
-- adjust their schemas first.
DROP FUNCTION IF EXISTS h9_cell(uuid, integer);
DROP FUNCTION IF EXISTS h9_grid(integer, geometry);

-- Re-declare with the new signatures (identical body to postgis_hex9--1.1.0.sql).
-- The C symbols are unchanged — only the SQL signatures move.

CREATE OR REPLACE FUNCTION h9_cell(uuid, integer, integer DEFAULT 0)
    RETURNS geometry
    AS 'MODULE_PATHNAME', 'h9_cell'
    LANGUAGE 'c' IMMUTABLE STRICT PARALLEL SAFE
    COST 200;

CREATE OR REPLACE FUNCTION h9_grid(geometry, integer, integer DEFAULT 0)
    RETURNS TABLE(hex9 uuid, geom geometry, centroid geometry)
    AS 'MODULE_PATHNAME', 'h9_grid'
    LANGUAGE 'c' STABLE STRICT PARALLEL SAFE
    ROWS 1000 COST 5000;
