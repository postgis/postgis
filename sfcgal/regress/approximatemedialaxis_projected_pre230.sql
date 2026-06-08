-- projected=true falls back to non-projected result with a notice on SFCGAL < 2.3.0
SELECT 'square_proj_true', ST_AsEwkt(CG_ApproximateMedialAxis('SRID=3857;POLYGON((0 0,1 0,1 1,0 1,0 0))', true));
SELECT 'rect_proj_true', ST_AsEwkt(CG_ApproximateMedialAxis('SRID=3857;POLYGON((0 0,2 0,2 1,0 1,0 0))', true));
