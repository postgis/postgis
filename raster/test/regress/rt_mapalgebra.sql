SET client_min_messages TO warning;

DROP TABLE IF EXISTS raster_nmapalgebra_in;
CREATE TABLE raster_nmapalgebra_in (
	rid integer,
	rast raster
);

INSERT INTO raster_nmapalgebra_in
	SELECT 0, NULL::raster AS rast UNION ALL
	SELECT 1, ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0) AS rast UNION ALL
	SELECT 2, ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0), 1, '8BUI', 1, 0) AS rast UNION ALL
	SELECT 3, ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(2, 2, 1, -1, 1, -1, 0, 0, 0), 1, '8BUI', 2, 0), 2, '32BF', 20, 0) AS rast UNION ALL
	SELECT 4, ST_AddBand(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(2, 2, 1, -1, 1, -1, 0, 0, 0), 1, '8BUI', 2, 0), 2, '32BF', 20, 0), 3, '16BUI', 200, 0) AS rast
;

CREATE OR REPLACE FUNCTION raster_nmapalgebra_test(
	value double precision[][][],
	pos int[][],
	VARIADIC userargs text[]
)
	RETURNS double precision
	AS $$
	BEGIN
		RAISE NOTICE 'value = %', value;
		RAISE NOTICE 'pos = %', pos;
		RAISE NOTICE 'userargs = %', userargs;

		IF userargs IS NULL OR array_length(userargs, 1) < 1 THEN
			RETURN 255;
		ELSE
			RETURN userargs[array_lower(userargs, 1)];
		END IF;
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION raster_nmapalgebra_test_no_userargs(
	value double precision[][][],
	pos int[][]
)
	RETURNS double precision
	AS $$
		SELECT $1[1][1][1] + $2[0][1] + $2[0][2];
	$$ LANGUAGE 'sql' IMMUTABLE STRICT;

SET client_min_messages TO notice;

SELECT
	rid,
	ST_Value(
		ST_MapAlgebra(
			ARRAY[ROW(rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		),
		1, 1, 1
	) = 255
FROM raster_nmapalgebra_in
WHERE rid IN (0, 1);

SELECT
	rid,
	ST_Value(
		ST_MapAlgebra(
			ARRAY[ROW(rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		),
		1, 1, 1
	) = 255
FROM raster_nmapalgebra_in
WHERE rid IN (2,3,4);

SELECT
	rid,
	ST_Value(
		ST_MapAlgebra(
			ARRAY[ROW(rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test_no_userargs(double precision[], int[])'::regprocedure
		),
		1, 1, 1
	) = 3
FROM raster_nmapalgebra_in
WHERE rid = 2;

SELECT
	rid,
	round(ST_Value(
		ST_MapAlgebra(
			ARRAY[ROW(rast, 2)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			NULL,
			'INTERSECTION', NULL,
			0, 0,
			'3.14'
		),
		1, 1, 1
	)::numeric, 2) = 3.14
FROM raster_nmapalgebra_in
WHERE rid IN (3,4);

-- NOTE OFFSET 0 is in place to force PostgreSQL 12+ to materialized CTE as it did in older versions
-- otherwise get extra notices
WITH foo AS (
	SELECT
		rid,
		ST_MapAlgebra(
			ARRAY[ROW(rast, 3)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			'8BUI',
			'INTERSECTION', NULL,
			1, 1
		) AS rast
	FROM raster_nmapalgebra_in
	WHERE rid IN (3,4) OFFSET 0
)
SELECT
	rid,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1)),
	ST_Value(rast, 1, 1, 1)
FROM foo;

INSERT INTO raster_nmapalgebra_in
	SELECT 10, ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0), 1, '16BUI', 1, 0) AS rast UNION ALL
	SELECT 11, ST_AddBand(ST_MakeEmptyRaster(2, 2, 2, 0, 1, -1, 0, 0, 0), 1, '16BUI', 2, 0) AS rast UNION ALL
	SELECT 12, ST_AddBand(ST_MakeEmptyRaster(2, 2, 4, 0, 1, -1, 0, 0, 0), 1, '16BUI', 3, 0) AS rast UNION ALL

	SELECT 13, ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, -2, 1, -1, 0, 0, 0), 1, '16BUI', 10, 0) AS rast UNION ALL
	SELECT 14, ST_AddBand(ST_MakeEmptyRaster(2, 2, 2, -2, 1, -1, 0, 0, 0), 1, '16BUI', 20, 0) AS rast UNION ALL
	SELECT 15, ST_AddBand(ST_MakeEmptyRaster(2, 2, 4, -2, 1, -1, 0, 0, 0), 1, '16BUI', 30, 0) AS rast UNION ALL

	SELECT 16, ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, -4, 1, -1, 0, 0, 0), 1, '16BUI', 100, 0) AS rast UNION ALL
	SELECT 17, ST_AddBand(ST_MakeEmptyRaster(2, 2, 2, -4, 1, -1, 0, 0, 0), 1, '16BUI', 200, 0) AS rast UNION ALL
	SELECT 18, ST_AddBand(ST_MakeEmptyRaster(2, 2, 4, -4, 1, -1, 0, 0, 0), 1, '16BUI', 300, 0) AS rast
