CREATE TABLE "test" (
	"num" bigint,
	"the_geom" geometry
);

INSERT INTO test (num, the_geom)
    SELECT i,
    CASE
        WHEN i%1000 = 0 THEN NULL
        WHEN i%1100 = 0 THEN 'POINTZ EMPTY'::geometry
    ELSE
        st_makepoint(i::numeric/10, i::numeric/10, i::numeric/10)
    END
    FROM generate_series(1, 20000) i;
