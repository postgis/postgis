select ST_AsText(ST_SnapToGrid(the_geom::geometry,0.00000001), 8) from loadedshp;