;

DO $$ DECLARE r record;
BEGIN
-- NOTE: added OFFSET 0 to CTE clauses to force PostgreSQL 12+ to materialize like old versions did
	WITH foo AS (
		SELECT
			t1.rid,
			ST_MapAlgebra(
				ARRAY[ROW(ST_Union(t2.rast), 1)]::rastbandarg[],
				'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
				'32BUI',
				'CUSTOM', t1.rast,
				1, 1
			) AS rast
		FROM raster_nmapalgebra_in t1
		CROSS JOIN raster_nmapalgebra_in t2
		WHERE t1.rid = 10
			AND t2.rid BETWEEN 10 AND 18
			AND ST_Intersects(t1.rast, t2.rast)
		GROUP BY t1.rid, t1.rast OFFSET 0
	)
	SELECT
		rid,
		(ST_Metadata(rast)),
		(ST_BandMetadata(rast, 1)),
		ST_Value(rast, 1, 1, 1)
	INTO r
	FROM foo;
	RAISE NOTICE 'record = %', r;

	WITH foo AS (
		SELECT
			t1.rid,
			ST_MapAlgebra(
				ARRAY[ROW(ST_Union(t2.rast), 1)]::rastbandarg[],
				'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
				'32BUI',
				'CUSTOM', t1.rast,
				1, 1
			) AS rast
		FROM raster_nmapalgebra_in t1
		CROSS JOIN raster_nmapalgebra_in t2
		WHERE t1.rid = 14
			AND t2.rid BETWEEN 10 AND 18
			AND ST_Intersects(t1.rast, t2.rast)
		GROUP BY t1.rid, t1.rast OFFSET 0
	)
	SELECT
		rid,
		(ST_Metadata(rast)),
		(ST_BandMetadata(rast, 1)),
		ST_Value(rast, 1, 1, 1)
	INTO r
	FROM foo;
	RAISE NOTICE 'record = %', r;

	WITH foo AS (
		SELECT
			t1.rid,
			ST_MapAlgebra(
				ARRAY[ROW(ST_Union(t2.rast), 1)]::rastbandarg[],
				'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
				'32BUI',
				'CUSTOM', t1.rast,
				1, 1,
				'1000'
			) AS rast
		FROM raster_nmapalgebra_in t1
		CROSS JOIN raster_nmapalgebra_in t2
		WHERE t1.rid = 17
			AND t2.rid BETWEEN 10 AND 18
			AND ST_Intersects(t1.rast, t2.rast)
		GROUP BY t1.rid, t1.rast OFFSET 0
	)
	SELECT
		rid,
		(ST_Metadata(rast)),
		(ST_BandMetadata(rast, 1)),
		ST_Value(rast, 1, 1, 1)
	INTO r
	FROM foo;
	RAISE NOTICE 'record = %', r;
END $$;

INSERT INTO raster_nmapalgebra_in
	SELECT 20, ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0), 1, '16BUI', 1, 0) AS rast UNION ALL
	SELECT 21, ST_AddBand(ST_MakeEmptyRaster(2, 2, 1, -1, 1, -1, 0, 0, 0), 1, '16BUI', 2, 0) AS rast UNION ALL
	SELECT 22, ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, -2, 1, -1, 0, 0, 0), 1, '16BUI', 3, 0) AS rast
