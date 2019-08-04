--
-- GML
--

-- Empty Geometry
SELECT 'gml_empty_geom', ST_AsGML(geography(ST_GeomFromEWKT('GEOMETRYCOLLECTION EMPTY')));

-- Precision
SELECT 'gml_precision_01', ST_AsGML(geography(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)')), -2);
SELECT 'gml_precision_02', ST_AsGML(geography(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)')), 19);

-- Version
SELECT 'gml_version_01', ST_AsGML(2, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 1)')));
SELECT 'gml_version_02', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 1)')));
SELECT 'gml_version_03', ST_AsGML(21, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 1)')));
SELECT 'gml_version_04', ST_AsGML(-4, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 1)')));

-- Option
SELECT 'gml_option_01', ST_AsGML(2, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 0);
SELECT 'gml_option_02', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 1);
SELECT 'gml_option_03', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 2);
SELECT 'gml_option_04', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 4);

-- These are geometry-only
SELECT 'gml_option_05', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 16);
SELECT 'gml_option_06', ST_AsGML(2, geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 2, 3 4)')), 0, 32);
SELECT 'gml_option_07', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 2, 3 4)')), 0, 32);

-- Deegree data
SELECT 'gml_deegree_01', ST_AsGML(2, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 0);
SELECT 'gml_deegree_02', ST_AsGML(2, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 1);
SELECT 'gml_deegree_03', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 0);
SELECT 'gml_deegree_04', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 1);

-- Prefix
SELECT 'gml_prefix_01', ST_AsGML(2, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 0, '');
SELECT 'gml_prefix_02', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 0, '');
SELECT 'gml_prefix_03', ST_AsGML(2, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 0, 'foo');
SELECT 'gml_prefix_04', ST_AsGML(3, geography(ST_GeomFromEWKT('SRID=4326;POINT(1 2)')), 0, 0, 'foo');

--
-- KML
--

-- SRID
SELECT 'kml_srid_01', ST_AsKML(geography(ST_GeomFromEWKT('SRID=10;POINT(0 1)')));
SELECT 'kml_srid_02', ST_AsKML(geography(ST_GeomFromEWKT('POINT(0 1)')));

-- Empty Geometry
SELECT 'kml_empty_geom', ST_AsKML(geography(ST_GeomFromEWKT(NULL)));

-- Precision
SELECT 'kml_precision_01', ST_AsKML(geography(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)')), -2);
SELECT 'kml_precision_02', ST_AsKML(geography(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)')), 19);

-- Projected -- there's no projected geography

--
-- SVG
--

-- Empty Geometry
SELECT 'svg_empty_geom', ST_AsSVG(geography(ST_GeomFromEWKT(NULL)));

-- Option
SELECT 'svg_option_01', ST_AsSVG(geography(ST_GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)')), 0);
SELECT 'svg_option_02', ST_AsSVG(geography(ST_GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)')), 1);
SELECT 'svg_option_03', ST_AsSVG(geography(ST_GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)')), 0, 0);
SELECT 'svg_option_04', ST_AsSVG(geography(ST_GeomFromEWKT('LINESTRING(1 1, 4 4, 5 7)')), 1, 0);

-- Precision
SELECT 'svg_precision_01', ST_AsSVG(geography(ST_GeomFromEWKT('POINT(1.1111111 1.1111111)')), 1, -2);
SELECT 'svg_precision_02', ST_AsSVG(geography(ST_GeomFromEWKT('POINT(1.1111111 1.1111111)')), 1, 19);

--
-- GeoJSON
--

-- Empty Geometry
SELECT 'geojson_empty_geom', ST_AsGeoJSON(geography(ST_GeomFromEWKT(NULL)));

-- Precision
SELECT 'geojson_precision_01', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)')), -2);
SELECT 'geojson_precision_02', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;POINT(1.1111111 1.1111111)')), 19);

-- CRS
SELECT 'geojson_crs_01', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;POINT(1 1)')), 0, 2);
SELECT 'geojson_crs_02', ST_AsGeoJSON(geography(ST_GeomFromEWKT('POINT(1 1)')), 0, 2);
SELECT 'geojson_crs_03', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;POINT(1 1)')), 0, 4);
SELECT 'geojson_crs_04', ST_AsGeoJSON(geography(ST_GeomFromEWKT('POINT(1 1)')), 0, 4);
SELECT 'geojson_crs_05', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=1;POINT(1 1)')), 0, 2);
SELECT 'geojson_crs_06', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=1;POINT(1 1)')), 0, 4);

-- Bbox
SELECT 'geojson_bbox_01', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0);
SELECT 'geojson_bbox_02', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 1);
SELECT 'geojson_bbox_03', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 3);
SELECT 'geojson_bbox_04', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 5);

-- CRS and Bbox
SELECT 'geojson_options_01', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 0);
SELECT 'geojson_options_02', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0);
SELECT 'geojson_options_03', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 1);
SELECT 'geojson_options_04', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 1);
SELECT 'geojson_options_05', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 2);
SELECT 'geojson_options_06', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 2);
SELECT 'geojson_options_07', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 3);
SELECT 'geojson_options_08', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 3);
SELECT 'geojson_options_09', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 4);
SELECT 'geojson_options_10', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 4);
SELECT 'geojson_options_11', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 5);
SELECT 'geojson_options_12', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 5);
SELECT 'geojson_options_13', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 6);
SELECT 'geojson_options_14', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 6);
SELECT 'geojson_options_15', ST_AsGeoJSON(geography(ST_GeomFromEWKT('LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 7);
SELECT 'geojson_options_16', ST_AsGeoJSON(geography(ST_GeomFromEWKT('SRID=4326;LINESTRING(1 1, 2 2, 3 3, 4 4)')), 0, 7);

--
-- Text
--

-- Precision
SELECT 'text_precision_01', ST_AsText(geography(GeomFromEWKT('SRID=4326;POINT(111.1111111 1.1111111)')));
SELECT 'text_precision_02', ST_AsText(geography(GeomFromEWKT('SRID=4326;POINT(111.1111111 1.1111111)')),2);
