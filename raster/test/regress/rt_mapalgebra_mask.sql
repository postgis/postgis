SET client_min_messages TO warning;

DROP TABLE IF EXISTS raster_nmapalgebra_in;
CREATE TABLE raster_nmapalgebra_mask_in (
	rid integer,
	rast raster
);

INSERT INTO raster_nmapalgebra_mask_in
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

SET client_min_messages TO notice;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,1],[1,1],[1,1]]::double precision[],false) from raster_nmapalgebra_mask_in;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,1,1],[1,1,1]]::double precision[],false) from raster_nmapalgebra_mask_in;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,1,1,1],[1,1,1,1],[1,1,1,1],[1,1,1,1]]::double precision[],false) from raster_nmapalgebra_mask_in;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,NULL::double precision[],false)) AS dv
from raster_nmapalgebra_mask_in) As f
ORDER BY rid, (dv).nband;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[NULL]::double precision[],false) from raster_nmapalgebra_mask_in;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[1]::double precision[],false) from raster_nmapalgebra_mask_in;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1]]::double precision[],false)) AS dv from raster_nmapalgebra_mask_in ) As f
ORDER BY rid, (dv).nband;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[NULL]]::double precision[],false)) AS dv
from raster_nmapalgebra_mask_in) AS f
ORDER BY rid, (dv).nband;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,1,1],[1,1,1],[1,1,1]]::double precision[],false)) AS dv
    from raster_nmapalgebra_mask_in) AS f
ORDER BY rid, (dv).nband;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,0,0],[0,0,0],[0,0,0]]::double precision[],false)) AS dv
    from raster_nmapalgebra_mask_in) AS f
ORDER BY rid, (dv).nband;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,1],[1,1],[1,1]]::double precision[],true) from raster_nmapalgebra_mask_in;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,1,1],[1,1,1]]::double precision[],true) from raster_nmapalgebra_mask_in;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,NULL::double precision[],true)) As dv from raster_nmapalgebra_mask_in) AS f
ORDER BY rid, (dv).nband;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[NULL]::double precision[],true) from raster_nmapalgebra_mask_in;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[1]::double precision[],true) from raster_nmapalgebra_mask_in;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1]]::double precision[],true)) As dv from raster_nmapalgebra_mask_in) AS f
ORDER BY rid, (dv).nband;

SELECT *
FROM (SELECT  rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[NULL]]::double precision[],true)) As dv from raster_nmapalgebra_mask_in) As f
ORDER BY rid, (dv).nband;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,1,1],[1,1,1],[1,1,1]]::double precision[],true)) As dv
from raster_nmapalgebra_mask_in) As f
ORDER BY rid, (dv).nband;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[1,0,0],[0,0,0],[0,0,0]]::double precision[],true)) AS dv
    from raster_nmapalgebra_mask_in) As f
ORDER BY rid, (dv).nband;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[.5,.5],[.5,.5],[.5,.5]]::double precision[],true) from raster_nmapalgebra_mask_in;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[.5,.5,.5],[.5,.5,.5]]::double precision[],true) from raster_nmapalgebra_mask_in;

select st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[.5]::double precision[],true) from raster_nmapalgebra_mask_in;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[.5]]::double precision[],true)) As dv from raster_nmapalgebra_mask_in) As f
ORDER BY rid, (dv).nband;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[.5,.5,.5],[.5,.5,.5],[.5,.5,.5]]::double precision[],true)) AS dv
from raster_nmapalgebra_mask_in) AS f
ORDER BY rid, (dv).nband;

SELECT *
FROM (SELECT rid, st_dumpvalues(st_mapalgebra(rast,1,'raster_nmapalgebra_test(double precision[], int[], text[])'::regprocedure,ARRAY[[.5,0,0],[0,0,0],[0,0,0]]::double precision[],true)) AS dv
    from raster_nmapalgebra_mask_in) AS f
ORDER BY rid, (dv).nband;

SET client_min_messages TO warning;

WITH src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0),
			1, '32BF', 0, NULL
		),
		1, 1, 1, ARRAY[
			[1, 2, 3],
			[4, 5, 6],
			[7, 8, 9]
		]::double precision[][]
	) AS rast
), kernel AS (
	SELECT ARRAY[
		[0, 1, 0],
		[1, 1, 1],
		[0, 1, 0]
	]::double precision[][] AS matrix
)
SELECT 'st_convolution_equiv',
	ST_AsBinary(ST_Convolution(rast, matrix)) =
	ST_AsBinary(ST_MapAlgebra(
		rast,
		1,
		'st_sum4ma(double precision[][][], integer[][], text[])'::regprocedure,
		matrix,
		true
	))
FROM src, kernel;

WITH src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0),
			1, '32BF', 0, NULL
		),
		1, 1, 1, ARRAY[
			[1, 2, 3],
			[4, 5, 6],
			[7, 8, 9]
		]::double precision[][]
	) AS rast
)
SELECT 'st_convolution_center',
	ST_Value(
		ST_Convolution(
			rast,
			ARRAY[
				[0, 1, 0],
				[1, 1, 1],
				[0, 1, 0]
			]::double precision[][]
		),
		2, 2
	)
FROM src;

WITH src AS (
	SELECT ST_SetValues(
		ST_AddBand(
			ST_MakeEmptyRaster(3, 3, 0, 0, 1, -1, 0, 0, 0),
			1, '8BUI', 0, NULL
		),
		1, 1, 1, ARRAY[
			[250, 250, 250],
			[250, 250, 250],
			[250, 250, 250]
		]::double precision[][]
	) AS rast
)
SELECT 'st_convolution_default_pixeltype',
	ST_BandPixelType(
		ST_Convolution(
			rast,
			ARRAY[
				[1, 1, 1],
				[1, 1, 1],
				[1, 1, 1]
			]::double precision[][]
		)
	)
FROM src;

WITH src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0),
		1, '32BF', 10, NULL
	) AS rast
)
SELECT 'st_convolution_explicit_pixeltype',
	ST_BandPixelType(
		ST_Convolution(
			rast,
			ARRAY[[1]]::double precision[][],
			'64BF'
		)
	)
FROM src;

WITH src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0),
		1, '8BUI', 10, NULL
	) AS rast
)
SELECT 'st_convolution_null_pixeltype',
	ST_BandPixelType(
		ST_Convolution(
			rast,
			ARRAY[[1]]::double precision[][],
			NULL
		)
	)
FROM src;

WITH src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(5, 5, 0, 0, 1, -1, 0, 0, 0),
		1, '32BF', 1, NULL
	) AS rast
)
SELECT 'st_convolution_rejects_5x5';

WITH src AS (
	SELECT ST_AddBand(
		ST_MakeEmptyRaster(5, 5, 0, 0, 1, -1, 0, 0, 0),
		1, '32BF', 1, NULL
	) AS rast
)
SELECT
	ST_Convolution(
		rast,
		ARRAY[
			[1, 1, 1, 1, 1],
			[1, 1, 1, 1, 1],
			[1, 1, 1, 1, 1],
			[1, 1, 1, 1, 1],
			[1, 1, 1, 1, 1]
		]::double precision[][]
	)
FROM src;

DROP FUNCTION IF EXISTS raster_nmapalgebra_test(double precision[], int[], text[]);
DROP TABLE IF EXISTS raster_nmapalgebra_mask_in;