;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	WHERE t1.rid = 20
		AND t2.rid = 21 OFFSET 0
)
SELECT
	rid1,
	rid2,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	WHERE t1.rid = 20
		AND t2.rid = 22 OFFSET 0
)
SELECT
	rid1,
	rid2,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	WHERE t1.rid = 21
		AND t2.rid = 22 OFFSET 0
)
SELECT
	rid1,
	rid2,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			NULL,
			'UNION', NULL,
			0, 0
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	WHERE t1.rid = 20
		AND t2.rid = 21 OFFSET 0
)
SELECT
	rid1,
	rid2,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			NULL,
			'UNION', NULL,
			0, 0
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	WHERE t1.rid = 20
		AND t2.rid = 22 OFFSET 0
)
SELECT
	rid1,
	rid2,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		t3.rid AS rid3,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1), ROW(t3.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			NULL,
			'UNION', NULL,
			0, 0
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	CROSS JOIN raster_nmapalgebra_in t3
	WHERE t1.rid = 20
		AND t2.rid = 21
		AND t3.rid = 22 OFFSET 0
)
SELECT
	rid1,
	rid2,
	rid3,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		t3.rid AS rid3,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1), ROW(t3.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			NULL,
			'FIRST', NULL,
			0, 0
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	CROSS JOIN raster_nmapalgebra_in t3
	WHERE t1.rid = 20
		AND t2.rid = 21
		AND t3.rid = 22 OFFSET 0
)
SELECT
	rid1,
	rid2,
	rid3,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		t3.rid AS rid3,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1), ROW(t3.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			NULL,
			'SECOND', NULL,
			0, 0
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	CROSS JOIN raster_nmapalgebra_in t3
	WHERE t1.rid = 20
		AND t2.rid = 21
		AND t3.rid = 22 OFFSET 0
)
SELECT
	rid1,
	rid2,
	rid3,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		t3.rid AS rid3,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1), ROW(t3.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			NULL,
			'LAST', NULL,
			0, 0
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	CROSS JOIN raster_nmapalgebra_in t3
	WHERE t1.rid = 20
		AND t2.rid = 21
		AND t3.rid = 22 OFFSET 0
)
SELECT
	rid1,
	rid2,
	rid3,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		t3.rid AS rid3,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t2.rast, 1), ROW(t3.rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	CROSS JOIN raster_nmapalgebra_in t3
	WHERE t1.rid = 20
		AND t2.rid = 21
		AND t3.rid = 22 OFFSET 0
)
SELECT
	rid1,
	rid2,
	rid3,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

INSERT INTO raster_nmapalgebra_in
	SELECT 30, ST_AddBand(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 0, 1, -1, 0, 0, 0), 1, '16BUI', 1, 0), 2, '8BUI', 10, 0), 3, '32BUI', 100, 0) AS rast UNION ALL
	SELECT 31, ST_AddBand(ST_AddBand(ST_AddBand(ST_MakeEmptyRaster(2, 2, 0, 1, 1, -1, 0, 0, 0), 1, '16BUI', 2, 0), 2, '8BUI', 20, 0), 3, '32BUI', 300, 0) AS rast
;

WITH foo AS (
	SELECT
		t1.rid AS rid,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 1), ROW(t1.rast, 2), ROW(t1.rast, 3)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		) AS rast
	FROM raster_nmapalgebra_in t1
	WHERE t1.rid = 30 OFFSET 0
)
SELECT
	rid,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 3), ROW(t1.rast, 1), ROW(t1.rast, 3)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		) AS rast
	FROM raster_nmapalgebra_in t1
	WHERE t1.rid = 30 OFFSET 0
)
SELECT
	rid,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 2), ROW(t1.rast, 2)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			'16BUI'::text
		) AS rast
	FROM raster_nmapalgebra_in t1
	WHERE t1.rid = 31 OFFSET 0
)
SELECT
	rid,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid1,
		t2.rid AS rid2,
		ST_MapAlgebra(
			ARRAY[ROW(t1.rast, 2), ROW(t2.rast, 1), ROW(t2.rast, 2)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,
			'16BUI'
		) AS rast
	FROM raster_nmapalgebra_in t1
	CROSS JOIN raster_nmapalgebra_in t2
	WHERE t1.rid = 30
		AND t2.rid = 31 OFFSET 0
)
SELECT
	rid1,
	rid2,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid,
		ST_MapAlgebra(
			t1.rast, ARRAY[3, 1, 3]::int[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		) AS rast
	FROM raster_nmapalgebra_in t1
	WHERE t1.rid = 30 OFFSET 0
)
SELECT
	rid,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

