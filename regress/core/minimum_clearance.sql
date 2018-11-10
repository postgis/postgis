SELECT 't1', ST_MinimumClearance(NULL::geometry) IS NULL;
SELECT 't2', ST_MinimumClearanceLine(NULL::geometry) IS NULL;
SELECT 't3', ST_MinimumClearance('POINT (0 0)') = 'Infinity';
SELECT 't4', ST_Equals(ST_MinimumClearanceLine('POINT (0 0)'), 'LINESTRING EMPTY');
SELECT 't5', ST_MinimumClearance('POLYGON ((0 0, 1 0, 1 1, 0.5 3.2e-4, 0 0))')=0.00032;
SELECT 't6', ST_Equals(ST_MinimumClearanceLine('POLYGON ((0 0, 1 0, 1 1, 0.5 3.2e-4, 0 0))'), 'LINESTRING (0.5 0.00032, 0.5 0)');
SELECT 't7', ST_MinimumClearance('POLYGON EMPTY') = 'Infinity';
SELECT 't8', ST_MinimumClearanceLine('POLYGON EMPTY') = 'LINESTRING EMPTY';
SELECT 't9', ST_SRID(ST_MinimumClearanceLine('SRID=3857;LINESTRING (0 0, 1 1)'))=3857;
