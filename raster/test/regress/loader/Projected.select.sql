SELECT st_srid(rast) from loadedrast limit 1;
SELECT st_extent(ST_SnapToGrid(rast::geometry,1)) from loadedrast;
