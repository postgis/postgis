SELECT 0, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('MULTIPOLYGON(((10 10, 20 10, 30 10, 40 10, 20 20, 10 20, 10 10)),((10 10, 20 10, 20 20, 10 20, 10 10)))'),
	ST_MakeEnvelope(12,12,18,18), true));

SELECT 1, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('MULTILINESTRING((0 0, 10 0,20 0,30 0), (0 15, 5 15, 10 15, 15 15, 20 15, 25 15, 30 15, 40 15), (13 13,15 15,17 17))'),
	ST_MakeEnvelope(12,12,18,18), true));

SELECT 2, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('LINESTRING(0 0, 10 0,20 0,30 0)'),
	ST_MakeEnvelope(12,12,18,18), true));

SELECT 3, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('MULTIPOLYGON Z(((10 10 1, 20 10 2, 30 10 3, 40 10 4, 20 20 5, 10 20 6, 10 10 1)),((10 10 1, 20 10 2, 20 20 3, 10 20 4, 10 10 5)))'),
	ST_MakeEnvelope(12,12,18,18), true));

SELECT 4, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('LINESTRING Z(0 0 1, 10 0 2,20 0 3,30 0 1)'),
	ST_MakeEnvelope(12,12,18,18), true));

SELECT 5, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('POLYGON((0 30, 15 30, 30 30, 30 0, 0 0, 0 30))'),
	ST_MakeEnvelope(12,12,18,18)));

SELECT 6, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('POLYGON((0 30, 15 30, 30 30, 30 0, 0 0, 0 30))'),
	ST_MakeEnvelope(12,12,18,18), false));

SELECT 7, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('POLYGON((0 30, 15 30, 30 30, 30 0, 0 0, 0 30))'),
	ST_MakeEnvelope(12,12,18,18), true));
