SELECT 0, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('MULTIPOLYGON(((10 10, 20 10, 30 10, 40 10, 20 20, 10 20, 10 10)),((10 10, 20 10, 20 20, 10 20, 10 10)))'),
    box2d(ST_GeomFromText('POLYGON((12 12, 18 12, 18 18, 12 18, 12 12))'))));


SELECT 1, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    ST_GeomFromText('MULTILINESTRING((0 0, 10 0,20 0,30 0), (0 15, 5 15, 10 15, 15 15, 20 15, 25 15, 30 15, 40 15), (13 13,15 15,17 17))'),
    box2d(st_GeomFromText('POLYGON((12 12, 18 12, 18 18, 12 18, 12 12))'))));

SELECT 2, ST_AsText(
    ST_RemoveIrrelevantPointsForView(
    st_GeomFromText('LINESTRING(0 0, 10 0,20 0,30 0)'),
    box2d(st_GeomFromText('POLYGON((12 12, 18 12, 18 18, 12 18, 12 12))'))));
