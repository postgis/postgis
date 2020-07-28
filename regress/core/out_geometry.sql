--
-- GML
--

-- Empty Geometry
SELECT 'gml_empty_geom', ST_AsGML(ST_GeomFromEWKT(NULL));

-- Precision
SELECT 'gml_precision_01', ST_AsGML(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), -2);
SELECT 'gml_precision_02', ST_AsGML(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), 19);

-- Version
SELECT 'gml_version_01', ST_AsGML(2, ST_GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'gml_version_02', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'gml_version_03', ST_AsGML(21, ST_GeomFromEWKT('SRID=4326;POINT(1 1)'));
SELECT 'gml_version_04', ST_AsGML(-4, ST_GeomFromEWKT('SRID=4326;POINT(1 1)'));

-- Option
SELECT 'gml_option_01', ST_AsGML(2, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 0);
SELECT 'gml_option_02', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 1);
SELECT 'gml_option_03', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 2);
SELECT 'gml_option_04', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 4);
SELECT 'gml_option_05', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16);
SELECT 'gml_option_06', ST_AsGML(2, ST_GeomFromEWKT('SRID=4326;LINESTRING(1 2, 3 4)'), 0, 32);
SELECT 'gml_option_07', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;LINESTRING(1 2, 3 4)'), 0, 32);

-- Deegree data
SELECT 'gml_deegree_01', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 0);
SELECT 'gml_deegree_02', ST_AsGML(2, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16);
SELECT 'gml_deegree_03', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16);
SELECT 'gml_deegree_04', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2 3)'), 0, 0);
SELECT 'gml_deegree_05', ST_AsGML(2, ST_GeomFromEWKT('SRID=4326;POINT(1 2 3)'), 0, 16);
SELECT 'gml_deegree_06', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2 3)'), 0, 16);

-- Prefix
SELECT 'gml_prefix_01', ST_AsGML(2, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16, '');
SELECT 'gml_prefix_02', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16, '');
SELECT 'gml_prefix_03', ST_AsGML(2, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16, 'foo');
SELECT 'gml_prefix_04', ST_AsGML(3, ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 16, 'foo');

-- LineString
SELECT 'gml_shortline_01', ST_AsGML(3, ST_GeomFromEWKT('LINESTRING(1 2, 3 4)'), 0, 6, '');
SELECT 'gml_shortline_02', ST_AsGML(3, ST_GeomFromEWKT('LINESTRING(1 2, 3 4)'), 0, 2, '');
SELECT 'gml_shortline_03', ST_AsGML(3, ST_GeomFromEWKT('MULTILINESTRING((1 2, 3 4), (5 6, 7 8))'), 0, 6, '');
SELECT 'gml_shortline_04', ST_AsGML(3, ST_GeomFromEWKT('MULTILINESTRING((1 2, 3 4), (5 6, 7 8))'), 0, 2, '');

-- CIRCULARSTRING / COMPOUNDCURVE / CURVEPOLYGON / MULTISURFACE / MULTICURVE
SELECT 'gml_out_curve_01', ST_AsGML(3, ST_GeomFromEWKT('CIRCULARSTRING(-2 0,0 2,2 0,0 2,2 4)'));
SELECT 'gml_out_curve_02', ST_ASGML(3, ST_GeomFromEWKT('COMPOUNDCURVE(CIRCULARSTRING(0 0,1 1,1 0),(1 0,0 1))'));
SELECT 'gml_out_curve_03', ST_AsGML(3, ST_GeomFromEWKT('CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0))'));
SELECT 'gml_out_curve_04', ST_AsGML(3, ST_GeomFromEWKT('MULTICURVE((5 5,3 5,3 3,0 3),CIRCULARSTRING(0 0,2 1,2 2))'));
SELECT 'gml_out_curve_05', ST_AsGML(3, ST_GeomFromEWKT('MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))'));

--
-- KML
--

-- SRID
SELECT 'kml_srid_01', ST_AsKML(ST_GeomFromEWKT('SRID=10;POINT(0 1)'));
SELECT 'kml_srid_02', ST_AsKML(ST_GeomFromEWKT('POINT(0 1)'));

-- Empty Geometry
SELECT 'kml_empty_geom', ST_AsKML(ST_GeomFromEWKT(NULL));

-- Precision
SELECT 'kml_precision_01', ST_AsKML(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), -2);
SELECT 'kml_precision_02', ST_AsKML(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), 19);

-- Prefix
SELECT 'kml_prefix_01', ST_AsKML(ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, '');
SELECT 'kml_prefix_02', ST_AsKML(ST_GeomFromEWKT('SRID=4326;POINT(1 2)'), 0, 'kml');

-- Projected
-- National Astronomical Observatory of Colombia - Bogota, Colombia (Placemark)
SELECT 'kml_projection_01', ST_AsKML(ST_GeomFromEWKT('SRID=3116;POINT(1000000 1000000)'), 3);

