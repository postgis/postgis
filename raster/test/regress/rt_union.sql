SELECT '#1 ' as run, ST_AsText((gval).geom),  (gval).val::text
            FROM (SELECT ST_DumpAsPolygons(ST_Union(rast) ) As gval
            FROM (
					SELECT   i As rid, ST_AddBand(
						ST_MakeEmptyRaster(10, 10, 10*i, 10*i, 2, 2, 0, 0, ST_SRID(ST_Point(0,0) )),
						'8BUI'::text, 10*i, 0)  As rast
                           FROM generate_series(0,10) As i ) As foo ) As foofoo ORDER BY (gval).val;
                           
SELECT '#2 ' As run, ST_AsText((gval).geom), (gval).val::text
            FROM (SELECT ST_DumpAsPolygons(ST_Union(rast,'MEAN') ) As gval
            FROM (
					SELECT   i As rid, ST_AddBand(
						ST_MakeEmptyRaster(10, 10, 10*i, 10*i, 2, 2, 0, 0, ST_SRID(ST_Point(0,0) )),
						'8BUI'::text, 10*i, 0)  As rast
                           FROM generate_series(0,10) As i ) As foo ) As foofoo
                       ORDER BY (gval).val LIMIT 2;
