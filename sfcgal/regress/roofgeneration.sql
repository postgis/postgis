-- Tests for CG_GenerateFlatRoof, CG_GenerateHippedRoof, CG_GenerateGableRoof,
-- CG_GenerateSkillionRoof, CG_GenerateRoof
-- Uses square polygon with integer coordinates to ensure exact WKT for flat/hipped.
-- Gable/skillion involve trigonometric values so only dimensionality is checked.

SELECT 'flat', ST_AsText(CG_GenerateFlatRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 2.0));
SELECT 'hipped', ST_AsText(CG_GenerateHippedRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 2.0));
SELECT 'flat_3d', ST_CoordDim(CG_GenerateFlatRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 2.0));
SELECT 'hipped_3d', ST_CoordDim(CG_GenerateHippedRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 2.0));
SELECT 'gable_3d', ST_CoordDim(CG_GenerateGableRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 2.0, 30.0));
SELECT 'skillion_3d', ST_CoordDim(CG_GenerateSkillionRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 2.0, 30.0, 0));
SELECT 'generic_hipped', ST_AsText(CG_GenerateRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 'HIPPED', 2.0, 30.0, 0));
SELECT 'generic_flat', ST_AsText(CG_GenerateRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 'FLAT', 2.0, 30.0, 0));
SELECT 'generic_gable_3d', ST_CoordDim(CG_GenerateRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 'GABLE', 2.0, 30.0, 0));
SELECT 'generic_skillion_3d', ST_CoordDim(CG_GenerateRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 'SKILLION', 2.0, 30.0, 0));
SELECT 'generic_gable_matches_specialized',
  ST_AsText(CG_GenerateRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 'GABLE', 5.0, 15.0, 0)) =
  ST_AsText(CG_GenerateGableRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 5.0, 15.0));
SELECT 'generic_skillion_matches_specialized',
  ST_AsText(CG_GenerateRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 'SKILLION', 5.0, 15.0, 0)) =
  ST_AsText(CG_GenerateSkillionRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 5.0, 15.0, 0));
SELECT 'invalid_type', CG_GenerateRoof('POLYGON((0 0,4 0,4 4,0 4,0 0))', 'DOME', 2.0, 30.0, 0);
