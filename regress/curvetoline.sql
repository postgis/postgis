
-- Semantic of tolerance depends on the `toltype` argument, which can be:
--    0: Tolerance is number of segments per quadrant
--    1: Tolerance is max distance between curve and line
--    2: Tolerance is max angle between radii defining line vertices
--
-- Supported flags:
--    1: Symmetric output (result in same vertices when inverting the curve)

SELECT 'semicircle1', ST_AsText(ST_SnapToGrid(ST_CurveToLine(
 'CIRCULARSTRING(0 0,100 -100,200 0)'::geometry,
	3, -- Tolerance
	0, -- Above is number of segments per quadrant
	0  -- no flags
), 2));

SELECT 'semicircle2', ST_AsText(ST_SnapToGrid(ST_CurveToLine(
 'CIRCULARSTRING(0 0,100 -100,200 0)'::geometry,
	20, -- Tolerance
	1, -- Above is max distance between curve and line
	0  -- no flags
), 2));

SELECT 'semicircle2.sym', ST_AsText(ST_SnapToGrid(ST_CurveToLine(
 'CIRCULARSTRING(0 0,100 -100,200 0)'::geometry,
	20, -- Tolerance
	1, -- Above is max distance between curve and line
	1  -- Symmetric flag
), 2));

SELECT 'semicircle3', ST_AsText(ST_SnapToGrid(ST_CurveToLine(
 'CIRCULARSTRING(0 0,100 -100,200 0)'::geometry,
	radians(40), -- Tolerance
	2, -- Above is max angle between generating radii
	0  -- no flags
), 2));

SELECT 'semicircle3.sym', ST_AsText(ST_SnapToGrid(ST_CurveToLine(
 'CIRCULARSTRING(0 0,100 -100,200 0)'::geometry,
	radians(40), -- Tolerance
	2, -- Above is max angle between generating radii
	1  -- Symmetric flag
), 2));
SELECT 'semicircle3.sym.ret', ST_AsText(ST_SnapToGrid(ST_CurveToLine(
 'CIRCULARSTRING(0 0,100 -100,200 0)'::geometry,
	radians(40), -- Tolerance
	2, -- Above is max angle between generating radii
	3  -- Symmetric and RetainAngle flags
), 2));
SELECT 'multiarc1', ST_AsText(ST_SnapToGrid(ST_CurveToLine(
 'CIRCULARSTRING(0 0,100 -100,200 0,400 200,600 0)'::geometry,
	radians(45), -- Tolerance
	2, -- Above is max angle between generating radii
	3  -- Symmetric and RetainAngle flags
), 2));
SELECT 'multiarc1.maxerr20.sym', ST_AsText(ST_SnapToGrid(ST_CurveToLine(
 'CIRCULARSTRING(0 0,100 -100,200 0,400 200,600 0)'::geometry,
	20, -- Tolerance
	1, -- Above is max distance between curve and line
	1  -- Symmetric
), 2));

