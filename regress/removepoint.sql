--  Can't remove points from a 2-point linestring
SELECT removepoint('LINESTRING(0 0, 1 1)', 0);

--  Out of range indexes
SELECT removepoint('LINESTRING(0 0, 1 1, 2 2)', 3);
SELECT removepoint('LINESTRING(0 0, 1 1, 2 2)', -1);

-- Removing first/last points
SELECT asewkt(removepoint('LINESTRING(0 0, 1 1, 2 2)', 0));
SELECT asewkt(removepoint('LINESTRING(0 0, 1 1, 2 2)', 2));

-- Removing first/last points with higher dimension
SELECT asewkt(removepoint('LINESTRING(0 0 0, 1 1 1, 2 2 2)', 0));
SELECT asewkt(removepoint('LINESTRING(0 0 0, 1 1 1, 2 2 2)', 2));
SELECT asewkt(removepoint('LINESTRINGM(0 0 0, 1 1 1, 2 2 2)', 0));
SELECT asewkt(removepoint('LINESTRINGM(0 0 0, 1 1 1, 2 2 2)', 2));
SELECT asewkt(removepoint('LINESTRING(0 0 0 0, 1 1 1 1, 2 2 2 2)', 0));
SELECT asewkt(removepoint('LINESTRING(0 0 0 0, 1 1 1 1, 2 2 2 2)', 2));

-- Removing intermediate points with higher dimension
SELECT asewkt(removepoint('LINESTRING(0 0 0 0, 1 1 1 1, 2 2 2 2, 3 3 3 3, 4 4 4 4, 5 5 5 5, 6 6 6 6, 7 7 7 7)', 2));
SELECT asewkt(removepoint('LINESTRING(0 0 0 0, 1 1 1 1, 2 2 2 2, 3 3 3 3, 4 4 4 4, 5 5 5 5, 6 6 6 6, 7 7 7 7)', 4));


