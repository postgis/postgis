SELECT count(*), pg_typeof("bad""col")::text FROM loadedrast GROUP BY pg_typeof("bad""col")::text;
SELECT
	position('<Item name="POSTGIS_TEST">metadata</Item>' in "bad""col") > 0,
	position('<Item name="INTERLEAVE" domain="IMAGE_STRUCTURE">PIXEL</Item>' in "bad""col") > 0
FROM loadedrast
ORDER BY rid
LIMIT 1;
