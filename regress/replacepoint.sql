--  Out of range indexes
SELECT ReplacePoint('LINESTRING(0 0, 1 1, 2 2)', 3, 'POINT(9 9)');
SELECT ReplacePoint('LINESTRING(0 0, 1 1, 2 2)', -1, 'POINT(9 9)');

--  Invalid inputs
SELECT ReplacePoint('MULTIPOINT(0 0, 1 1, 2 2)', 3, 'POINT(9 9)');
SELECT ReplacePoint('LINESTRING(0 0, 1 1, 2 2)', -1, 'MULTIPOINT(9 9, 0 0)');

-- Replacing 3dz line with 3dm point 
SELECT asewkt(ReplacePoint('LINESTRING(-1 -1 -1, 1 1 1, 2 2 2)', 0, 'POINT(90 91 92)'));

-- Replacing 3dm line with 3dz point 
SELECT asewkt(ReplacePoint('LINESTRINGM(0 0 0, 1 1 1, 2 2 2)', 1, 'POINTM(90 91 92)'));

-- Replacing 3dm line with 4d point 
SELECT asewkt(ReplacePoint('LINESTRINGM(0 0 0, 1 1 1, 2 2 2)', 2, 'POINT(90 91 92 93)'));

-- Replacing 3dz line with 4d point 
SELECT asewkt(ReplacePoint('LINESTRING(0 0 0, 1 1 1, 2 2 2)', 1, 'POINT(90 91 92 93)'));

-- Replacing 4d line with 2d/3dm/3dz/4d point 
SELECT asewkt(ReplacePoint('LINESTRING(0 0 0 0, 1 1 1 1, 2 2 2 2, 4 4 4 4)', 3, 'POINT(90 91)'));
SELECT asewkt(ReplacePoint('LINESTRING(0 0 0 0, 1 1 1 1, 2 2 2 2, 4 4 4 4)', 2, 'POINT(90 91 92)'));
SELECT asewkt(ReplacePoint('LINESTRING(0 0 0 0, 1 1 1 1, 2 2 2 2, 4 4 4 4)', 1, 'POINTM(90 91 92)'));
SELECT asewkt(ReplacePoint('LINESTRING(0 0 0 0, 1 1 1 1, 2 2 2 2, 4 4 4 4)', 0, 'POINT(90 91 92 93)'));

