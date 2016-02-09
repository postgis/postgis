select ST_Ashexewkb(the_geom::geometry, 'NDR') from loadedshp order by gid;
select ST_Ashexewkb(the_geom::geometry, 'XDR') from loadedshp order by gid;
select ST_Asewkt(the_geom::geometry) from loadedshp order by gid;
