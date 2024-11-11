WITH inp(g,v) AS (
	VALUES
		( ST_Buffer(ST_MakePoint(10,0), 10), 1 ),
		( ST_Buffer(ST_MakePoint(20,0), 10), 2 )
),
agg AS (
	SELECT ST_AsRasterAgg(
		g, -- geometry
		v, -- value to render
		ST_MakeEmptyRaster(0,0,0,0,1.0), -- reference raster, for alignment
		'8BUI', -- pixel type
		99, -- nodata value
		'SUM', -- union operation
		true -- touch
	) r
	FROM inp
)
SELECT
	't1',
	ST_Width(r) w,
	ST_Height(r) h,
	ST_Value(r,'POINT(0 10)', exclude_nodata_value=>false) v0_10,
	ST_Value(r,'POINT(5 0)') v5_0,
	ST_Value(r,'POINT(15 0)') v15_0,
	ST_Value(r,'POINT(25 0)') v25_0
FROM agg;
