SELECT * FROM ST_SummaryStats(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,-1)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, FALSE
);
SELECT ST_SummaryStats(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,-1)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, FALSE
);
SELECT count FROM ST_SummaryStats(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,-1)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, FALSE
);
SELECT count FROM ST_SummaryStats(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,-1)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, TRUE
);
SELECT mean, stddev FROM ST_SummaryStats(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,-1)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, FALSE
);
SELECT mean, stddev FROM ST_SummaryStats(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,-1)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, TRUE
);
BEGIN;
CREATE TEMP TABLE test
	ON COMMIT DROP AS
	SELECT
		rast.rast
	FROM (
		SELECT ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_AddBand(
						ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,-1)
						, 1, '64BF', 0, 0
					)
					, 1, 1, 1, -10
				)
				, 1, 5, 4, 0
			)
			, 1, 5, 5, 3.14159
		) AS rast
	) AS rast
	FULL JOIN (
		SELECT generate_series(1, 10) AS id
	) AS id
		ON 1 = 1;
SELECT * FROM ST_SummaryStats('test', 'rast', 1, FALSE);
SELECT * FROM ST_SummaryStats('test', 'rast', 1, TRUE);
SELECT * FROM ST_SummaryStats('test', 'rast', 1);
SELECT * FROM ST_SummaryStats('test', 'rast');
ROLLBACK;
