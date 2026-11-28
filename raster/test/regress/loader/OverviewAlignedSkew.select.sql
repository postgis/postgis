SELECT 'base', count(*), ST_SameAlignment(rast) FROM loadedrast;
SELECT 'overview', count(*), ST_SameAlignment(rast), min(round(ST_SkewX(rast)::numeric, 1)), min(round(ST_SkewY(rast)::numeric, 1)) FROM o_2_loadedrast;
