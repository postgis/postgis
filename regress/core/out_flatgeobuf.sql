--set client_min_messages to DEBUG3;
SELECT 'T1', ST_AsFlatGeobuf(q)
    FROM (SELECT ST_MakePoint(1.1, 2.1) AS geom) AS q;
SELECT 'T2', ST_AsFlatGeobuf(q)
    FROM (SELECT null::geometry AS geom) AS q;
SELECT 'T3', ST_AsFlatGeobuf(q)
    FROM (SELECT ST_MakePoint(8678678.67867, 678678.546456) AS geom) AS q;
SELECT 'T4', ST_AsFlatGeobuf(q)
    FROM (SELECT ST_MakeLine(ST_MakePoint(1.1, 2.1), ST_MakePoint(10.1, 20.1)) AS geom) AS q;
