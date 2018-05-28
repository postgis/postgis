CREATE TABLE "test" (
	"num" bigint,
	"the_box" box3d
);

SELECT setseed(0.77777) ;

INSERT INTO test (num, the_box)
    SELECT i,
    CASE
        WHEN i%1000 = 0 THEN NULL
    ELSE
        st_3Dmakebox(
			st_makepoint(i::numeric/10, i::numeric/10, i::numeric/10),
			st_makepoint(i::numeric/10 + random()*5, i::numeric/10 + random()*5, i::numeric/10 + random()*5)
		)
    END
    FROM generate_series(1, 20000) i;
