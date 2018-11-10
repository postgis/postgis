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
-- Meeting
select 'cpa4', ST_ClosestPointOfApproach(
  'LINESTRINGZM(0 0 0 0, 0 0 0 10)',
  'LINESTRINGZM(0 5 0 10, 10 0 5 11)');
-- Disjoint
select 'cpa5', ST_ClosestPointOfApproach(
  'LINESTRINGM(0 0 0, 0 0 4)',
  'LINESTRINGM(0 0 5, 0 0 10)');
-- Ticket #3136
WITH inp as ( SELECT
 'LINESTRING M (0 0 80000002,1 0 80000003)'::geometry g1,
 'LINESTRING M (2 2 80000000,1 1 80000001,0 0 80000002)'::geometry g2 )
SELECT 'cpa#3136',
ST_ClosestPointOfApproach(g2,g1), ST_ClosestPointOfApproach(g1,g2)
FROM inp;

----------------------------------------
--
-- ST_DistanceCPA
--
----------------------------------------

SELECT 'cpad1', ST_DistanceCPA('LINESTRINGM(0 0 0, 1 0 1)'::geometry
          ,'LINESTRINGM(0 0 0, 1 0 1)'::geometry);
SELECT 'cpad2', ST_DistanceCPA('LINESTRINGM(0 0 0, 1 0 1)'::geometry
          ,'LINESTRINGM(0 0 1, 1 0 2)'::geometry);
SELECT 'cpad3', ST_DistanceCPA('LINESTRING(0 0 0 0, 1 0 0 1)'::geometry
          ,'LINESTRING(0 0 3 0, 1 0 2 1)'::geometry);
-- Disjoint
SELECT 'cpad4', ST_DistanceCPA('LINESTRINGM(0 0 0, 10 0 10)'::geometry
          ,'LINESTRINGM(10 0 11, 10 10 20)'::geometry);
SELECT 'invalid', ST_DistanceCPA('LINESTRING(0 0 0, 1 0 0)'::geometry
            ,'LINESTRING(0 0 3 0, 1 0 2 1)'::geometry);

----------------------------------------
--
-- ST_CPAWithin
--
----------------------------------------

SELECT 'cpaw1', d, ST_CPAWithin(
       'LINESTRINGM(0 0 0, 1 0 1)'::geometry
      ,'LINESTRINGM(0 0 0, 1 0 1)'::geometry, d)
      FROM ( VALUES (0.0),(1) ) f(d) ORDER BY d;
SELECT 'cpaw2', d, ST_CPAWithin(
       'LINESTRINGM(0 0 0, 1 0 1)'::geometry
      ,'LINESTRINGM(0 0 1, 1 0 2)'::geometry, d)
      FROM ( VALUES (0.5),(1),(1.2) ) f(d) ORDER BY d;
SELECT 'cpaw3', d, ST_CPAWithin(
       'LINESTRING(0 0 0 0, 1 0 0 1)'::geometry
      ,'LINESTRING(0 0 3 0, 1 0 2 1)'::geometry, d)
      FROM ( VALUES (1.9),(2),(2.0001) ) f(d) ORDER BY d;
-- temporary disjoint
SELECT 'cpaw4', ST_CPAWithin(
       'LINESTRINGM(0 0 0, 10 0 10)'::geometry
      ,'LINESTRINGM(10 0 11, 10 10 20)'::geometry, 1e15);
SELECT 'cpaw_invalid', ST_CPAWithin(
       'LINESTRING(0 0 0, 1 0 0)'::geometry
      ,'LINESTRING(0 0 3 0, 1 0 2 1)'::geometry, 1e16);
