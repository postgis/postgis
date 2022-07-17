DO language plpgsql $$DECLARE param_postgis_schema text;
BEGIN
-- check if PostGIS is already installed
param_postgis_schema = (SELECT n.nspname from pg_extension e join pg_namespace n on e.extnamespace = n.oid WHERE extname = 'postgis');

-- if in middle install, it will be the current_schema or what was there already
param_postgis_schema = COALESCE(param_postgis_schema, current_schema());

IF param_postgis_schema != current_schema() THEN
	EXECUTE 'set search_path TO ' || quote_ident(param_postgis_schema);
END IF;

-- PostGIS set search path of functions

--
-- ALTER FUNCTION script
--

EXECUTE 'ALTER FUNCTION  box3d(raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_summary(rast raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_makeemptyraster(width int, height int, upperleftx float8, upperlefty float8, pixelsize float8 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_makeemptyraster(rast raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_band(rast raster, nband int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_band(rast raster, nbands text, delimiter char  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_summarystats(
	rast raster,
	nband int ,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_summarystats(
	rast raster,
	exclude_nodata_value boolean
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxsummarystats(
	rast raster,
	nband int ,
	exclude_nodata_value boolean ,
	sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxsummarystats(
	rast raster,
	nband int,
	sample_percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxsummarystats(
	rast raster,
	exclude_nodata_value boolean,
	sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxsummarystats(
	rast raster,
	sample_percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_summarystats(
	rastertable text,
	rastercolumn text,
	nband integer ,
	exclude_nodata_value boolean ,
	sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_summarystats(
	rastertable text,
	rastercolumn text,
	nband integer ,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_summarystats(
	rastertable text,
	rastercolumn text,
	exclude_nodata_value boolean
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxsummarystats(
	rastertable text,
	rastercolumn text,
	nband integer ,
	exclude_nodata_value boolean ,
	sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxsummarystats(
	rastertable text,
	rastercolumn text,
	nband integer,
	sample_percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxsummarystats(
	rastertable text,
	rastercolumn text,
	exclude_nodata_value boolean
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxsummarystats(
	rastertable text,
	rastercolumn text,
	sample_percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_count(rast raster, nband int , exclude_nodata_value boolean , sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_count(rast raster, nband int , exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_count(rast raster, exclude_nodata_value boolean ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxcount(rast raster, nband int , exclude_nodata_value boolean , sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxcount(rast raster, nband int, sample_percent double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxcount(rast raster, exclude_nodata_value boolean, sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxcount(rast raster, sample_percent double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_countagg_finalfn(agg agg_count ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  __st_countagg_transfn(
	agg agg_count,
	rast raster,
 	nband integer , exclude_nodata_value boolean ,
	sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_countagg_transfn(
	agg agg_count,
	rast raster,
 	nband integer, exclude_nodata_value boolean,
	sample_percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_countagg_transfn(
	agg agg_count,
	rast raster,
 	nband integer, exclude_nodata_value boolean
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_countagg_transfn(
	agg agg_count,
	rast raster,
 	exclude_nodata_value boolean
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_count(rastertable text, rastercolumn text, nband integer , exclude_nodata_value boolean , sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_count(rastertable text, rastercolumn text, nband int , exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_count(rastertable text, rastercolumn text, exclude_nodata_value boolean ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxcount(rastertable text, rastercolumn text, nband int , exclude_nodata_value boolean , sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxcount(rastertable text, rastercolumn text, nband int, sample_percent double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxcount(rastertable text, rastercolumn text, exclude_nodata_value boolean, sample_percent double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxcount(rastertable text, rastercolumn text, sample_percent double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_histogram(
	rast raster, nband int,
	exclude_nodata_value boolean,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_histogram(
	rast raster, nband int,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rast raster, nband int,
	exclude_nodata_value boolean,
	sample_percent double precision,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rast raster, nband int,
	sample_percent double precision,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rast raster,
	sample_percent double precision,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rast raster, nband int,
	sample_percent double precision,
	bins int, width double precision[] ,
	right boolean ,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rast raster, nband int,
	sample_percent double precision,
	bins int, right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_histogram(
	rastertable text, rastercolumn text, nband int,
	exclude_nodata_value boolean,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_histogram(
	rastertable text, rastercolumn text, nband int,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rastertable text, rastercolumn text, nband int,
	exclude_nodata_value boolean,
	sample_percent double precision,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rastertable text, rastercolumn text, nband int,
	sample_percent double precision,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rastertable text, rastercolumn text,
	sample_percent double precision,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rastertable text, rastercolumn text, nband int,
	sample_percent double precision,
	bins int, width double precision[] ,
	right boolean ,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxhistogram(
	rastertable text, rastercolumn text, nband int,
	sample_percent double precision,
	bins int,
	right boolean,
	OUT min double precision,
	OUT max double precision,
	OUT count bigint,
	OUT percent double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(
	rast raster,
	nband int,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(
	rast raster,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(rast raster, nband int, exclude_nodata_value boolean, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(rast raster, nband int, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(rast raster, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxquantile(
	rast raster,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxquantile(rast raster, nband int, exclude_nodata_value boolean, sample_percent double precision, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxquantile(rast raster, nband int, sample_percent double precision, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxquantile(rast raster, sample_percent double precision, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(
	rastertable text,
	rastercolumn text,
	nband int,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(
	rastertable text,
	rastercolumn text,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(rastertable text, rastercolumn text, nband int, exclude_nodata_value boolean, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(rastertable text, rastercolumn text, nband int, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_quantile(rastertable text, rastercolumn text, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxquantile(
	rastertable text,
	rastercolumn text,
	quantiles double precision[],
	OUT quantile double precision,
	OUT value double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxquantile(rastertable text, rastercolumn text, nband int, exclude_nodata_value boolean, sample_percent double precision, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxquantile(rastertable text, rastercolumn text, nband int, sample_percent double precision, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_approxquantile(rastertable text, rastercolumn text, sample_percent double precision, quantile double precision ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuecount(rast raster, nband integer, exclude_nodata_value boolean, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuecount(rast raster, nband integer, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuecount(rast raster, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuepercent(rast raster, nband integer, exclude_nodata_value boolean, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuepercent(rast raster, nband integer, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuepercent(rast raster, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuecount(rastertable text, rastercolumn text, nband integer, exclude_nodata_value boolean, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuecount(rastertable text, rastercolumn text, nband integer, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuecount(rastertable text, rastercolumn text, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuepercent(rastertable text, rastercolumn text, nband integer, exclude_nodata_value boolean, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuepercent(rastertable text, rastercolumn text, nband integer, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_valuepercent(rastertable text, rastercolumn text, searchvalue double precision, roundto double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_reclass(rast raster, VARIADIC reclassargset reclassarg[] ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_reclass(rast raster, reclassexpr text, pixeltype text ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_colormap(
	rast raster, nband int ,
	colormap text ,
	method text  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_colormap(
	rast raster,
	colormap text,
	method text  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_astiff(rast raster, options text[] , srid integer  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_astiff(rast raster, compression text, srid integer  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_asjpeg(rast raster, options text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_asjpeg(rast raster, nbands int[], quality int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_asjpeg(rast raster, nband int, quality int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_aspng(rast raster, options text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_aspng(rast raster, nbands int[], compression int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_aspng(rast raster, nband int, compression int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_asraster(
	geom geometry,
	ref raster,
	pixeltype text[] ,
	value double precision[] ,
	nodataval double precision[] ,
	touched boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_resample(
	rast raster,
	ref raster,
	algorithm text ,
	maxerr double precision ,
	usescale boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_resample(
	rast raster,
	ref raster,
	usescale boolean,
	algorithm text ,
	maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_transform(rast raster, srid integer, algorithm text , maxerr double precision , scalex double precision , scaley double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_transform(rast raster, srid integer, scalex double precision, scaley double precision, algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_transform(rast raster, srid integer, scalexy double precision, algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_transform(
	rast raster,
	alignto raster,
	algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_rescale(rast raster, scalex double precision, scaley double precision, algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_rescale(rast raster, scalexy double precision, algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_reskew(rast raster, skewx double precision, skewy double precision, algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_reskew(rast raster, skewxy double precision, algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_snaptogrid(
	rast raster,
	gridx double precision, gridy double precision,
	algorithm text , maxerr double precision ,
	scalex double precision , scaley double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_snaptogrid(
	rast raster,
	gridx double precision, gridy double precision,
	scalex double precision, scaley double precision,
	algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_snaptogrid(
	rast raster,
	gridx double precision, gridy double precision,
	scalexy double precision,
	algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_resize(
	rast raster,
	width text, height text,
	algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_resize(
	rast raster,
	width integer, height integer,
	algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_resize(
	rast raster,
	percentwidth double precision, percentheight double precision,
	algorithm text , maxerr double precision  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_max4ma(matrix float[][], nodatamode text, variadic args text[] ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_min4ma(matrix float[][], nodatamode text, variadic args text[] ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_sum4ma(matrix float[][], nodatamode text, variadic args text[] ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_mean4ma(matrix float[][], nodatamode text, variadic args text[] ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_range4ma(matrix float[][], nodatamode text, variadic args text[] ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_mapalgebra(
	rast raster, nband int[],
	callbackfunc regprocedure,
	pixeltype text ,
	extenttype text , customextent raster ,
	distancex integer , distancey integer ,
	VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_convertarray4ma(value double precision[][] ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_max4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_min4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_sum4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_mean4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_range4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_invdistweight4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_mindist4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_slope4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_slope(
	rast raster, nband integer,
	customextent raster,
	pixeltype text , units text ,
	scale double precision ,	interpolate_nodata boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_aspect4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_aspect(
	rast raster, nband integer,
	customextent raster,
	pixeltype text , units text ,
	interpolate_nodata boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_hillshade4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_hillshade(
	rast raster, nband integer,
	customextent raster,
	pixeltype text ,
	azimuth double precision , altitude double precision ,
	max_bright double precision , scale double precision ,
	interpolate_nodata boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_tpi4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_tpi(
	rast raster, nband integer,
	customextent raster,
	pixeltype text , interpolate_nodata boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_roughness4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_roughness(
	rast raster, nband integer,
	customextent raster,
	pixeltype text , interpolate_nodata boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_tri4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_tri(
	rast raster, nband integer,
	customextent raster,
	pixeltype text ,	interpolate_nodata boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_grayscale4ma(value double precision[][][], pos integer[][], VARIADIC userargs text[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_grayscale(
	rastbandargset rastbandarg[],
	extenttype text  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_grayscale(
	rast raster,
 	redband integer ,
 	greenband integer ,
 	blueband integer ,
	extenttype text  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_bandisnodata(rast raster, forceChecking boolean ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_BandMetaData(
	rast raster,
	band int ,
	OUT pixeltype text,
	OUT nodatavalue double precision,
	OUT isoutdb boolean,
	OUT path text,
	OUT outdbbandnum integer,
	OUT filesize bigint,
	OUT filetimestamp bigint
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_value(rast raster, x integer, y integer, exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_value(rast raster, band integer, pt geometry, exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_Value(rast raster, pt geometry, exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_PixelofValue(
	rast raster,
	search double precision[],
	exclude_nodata_value boolean ,
	OUT val double precision,
	OUT x integer,
	OUT y integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_pixelofvalue(
	rast raster,
	nband integer,
	search double precision,
	exclude_nodata_value boolean ,
	OUT x integer,
	OUT y integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_pixelofvalue(
	rast raster,
	search double precision,
	exclude_nodata_value boolean ,
	OUT x integer,
	OUT y integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_georeference(rast raster, format text  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_setgeoreference(rast raster, georef text, format text  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_setgeoreference(
	rast raster,
	upperleftx double precision, upperlefty double precision,
	scalex double precision, scaley double precision,
	skewx double precision, skewy double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_setbandindex(rast raster, band integer, outdbindex integer, force boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_SetValues(
	rast raster, nband integer,
	x integer, y integer,
	width integer, height integer,
	newvalue double precision,
	keepnodata boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_SetValues(
	rast raster,
	x integer, y integer,
	width integer, height integer,
	newvalue double precision,
	keepnodata boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_dumpvalues(rast raster, nband integer, exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_pixelaspolygons(
	rast raster,
	band integer ,
	exclude_nodata_value boolean ,
	OUT geom geometry,
	OUT val double precision,
	OUT x int,
	OUT y int
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_pixelaspolygon(rast raster, x integer, y integer ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_pixelaspoints(
	rast raster,
	band integer ,
	exclude_nodata_value boolean ,
	OUT geom geometry,
	OUT val double precision,
	OUT x int,
	OUT y int
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_pixelaspoint(rast raster, x integer, y integer ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_pixelascentroids(
	rast raster,
	band integer ,
	exclude_nodata_value boolean ,
	OUT geom geometry,
	OUT val double precision,
	OUT x int,
	OUT y int
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_pixelascentroid(rast raster, x integer, y integer ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_worldtorastercoord(
	rast raster,
	longitude double precision, latitude double precision,
	OUT columnx integer,
	OUT rowy integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_worldtorastercoord(
	rast raster, pt geometry,
	OUT columnx integer,
	OUT rowy integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_worldtorastercoordx(rast raster, xw float8, yw float8 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_worldtorastercoordx(rast raster, xw float8 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_worldtorastercoordx(rast raster, pt geometry ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_worldtorastercoordy(rast raster, xw float8, yw float8 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_worldtorastercoordy(rast raster, yw float8 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_worldtorastercoordy(rast raster, pt geometry ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_rastertoworldcoord(
	rast raster,
	columnx integer, rowy integer,
	OUT longitude double precision,
	OUT latitude double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_rastertoworldcoordx(rast raster, xr int, yr int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_rastertoworldcoordx(rast raster, xr int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_rastertoworldcoordy(rast raster, xr int, yr int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_rastertoworldcoordy(rast raster, yr int ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_asbinary(raster, outasin boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_hash(raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_eq(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_overleft(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_overright(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_left(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_right(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_overabove(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_overbelow(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_above(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_below(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_same(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_contained(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_contain(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_overlap(raster, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_geometry_contain(raster, geometry ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_contained_by_geometry(raster, geometry ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  raster_geometry_overlap(raster, geometry ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  geometry_raster_contain(geometry, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  geometry_contained_by_raster(geometry, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  geometry_raster_overlap(geometry, raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_samealignment(
	ulx1 double precision, uly1 double precision, scalex1 double precision, scaley1 double precision, skewx1 double precision, skewy1 double precision,
	ulx2 double precision, uly2 double precision, scalex2 double precision, scaley2 double precision, skewx2 double precision, skewy2 double precision
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_samealignment_transfn(agg agg_samealignment, rast raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _ST_samealignment_finalfn(agg agg_samealignment ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_iscoveragetile(rast raster, coverage raster, tilewidth integer, tileheight integer ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _st_intersects(geom geometry, rast raster, nband integer  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_Intersection(geomin geometry, rast raster, band integer  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_intersection(rast raster, band integer, geomin geometry ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_Intersection(rast raster, geomin geometry ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_Intersection(
	rast1 raster, band1 int,
	rast2 raster, band2 int,
	returnband text ,
	nodataval double precision[]  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_clip(
	rast raster, nband integer[],
	geom geometry,
	nodataval double precision[] , crop boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_nearestvalue(
	rast raster,
	pt geometry,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_nearestvalue(
	rast raster, band integer,
	columnx integer, rowy integer,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_nearestvalue(
	rast raster,
	columnx integer, rowy integer,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_neighborhood(
	rast raster, band integer,
	columnx integer, rowy integer,
	distancex integer, distancey integer,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_neighborhood(
	rast raster,
	columnx integer, rowy integer,
	distancex integer, distancey integer,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_neighborhood(
	rast raster, band integer,
	pt geometry,
	distancex integer, distancey integer,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_neighborhood(
	rast raster,
	pt geometry,
	distancex integer, distancey integer,
	exclude_nodata_value boolean  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_srid(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_scale(rastschema name, rasttable name, rastcolumn name, axis char ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_blocksize(rastschema name, rasttable name, rastcolumn name, axis text ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_extent(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_alignment(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_spatially_unique(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_coverage_tile(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_regular_blocking(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_num_bands(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_pixel_types(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_pixel_types(rast raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_nodata_values(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_nodata_values(rast raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_out_db(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_out_db(rast raster ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _raster_constraint_info_index(rastschema name, rasttable name, rastcolumn name ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _overview_constraint_info(
	ovschema name, ovtable name, ovcolumn name,
	OUT refschema name, OUT reftable name, OUT refcolumn name, OUT factor integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  _UpdateRasterSRID(
	schema_name name, table_name name, column_name name,
	new_srid integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  UpdateRasterSRID(
	schema_name name, table_name name, column_name name,
	new_srid integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  UpdateRasterSRID(
	table_name name, column_name name,
	new_srid integer
 ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_Retile(tab regclass, col name, ext geometry, sfx float8, sfy float8, tw int, th int, algo text  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  ST_CreateOverview(tab regclass, col name, factor int, algo text  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
EXECUTE 'ALTER FUNCTION  st_makeemptycoverage(tilewidth int, tileheight int, width int, height int, upperleftx float8, upperlefty float8, scalex float8, scaley float8, skewx float8, skewy float8, srid int4  ) SET search_path=' || quote_ident(param_postgis_schema) || ',pg_catalog;';
END;$$;