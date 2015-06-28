SELECT 'square', ST_AsEwkt(ST_ApproximateMedialAxis('SRID=3857;POLYGON((0 0,1 0,1 1,0 1,0 0))'));
SELECT 'rect1', ST_AsEwkt(ST_ApproximateMedialAxis('SRID=4326;POLYGON((0 0,2 0,2 1,0 1,0 0))'));
