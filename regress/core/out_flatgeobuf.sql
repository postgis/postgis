--set client_min_messages to DEBUG3;
SELECT 'T1', ST_AsFlatGeobuf(q, 'geom')
    FROM (SELECT ST_MakePoint(1.1, 2.1) AS geom) AS q;
SELECT 'T3', ST_AsFlatGeobuf(q, 'geom')
    FROM (SELECT ST_MakePoint(8678678.67867, 678678.546456) AS geom) AS q;
