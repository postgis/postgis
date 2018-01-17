CREATE TABLE "test" (
        "num" numeric
,
        "the_geog" geography
);

INSERT INTO test (num, the_geog)
    SELECT i,
    CASE
        WHEN i%0.1 = 0.0 THEN NULL
        WHEN i%0.11 = 0 THEN 'SRID=4326;POINTZ EMPTY'::geography
    ELSE
        ST_GeographyFromText('SRID=4326;POINTZ(' || i || ' ' || i || ' ' || i || ')')
    END
    FROM generate_series(-20.0, 80.0, 0.01) i;
