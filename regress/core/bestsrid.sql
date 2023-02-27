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

-- infinite coords
SELECT _ST_BestSRID(ST_Point('-infinity', 'infinity'));

SELECT '#5347', ST_Intersection('0102000020E610000005000000000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F000000000000F07F'::geography,
                       '0101000020E6100000000000000000F07F000000000000F07F'::geography);

