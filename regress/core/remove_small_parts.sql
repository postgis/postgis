SELECT 0, ST_AsText(
    ST_RemoveSmallParts(
    ST_GeomFromText('MULTIPOLYGON(
	((60 160, 120 160, 120 220, 60 220, 60 160), (70 170, 70 210, 110 210, 110 170, 70 170)),
	((85 75, 155 75, 155 145, 85 145, 85 75)),
	((50 110, 70 110, 70 130, 50 130, 50 110)))'),
	50, 50));

SELECT 1, ST_AsText(
    ST_RemoveSmallParts(
    ST_GeomFromText('LINESTRING(10 10, 20 20)'),
    50, 50));

SELECT 2, ST_AsText(
    ST_RemoveSmallParts(
    ST_GeomFromText('LINESTRING(10 10 10, 20 20 20)'),
    50, 50));
