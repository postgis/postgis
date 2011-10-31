
select ashexewkb(the_geom, 'NDR') from loadedshp order by gid;
select ashexewkb(the_geom, 'XDR') from loadedshp order by gid;
select asewkt(the_geom) from loadedshp order by gid;
