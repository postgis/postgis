select ST_Ashexewkb(the_geom::geometry, 'NDR') from loadedshp;
select ST_Ashexewkb(the_geom::geometry, 'XDR') from loadedshp;
select ST_Asewkt(the_geom::geometry) from loadedshp;
