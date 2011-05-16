SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '64BF', 123.4567, NULL),
		'GTiff'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0,-1)
					, 1, '64BF', 1234.5678, NULL
				)
				, '64BF', 987.654321, NULL
			)
			, '64BF', 9876.54321, NULL
		),
		'GTiff'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '64BF', 1234.5678, -9999
				)
				, '64BF', 987.654321, -9999
			)
			, '64BF', 9876.54321, -9999
		),
		'GTiff'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BSI', 123, NULL),
		'PNG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BUI', 123, NULL),
		'PNG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BSI', -123, NULL),
		'PNG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BUI', 254, NULL),
		'PNG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
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
		'PNG',
		ARRAY['ZLEVEL=1']
	)
);
SELECT md5(
	ST_AsGDALRaster(
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
		'PNG',
		ARRAY['ZLEVEL=9']
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BSI', 123, NULL),
		'JPEG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BUI', 123, NULL),
		'JPEG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BSI', -123, NULL),
		'JPEG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '8BUI', 254, NULL),
		'JPEG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
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
		'JPEG'
	)
);
SELECT md5(
	ST_AsGDALRaster(
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
		'JPEG',
		ARRAY['QUALITY=90','PROGRESSIVE=ON']
	)
);
