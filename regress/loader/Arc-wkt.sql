
select ashexewkb(the_geom, 'NDR') from loadedshp order by 1;
select ashexewkb(the_geom, 'XDR') from loadedshp order by 1;
select asewkt(the_geom) from loadedshp order by 1;