WITH foo AS (
	SELECT
		t1.rid AS rid,
		ST_MapAlgebra(
			t1.rast, 2,
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		) AS rast
	FROM raster_nmapalgebra_in t1
	WHERE t1.rid = 30 OFFSET 0
)
SELECT
	rid,
	(ST_Metadata(rast)),
	(ST_BandMetadata(rast, 1))
FROM foo;

-- Ticket #2803
-- http://trac.osgeo.org/postgis/ticket/2803
ALTER FUNCTION raster_nmapalgebra_test(
	value double precision[][][],
	pos int[][],
	VARIADIC userargs text[]
)
	IMMUTABLE STRICT;

SELECT
	rid,
	ST_Value(
		ST_MapAlgebra(
			ARRAY[ROW(rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure
		),
		1, 1, 1
	) = 255
FROM raster_nmapalgebra_in
WHERE rid IN (2);

-- Ticket #2807
-- https://trac.osgeo.org/postgis/ticket/2807
CREATE OR REPLACE FUNCTION raster_nmapalgebra_one(
	value double precision[][][],
	pos int[][],
	VARIADIC userargs text[]
)
	RETURNS double precision
	AS $$ SELECT 1::double precision $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION raster_nmapalgebra_null(
	value double precision[][][],
	pos int[][],
	VARIADIC userargs text[]
)
	RETURNS double precision
	AS $$ SELECT NULL::double precision $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION raster_nmapalgebra_zero(
	value double precision[][][],
	pos int[][],
	VARIADIC userargs text[]
)
	RETURNS double precision
	AS $$ SELECT 0::double precision $$
	LANGUAGE 'sql' IMMUTABLE;

CREATE OR REPLACE FUNCTION raster_nmapalgebra_sum(
	value double precision[][][],
	pos int[][],
	VARIADIC userargs text[]
)
	RETURNS double precision
	AS $$ SELECT value[1][1][1] + value[2][1][1] $$
	LANGUAGE 'sql' IMMUTABLE;

SET client_min_messages TO warning;

WITH src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(2, 2, 0, 0, 1),
		'8BUI', 255.0
	) AS rast
)
SELECT
	'#2807.expr',
	(ST_BandMetadata(ST_MapAlgebra(rast, 1, '8BUI', '0'), 1)),
	ST_Value(ST_MapAlgebra(rast, 1, '8BUI', '0'), 1, 1)
FROM src;

WITH src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(2, 2, 0, 0, 1),
		'8BUI', 255.0
	) AS rast
)
SELECT
	'#2807.callback',
	(ST_BandMetadata(ST_MapAlgebra(
		rast, 1,
		'raster_nmapalgebra_one(double precision[], int[], text[])'::regprocedure
	), 1)),
	ST_Value(ST_MapAlgebra(
		rast, 1,
		'raster_nmapalgebra_one(double precision[], int[], text[])'::regprocedure
	), 1, 1)
