-- Repeat all tests with the new function names.
-- Single 3dz polygon w/out holes
select 'BuildArea', ST_asewkt(ST_buildarea('SRID=2;LINESTRING(0 0 2, 10 0 4, 10 10 6, 0 10 8, 0 0 10)'));

-- Single 3dz polygon with holes
select 'BuildArea', ST_asewkt(ST_buildarea('SRID=3;MULTILINESTRING((0 0 2, 10 0 4, 10 10 6, 0 10 8, 0 0 10),(2 2 1, 2 4 2, 4 4 3, 4 2 4, 2 2 5),(5 5 10, 5 7 9, 7 7 8, 7 5 7, 5 5 6))'));

-- Single 3dm polygon with holes (M is currently discarded)
select 'BuildArea', ST_asewkt(ST_buildarea('SRID=3;MULTILINESTRINGM((0 0 2, 10 0 4, 10 10 6, 0 10 8, 0 0 10),(2 2 1, 2 4 2, 4 4 3, 4 2 4, 2 2 5),(5 5 10, 5 7 9, 7 7 8, 7 5 7, 5 5 6))'));

-- Single 4d polygon with holes (M is currently discarded)
select 'BuildArea', ST_asewkt(ST_buildarea('SRID=3;MULTILINESTRING((0 0 2 9, 10 0 4 9, 10 10 6 9, 0 10 8 9, 0 0 10 9),(2 2 1 9, 2 4 2 9, 4 4 3 9, 4 2 4 9, 2 2 5 9),(5 5 10 9, 5 7 9 9, 7 7 8 9, 7 5 7 9, 5 5 6 9))'));

-- Multi 4d polygon with holes (M is currently discarded)
select 'BuildArea', ST_asewkt(ST_buildarea('SRID=3;MULTILINESTRING( (0 0 2 9, 10 0 4 9, 10 10 6 9, 0 10 8 9, 0 0 10 9), (2 2 1 9, 2 4 2 9, 4 4 3 9, 4 2 4 9, 2 2 5 9), (5 5 10 9, 5 7 9 9, 7 7 8 9, 7 5 7 9, 5 5 6 9), (20 0 2 9,30 0 4 9,30 10 6 9,20 10 8 9,20 0 10 9), (22 2 1 9,22 4 2 9,24 4 3 9,24 2 4 9,22 2 5 9), (25 5 10 9,25 7 9 9,27 7 8 9,27 5 7 9,25 5 6 9))'));

-- Multi 2d polygon with holes (OGC doesn't support higher dims)
select 'BdMPolyFromText', ST_asewkt(ST_BdMPolyFromText('MULTILINESTRING( (0 0, 10 0, 10 10, 0 10, 0 0), (2 2, 2 4, 4 4, 4 2, 2 2), (5 5, 5 7, 7 7, 7 5, 5 5), (20 0,30 0,30 10,20 10,20 0), (22 2,22 4,24 4,24 2,22 2), (25 5,25 7,27 7,27 5,25 5))', 3));

-- Single 2d polygon with holes (OGC doesn't support higher dims)
select 'BdPolyFromText', ST_asewkt(ST_BdPolyFromText('MULTILINESTRING((0 0, 10 0, 10 10, 0 10, 0 0),(2 2, 2 4, 4 4, 4 2, 2 2),(5 5, 5 7, 7 7, 7 5, 5 5))', 3));

-- Invalid input (not a linestring) to BdPolyFromText and BdMPolyFromText
select ST_BdPolyFromText('POINT(0 0)', 3);
select ST_BdMPolyFromText('POINT(0 0)', 3);

-- MultiPolygon forming input to BdPolyFromText 
select ST_BdPolyFromText('MULTILINESTRING( (0 0, 10 0, 10 10, 0 10, 0 0), (2 2, 2 4, 4 4, 4 2, 2 2), (5 5, 5 7, 7 7, 7 5, 5 5), (20 0,30 0,30 10,20 10,20 0), (22 2,22 4,24 4,24 2,22 2), (25 5,25 7,27 7,27 5,25 5))', 3);

-- SinglePolygon forming input to BdMPolyFromText 
select 'BdMPolyFromText', ST_asewkt(ST_BdMPolyFromText('MULTILINESTRING((0 0, 10 0, 10 10, 0 10, 0 0),(2 2, 2 4, 4 4, 4 2, 2 2),(5 5, 5 7, 7 7, 7 5, 5 5))', 3));
