SELECT count(*), pg_typeof(gdal_metadata)::text FROM loadedrast GROUP BY pg_typeof(gdal_metadata)::text;
SELECT
	position('<Item name="POSTGIS_TEST">metadata</Item>' in gdal_metadata) > 0,
	position('<Item name="INTERLEAVE" domain="IMAGE_STRUCTURE">PIXEL</Item>' in gdal_metadata) > 0,
	position('<Item domain="xml:XMP">&lt;x:xmpmeta xmlns:x=&quot;adobe:ns:meta/&quot;&gt;&lt;rdf:RDF/&gt;&lt;/x:xmpmeta&gt;</Item>' in gdal_metadata) > 0,
	position('<Item name="&lt;x:xmpmeta xmlns:x"' in gdal_metadata) = 0
FROM loadedrast
ORDER BY rid
LIMIT 1;
