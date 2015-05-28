SELECT 'invalidTrajectory1', ST_IsValidTrajectory('POINTM(0 0 0)'::geometry);
SELECT 'invalidTrajectory2', ST_IsValidTrajectory('LINESTRINGZ(0 0 0,1 1 1)'::geometry);
SELECT 'invalidTrajectory3', ST_IsValidTrajectory('LINESTRINGM(0 0 0,1 1 0)'::geometry);
SELECT 'invalidTrajectory4', ST_IsValidTrajectory('LINESTRINGM(0 0 1,1 1 0)'::geometry);
SELECT 'invalidTrajectory8', ST_IsValidTrajectory('LINESTRINGM(0 0 0,1 1 1,1 1 2,1 0 1)'::geometry);

SELECT 'validTrajectory1', ST_IsValidTrajectory('LINESTRINGM(0 0 0,1 1 1)'::geometry);
SELECT 'validTrajectory2', ST_IsValidTrajectory('LINESTRINGM EMPTY'::geometry);
SELECT 'validTrajectory3', ST_IsValidTrajectory('LINESTRINGM(0 0 0,1 1 1,1 1 2)'::geometry);
