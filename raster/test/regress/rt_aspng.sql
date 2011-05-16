SELECT md5(
	ST_AsPNG(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BSI', 123, NULL)
	)
);
SELECT md5(
	ST_AsPNG(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BUI', 123, NULL)
	)
);
SELECT md5(
	ST_AsPNG(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BSI', -123, NULL)
	)
);
SELECT md5(
	ST_AsPNG(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BUI', 254, NULL)
	)
);
SELECT md5(
	ST_AsPNG(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '8BSI', 1, -1
				)
				, 2, '8BSI', 11, -1
			)
			, 3, '8BSI', 111, -1
		),
		ARRAY['ZLEVEL=1']
	)
);
SELECT md5(
	ST_AsPNG(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '8BSI', 1, -1
				)
				, 2, '8BSI', 11, -1
			)
			, 3, '8BSI', 111, -1
		),
		ARRAY['ZLEVEL=9']
	)
);
SELECT md5(
	ST_AsPNG(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '8BSI', 1, 1
				)
				, 2, '8BSI', 11, 1
			)
			, 3, '8BSI', 111, 1
		),
		ARRAY['ZLEVEL=9']
	)
);
SELECT md5(
	ST_AsPNG(
		ST_AddBand(
			ST_AddBand(
				ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
				, 1, '8BUI', 1, 1
			)
			, 2, '8BUI', 11, 1
		),
		ARRAY[1],
		6
	)
);
SELECT md5(
	ST_AsPNG(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '8BUI', 1, 1
				)
				, 2, '8BUI', 11, 1
			)
			, 3, '8BUI', 111, 1
		),
		ARRAY[3,1],
		6
	)
);
