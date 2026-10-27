-- South lambert
select x, y, _ST_BestSRID(ST_Point(x, y))
from ( select 0 as x, -70 as y ) as foo ;

-- North lambert
select x, y, _ST_BestSRID(ST_Point(x, y))
from ( select 0 as x, 70 as y ) as foo ;

-- UTM north
select x, 60, _ST_BestSRID(ST_Point(x, 60))
from generate_series(-177, 177, 6) as x ;
-- Corner cases
select -180, 60, _ST_BestSRID(ST_Point(-180, 60));
select 180, 60, _ST_BestSRID(ST_Point(180, 60));

-- UTM south
select x, -60, _ST_BestSRID(ST_Point(x, -60))
from generate_series(-177, 177, 6) as x;
-- Corner cases
select -180, -60, _ST_BestSRID(ST_Point(-180, -60));
select 180, -60, _ST_BestSRID(ST_Point(180, -60));

-- World mercator
select 'world', _ST_BestSRID(ST_Point(-160, -40), ST_Point(160, 40));

-- Public ST_BestSRID returns EPSG codes where the internal choice has an EPSG equivalent
select 'public.north_utm', ST_BestSRID(ST_Point(-3, 60));
select 'public.south_utm', ST_BestSRID(ST_Point(-3, -60));
select 'public.north_laea', ST_BestSRID(ST_Point(0, 70));
select 'public.south_laea', ST_BestSRID(ST_Point(0, -70));
select 'public.world_mercator', ST_BestSRID(ST_Point(-160, -40), ST_Point(160, 40));
select 'public.custom_laea', ST_BestSRID('POLYGON((0 0,10 0,10 10,0 10,0 0))'::geography);
select 'public.one_empty', ST_BestSRID('POINT EMPTY'::geography);
select 'public.two_empty', ST_BestSRID('POINT EMPTY'::geography, 'POINT EMPTY'::geography);
select 'public.left_empty', ST_BestSRID('POINT EMPTY'::geography, ST_Point(-3, 60));

-- infinite coords
SELECT _ST_BestSRID(ST_Point('-infinity', 'infinity'));

-- Keep this statement on one line so parser notice locations stay stable
-- across LF and CRLF platforms.
SELECT '#5347', ST_Intersection('0102000020E610000005000000000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F'::geography, '0101000020E6100000000000000000F07F000000000000F07F'::geography);
