SELECT 'square', ST_AsText(ST_ApproximateMedialAxis('POLYGON((0 0,1 0,1 1,0 1,0 0))'));
SELECT 'rect1', ST_AsText(ST_ApproximateMedialAxis('POLYGON((0 0,2 0,2 1,0 1,0 0))'));

