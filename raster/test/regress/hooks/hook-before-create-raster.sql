-- Create an overload before postgis_raster defines raster_columns.
-- Its result is a harmless marker for function-resolution tests.
CREATE FUNCTION public._raster_constraint_info_scale(
	rastschema name,
	rasttable name,
	rastcolumn name,
	axis text
)
	RETURNS double precision
	AS 'SELECT -8008.0::double precision'
	LANGUAGE 'sql' IMMUTABLE STRICT;
