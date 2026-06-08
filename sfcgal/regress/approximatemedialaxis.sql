SELECT 'square', ST_AsEwkt(CG_ApproximateMedialAxis('SRID=3857;POLYGON((0 0,1 0,1 1,0 1,0 0))'));
SELECT 'rect', ST_AsEwkt(CG_ApproximateMedialAxis('SRID=3857;POLYGON((0 0,2 0,2 1,0 1,0 0))'));
-- projected=false must equal 1-arg overload
SELECT 'square_proj_false', ST_AsEwkt(CG_ApproximateMedialAxis('SRID=3857;POLYGON((0 0,1 0,1 1,0 1,0 0))', false));
SELECT 'rect_proj_false', ST_AsEwkt(CG_ApproximateMedialAxis('SRID=3857;POLYGON((0 0,2 0,2 1,0 1,0 0))', false));