--
-- Encoded Polyline
--
SELECT 'encoded_polyline_01', ST_AsEncodedPolyline(ST_GeomFromEWKT('SRID=4326;LINESTRING(-120.2 38.5,-120.95 40.7,-126.453 43.252)'));
SELECT 'encoded_polyline_02', ST_AsEncodedPolyline(ST_GeomFromEWKT('SRID=4326;MULTIPOINT(-120.2 38.5,-120.95 40.7,-126.453 43.252)'));
SELECT 'encoded_polyline_03', ST_AsEncodedPolyline(ST_GeomFromEWKT('LINESTRING(-120.2 38.5,-120.95 40.7,-126.453 43.252)'));
SELECT 'encoded_polyline_04', ST_AsEncodedPolyline(ST_GeomFromEWKT('SRID=4326;LINESTRING(-120.234467 38.5,-120.95 40.7343495,-126.453 43.252)'), 6);

--
-- SVG
--

-- Empty Geometry
SELECT 'svg_empty_geom', ST_AsSVG(ST_GeomFromEWKT(NULL));

-- Option
SELECT 'svg_option_01', ST_AsSVG(ST_GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)'), 0);
SELECT 'svg_option_02', ST_AsSVG(ST_GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)'), 1);
SELECT 'svg_option_03', ST_AsSVG(ST_GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)'), 0, 0);
SELECT 'svg_option_04', ST_AsSVG(ST_GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)'), 1, 0);

-- Precision
SELECT 'svg_precision_01', ST_AsSVG(ST_GeomFromEWKT('POINT(1.1111111 1.1111111)'), 1, -2);
SELECT 'svg_precision_02', ST_AsSVG(ST_GeomFromEWKT('POINT(1.1111111 1.1111111)'), 1, 19);

--
-- GeoJSON
--

-- Empty Geometry
SELECT 'geojson_empty_geom', ST_AsGeoJSON(ST_GeomFromEWKT(NULL));

-- Precision
SELECT 'geojson_precision_01', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), -2);
SELECT 'geojson_precision_02', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)'), 19);

-- Version
SELECT 'geojson_version_01', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;POINT(1 1)'));

-- CRS
SELECT 'geojson_crs_01', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;POINT(1 1)'), 0, 2);
SELECT 'geojson_crs_02', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;POINT(1 1)'), 0, 2);
SELECT 'geojson_crs_03', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;POINT(1 1)'), 0, 4);
SELECT 'geojson_crs_04', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;POINT(1 1)'), 0, 4);
SELECT 'geojson_crs_05', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=1;POINT(1 1)'), 0, 2);
SELECT 'geojson_crs_06', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=1;POINT(1 1)'), 0, 4);
SELECT 'geojson_crs_07', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=3005;POINT(1 1)'));

-- Bbox
SELECT 'geojson_bbox_01', ST_AsGeoJSON(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0);
SELECT 'geojson_bbox_02', ST_AsGeoJSON(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 1);
SELECT 'geojson_bbox_03', ST_AsGeoJSON(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 3);
SELECT 'geojson_bbox_04', ST_AsGeoJSON(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 5);

-- CRS and Bbox
SELECT 'geojson_options_01', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 0);
SELECT 'geojson_options_02', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0);
SELECT 'geojson_options_03', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 1);
SELECT 'geojson_options_04', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 1);
SELECT 'geojson_options_05', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 2);
SELECT 'geojson_options_06', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 2);
SELECT 'geojson_options_07', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 3);
SELECT 'geojson_options_08', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 3);
SELECT 'geojson_options_09', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 4);
SELECT 'geojson_options_10', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 4);
SELECT 'geojson_options_11', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 5);
SELECT 'geojson_options_12', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 5);
SELECT 'geojson_options_13', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 6);
SELECT 'geojson_options_14', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 6);
SELECT 'geojson_options_15', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=0;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 7);
SELECT 'geojson_options_16', ST_AsGeoJSON(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)'), 0, 7);

-- Out and in to PostgreSQL native geometric types
WITH p AS ( SELECT '((0,0),(0,1),(1,1),(1,0),(0,0))'::text AS p )
  SELECT 'pgcast_01', p = p::polygon::geometry::polygon::text FROM p;
WITH p AS ( SELECT '[(0,0),(1,1)]'::text AS p )
  SELECT 'pgcast_02', p = p::path::geometry::path::text FROM p;
WITH p AS ( SELECT '(1,1)'::text AS p )
  SELECT 'pgcast_03', p = p::point::geometry::point::text FROM p;
SELECT 'pgcast_03','POLYGON EMPTY'::geometry::polygon IS NULL;
SELECT 'pgcast_04','LINESTRING EMPTY'::geometry::path IS NULL;
SELECT 'pgcast_05','POINT EMPTY'::geometry::point IS NULL;
SELECT 'pgcast_06',ST_AsText('((0,0),(0,1),(1,1),(1,0))'::polygon::geometry);


--
-- Text
--

-- Precision
SELECT 'text_precision_01', ST_AsText(GeomFromEWKT('SRID=4326;POINT(111.1111111 1.1111111)'));
SELECT 'text_precision_02', ST_AsText(GeomFromEWKT('SRID=4326;POINT(111.1111111 1.1111111)'),2);

--
-- ST_AsEWKT
--
SELECT 'EWKT_' || i, ST_AsEWKT('SRID=4326;POINT(12345678.123456789 1)'::geometry, i)
FROM generate_series(0, 20) AS t(i)
ORDER BY i;