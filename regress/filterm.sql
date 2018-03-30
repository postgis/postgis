SELECT '1', ST_astext(ST_FilterByM('LINESTRING(0 0 0 0, 8 8 5 2, 0 16 0 5, 16 16 0 2)',2,null,'t'));
SELECT '2', ST_astext(ST_FilterByM('LINESTRING(0 0 0 0, 8 8 5 2, 0 16 0 5, 16 16 0 2)',2,4,'t'));
