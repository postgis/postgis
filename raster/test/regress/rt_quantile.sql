SELECT
	round(quantile::numeric, 3),
	round(value::numeric, 3)
FROM ST_Quantile(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, 1, FALSE, ARRAY[0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1]::double precision[]
);
SELECT
	round(quantile::numeric, 3),
	round(value::numeric, 3)
FROM ST_Quantile(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, 1, FALSE, ARRAY[0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1]::double precision[]
);
SELECT
	round(quantile::numeric, 3),
	round(value::numeric, 3)
FROM ST_Quantile(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
					, 1, '64BF', 0, NULL
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
	, 1, ARRAY[0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1]::double precision[]
);
SELECT
	round(quantile::numeric, 3),
	round(value::numeric, 3)
FROM ST_Quantile(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
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
	round(quantile::numeric, 3),
	round(value::numeric, 3)
FROM ST_Quantile(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
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
	round(quantile::numeric, 3),
	round(value::numeric, 3)
FROM ST_Quantile(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	),
	ARRAY[0.05, 0.95]::double precision[]
);
SELECT
	round(quantile::numeric, 3),
	round(value::numeric, 3)
FROM ST_Quantile(
	ST_SetValue(
		ST_SetValue(
			ST_SetValue(
				ST_AddBand(
					ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
					, 1, '64BF', 0, 0
				)
				, 1, 1, 1, -10
			)
			, 1, 5, 4, 0
		)
		, 1, 5, 5, 3.14159
	)
);
SELECT round(
	ST_Quantile(
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_AddBand(
						ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
						, 1, '64BF', 0, 0
					)
					, 1, 1, 1, -10
				)
				, 1, 5, 4, 0
			)
			, 1, 5, 5, 3.14159
		),
		1, FALSE, 0.05
	)::numeric, 3
);
SELECT round(
	ST_Quantile(
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_AddBand(
						ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
						, 1, '64BF', 0, 0
					)
					, 1, 1, 1, -10
				)
				, 1, 5, 4, 0
			)
			, 1, 5, 5, 3.14159
		),
		1, 0.95
	)::numeric, 3
);
SELECT round(
	ST_Quantile(
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_AddBand(
						ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
						, 1, '64BF', 0, 0
					)
					, 1, 1, 1, -10
				)
				, 1, 5, 4, 0
			)
			, 1, 5, 5, 3.14159
		),
		FALSE, 0.7
	)::numeric, 3
);
SELECT round(
	ST_Quantile(
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_AddBand(
						ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
						, 1, '64BF', 0, 0
					)
					, 1, 1, 1, -10
				)
				, 1, 5, 4, 0
			)
			, 1, 5, 5, 3.14159
		),
		0.45
	)::numeric, 3
);
SELECT round(
	ST_Quantile(
		ST_SetValue(
			ST_SetValue(
				ST_SetValue(
					ST_AddBand(
						ST_MakeEmptyRaster(10, 10, 10, 10, 2, 2, 0, 0,0)
						, 1, '64BF', 0, 0
					)
					, 1, 1, 1, -10
				)
				, 1, 5, 4, 0
			)
			, 1, 5, 5, 3.14159
		),
		2, 0.45
	)::numeric, 3
);
