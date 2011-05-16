SELECT md5(
	ST_AsTIFF(
		ST_AddBand(ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1), 1, '64BF', 123.4567, NULL)
	)
);
SELECT md5(
	ST_AsTIFF(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0,-1)
					, 1, '64BF', 1234.5678, NULL
				)
				, '64BF', 987.654321, NULL
			)
			, '64BF', 9876.54321, NULL
		)
	)
);
SELECT md5(
	ST_AsTIFF(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '64BF', 1234.5678, -9999
				)
				, '64BF', 987.654321, -9999
			)
			, '64BF', 9876.54321, -9999
		)
	)
);
SELECT md5(
	ST_AsTIFF(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '64BF', 1234.5678, -9999
				)
				, '64BF', 987.654321, -9999
			)
			, '64BF', 9876.54321, -9999
		)
		, ARRAY[3]
	)
);
SELECT md5(
	ST_AsTIFF(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '64BF', 1234.5678, -9999
				)
				, '64BF', 987.654321, -9999
			)
			, '64BF', 9876.54321, -1
		)
	)
);
SELECT md5(
	ST_AsTIFF(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '64BF', 1234.5678, -1
				)
				, '64BF', 987.654321, -9999
			)
			, '64BF', 9876.54321, -9999
		)
	)
);
SELECT md5(
	ST_AsTIFF(
		ST_AddBand(
			ST_AddBand(
				ST_AddBand(
					ST_MakeEmptyRaster(200, 200, 10, 10, 2, 2, 0, 0, -1)
					, 1, '64BF', 1234.5678, -9999
				)
				, '64BF', 987.654321, -9999
			)
			, '64BF', 9876.54321, -1
		)
		, 'JPEG90'
	)
);
