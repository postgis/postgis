--set client_min_messages to DEBUG3;
SELECT 'T1', encode(ST_AsFlatGeobuf(q, 'geom'), 'base64')
    FROM (SELECT ST_MakePoint(1.1, 2.1) AS geom) AS q;