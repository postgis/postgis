----------------------------------------
--
-- ST_IsValidTrajectory
--
----------------------------------------

SELECT 'invalidTrajectory1', ST_IsValidTrajectory('POINTM(0 0 0)'::geometry);
SELECT 'invalidTrajectory2', ST_IsValidTrajectory('LINESTRINGZ(0 0 0,1 1 1)'::geometry);
SELECT 'invalidTrajectory3', ST_IsValidTrajectory('LINESTRINGM(0 0 0,1 1 0)'::geometry);
SELECT 'invalidTrajectory4', ST_IsValidTrajectory('LINESTRINGM(0 0 1,1 1 0)'::geometry);
SELECT 'invalidTrajectory8', ST_IsValidTrajectory('LINESTRINGM(0 0 0,1 1 1,1 1 2,1 0 1)'::geometry);

SELECT 'validTrajectory1', ST_IsValidTrajectory('LINESTRINGM(0 0 0,1 1 1)'::geometry);
SELECT 'validTrajectory2', ST_IsValidTrajectory('LINESTRINGM EMPTY'::geometry);
SELECT 'validTrajectory3', ST_IsValidTrajectory('LINESTRINGM(0 0 0,1 1 1,1 1 2)'::geometry);

----------------------------------------
--
-- ST_ClosestPointOfApproach
--
----------------------------------------

-- Converging
select 'cpa1', ST_ClosestPointOfApproach(
  'LINESTRINGZM(0 0 0 0, 10 10 10 10)',
  'LINESTRINGZM(0 0 0 1, 10 10 10 10)');
-- Following
select 'cpa2', ST_ClosestPointOfApproach(
  'LINESTRINGZM(0 0 0 0, 10 10 10 10)',
  'LINESTRINGZM(0 0 0 5, 10 10 10 15)');
-- Crossing
select 'cpa3', ST_ClosestPointOfApproach(
  'LINESTRINGZM(0 0 0 0, 0 0 0 10)',
  'LINESTRINGZM(-30 0 5 4, 10 0 5 6)');
-- Ticket #3136
WITH inp as ( SELECT
 'LINESTRING M (0 0 80000002,1 0 80000003)'::geometry g1,
 'LINESTRING M (2 2 80000000,1 1 80000001,0 0 80000002)'::geometry g2 )
SELECT 'cpa#3136',
ST_ClosestPointOfApproach(g2,g1), ST_ClosestPointOfApproach(g1,g2)
FROM inp;