FROM src;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 1.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 2.0, NULL
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, rast2,
		'[rast1.val] + [rast2.val]',
		'16BUI', 'UNION'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.union-coextensive',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 1.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 1, 0, 1),
			'16BUI', 2.0, NULL
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, rast2,
		'[rast1.val] + [rast2.val]',
		'16BUI', 'UNION'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.union-gap',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 1.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(2, 1, 0, 0, 1),
			'16BUI', 2.0, NULL
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, rast2,
		'[rast1.val] + [rast2.val]',
		'16BUI', 'FIRST'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.first-covered',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(2, 1, 0, 0, 1),
			'16BUI', 0.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 1.0, NULL
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, rast2,
		'[rast1.val] + [rast2.val]',
		'16BUI', 'FIRST',
		NULL, '[rast1.val]'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.expr-handled-extent-gap',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 0.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 5.0, 5.0
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, rast2,
		'[rast1.val] + [rast2.val]',
		'16BUI', 'INTERSECTION',
		NULL, '[rast1.val]'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.expr-handled-source-nodata',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 0.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 5.0, 5.0
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, 1,
		rast2, 1,
		'raster_nmapalgebra_zero(double precision[][][], int[][], text[])'::regprocedure,
		'16BUI'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.callback-handled-source-nodata',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 0.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 5.0, 5.0
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, 1,
		rast2, 1,
		'raster_nmapalgebra_zero(double precision[][][], int[][], text[])'::regprocedure,
		'16BUI'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.callback-later-band-nodata',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 1.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 2.0, NULL
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, 1,
		rast2, 2,
		'[rast1.val] + [rast2.val]',
		'16BUI', 'INTERSECTION'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.expr-missing-band-nodata',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 1.0, NULL
		) AS rast1,
		ST_AddBand(
			ST_MakeEmptyRaster(1, 1, 0, 0, 1),
			'16BUI', 2.0, NULL
		) AS rast2
), alg AS (
	SELECT ST_MapAlgebra(
		rast1, 1,
		rast2, 2,
		'raster_nmapalgebra_sum(double precision[][][], int[][], text[])'::regprocedure,
		'16BUI'
	) AS rast
	FROM src
)
SELECT
	'#2807.nmap.callback-missing-band-nodata',
	(ST_BandMetadata(rast, 1)).nodatavalue,
	(ST_DumpValues(rast)).valarray
FROM alg;

WITH src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(2, 2, 0, 0, 1),
		'8BUI', 255.0
	) AS rast
)
SELECT
	'#2807.expr.null',
	(ST_BandMetadata(ST_MapAlgebra(rast, 1, '8BUI', 'NULL'), 1))
FROM src;

WITH src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(2, 2, 0, 0, 1),
		'8BUI', 255.0
	) AS rast
)
SELECT
	'#2807.callback.null',
	(ST_BandMetadata(ST_MapAlgebra(
		rast, 1,
		'raster_nmapalgebra_null(double precision[], int[], text[])'::regprocedure
	), 1))
FROM src;

SET client_min_messages TO notice;

-- Ticket #2802
-- http://trac.osgeo.org/postgis/ticket/2802
CREATE OR REPLACE FUNCTION raster_nmapalgebra_test_bad_return(
	value double precision[][][],
	pos int[][],
	VARIADIC userargs text[]
)
	RETURNS text
	AS $$
	BEGIN
		RETURN 255;
	END;
	$$ LANGUAGE 'plpgsql' IMMUTABLE;

SELECT
	rid,
	ST_Value(
		ST_MapAlgebra(
			ARRAY[ROW(rast, 1)]::rastbandarg[],
			'raster_nmapalgebra_test_bad_return(double precision[], int[], text[])'::regprocedure
		),
		1, 1, 1
	) = 255
FROM raster_nmapalgebra_in
WHERE rid IN (2);

DROP FUNCTION IF EXISTS raster_nmapalgebra_one(double precision[], int[], text[]);
DROP FUNCTION IF EXISTS raster_nmapalgebra_null(double precision[], int[], text[]);
DROP FUNCTION IF EXISTS raster_nmapalgebra_zero(double precision[][][], int[][], text[]);
DROP FUNCTION IF EXISTS raster_nmapalgebra_sum(double precision[][][], int[][], text[]);
DROP FUNCTION IF EXISTS raster_nmapalgebra_test(double precision[], int[], text[]);
DROP FUNCTION IF EXISTS raster_nmapalgebra_test_no_userargs(double precision[], int[]);
DROP FUNCTION IF EXISTS raster_nmapalgebra_test_bad_return(double precision[], int[], text[]);
DROP TABLE IF EXISTS raster_nmapalgebra_in;
