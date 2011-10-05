SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0),
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0),
	ST_MakeEmptyRaster(1, 1, 0, 0, 1.1, 1.1, 0, 0)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1.1, 1.1, 0, 0),
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0),
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, -0.1, 0)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0),
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0.1)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0),
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, -0.1, 0.1)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0),
	ST_MakeEmptyRaster(1, 1, 1, 1, 1, 1, 0, 0)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, 0, 0),
	ST_MakeEmptyRaster(1, 1, 0.1, 0.1, 1, 1, 0, 0)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, -0.1, 0.1),
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, -0.1, 0.1)
);
SELECT ST_SameAlignment(
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, -0.1, 0.1),
	ST_MakeEmptyRaster(1, 1, 0, 0, 1, 1, -0.1, 0)
);
