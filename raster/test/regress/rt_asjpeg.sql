SELECT md5(
	ST_AsJPEG(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BSI', 123, NULL)
	)
);
SELECT md5(
	ST_AsJPEG(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BUI', 123, NULL)
	)
);
SELECT md5(
	ST_AsJPEG(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BSI', -123, NULL)
	)
);
SELECT md5(
	ST_AsJPEG(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BUI', 254, NULL)
	)
);
SELECT md5(
	ST_AsJPEG(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '8BUI', 1, 255
				)
				, 2, '8BUI', 11, 0
			)
			, 3, '8BUI', 111, 127
		)
	)
);
SELECT md5(
	ST_AsJPEG(
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
		ARRAY['QUALITY=90','PROGRESSIVE=ON']
	)
);
SELECT md5(
	ST_AsJPEG(
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
		ARRAY[3,1],
		50
	)
);
SELECT md5(
	ST_AsJPEG(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '8BSI', 1, -1
				)
				, 2, '8BSI', 11, -1
			)
			, 3, '8BUI', 111, -1
		),
		ARRAY[3],
		10
	)
);
