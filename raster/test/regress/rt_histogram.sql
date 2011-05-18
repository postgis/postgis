SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
);
SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
	),
	1, FALSE, 0, ARRAY[]::double precision[], FALSE
);
SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
	),
	1, FALSE, 1, FALSE
);
SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
	),
	1, FALSE, 5
);
SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
	),
	1, FALSE
);
SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
	),
	1
);
SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
	),
	1, 0, ARRAY[5]::double precision[], FALSE
);
SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
	),
	1, 3, FALSE
);
SELECT
	round(min::numeric, 3),
	round(max::numeric, 3),
	count,
	round(proportion::numeric, 3)
FROM ST_Histogram(
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
	),
	1, 5
);
