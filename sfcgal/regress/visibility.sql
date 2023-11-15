SELECT 'Visibility point in polygon', ST_AsText(CG_Visibility('POLYGON((0.0 4.0,0.0 0.0,3.0 2.0,4.0 0.0,4.0 4.0,1.0 2.0,0.0 4.0))', 'POINT(0.5 2.0)'), 2);
SELECT 'Visibility point in polygon with hole', ST_AsText(CG_Visibility('POLYGON((0.0 4.0,0.0 0.0,3.0 2.0,4.0 0.0,4.0 4.0,1.0 2.0,0.0 4.0),(0.2 1.8,0.9 1.8,0.7 1.2,0.2 1.8))', 'POINT(0.5 2.0)'), 2);
SELECT 'Visibility segment in polygon', ST_AsText(CG_Visibility('POLYGON((0.0 4.0,0.0 0.0,3.0 2.0,4.0 0.0,4.0 4.0,1.0 2.0,0.0 4.0))', 'POINT(1.0 2.0)', 'POINT(4.0 4.0)'), 2);
SELECT 'Visibility segment in polygon with hole', ST_AsText(CG_Visibility('POLYGON((1.0 2.0,12.0 3.0,19.0 -2.0,12.0 6.0,14.0 14.0,9.0 5.0,1.0 2.0),(8.0 3.0,8.0 4.0,10.0 3.0,8.0 3.0),(10.0 6.0,11.0 7.0,11.0 6.0,10.0 6.0))', 'POINT(19.0 -2.0)', 'POINT(12.0 6.0)'), 2);
