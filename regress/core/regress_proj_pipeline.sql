-- Tests for ST_TransformPipeline()
SELECT '1', ST_AsText(ST_SnapToGrid(ST_TransformPipeline('SRID=4326;POINT(174 -37)'::geometry, '+proj=pipeline +step +proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=tmerc +lat_0=0 +lon_0=173 +k=0.9996 +x_0=1600000 +y_0=10000000 +ellps=GRS80'), 10));

-- also checks deg->rad on input
SELECT '2', ST_AsText(ST_SnapToGrid(ST_TransformPipeline('SRID=4326;POINT(2 49)'::geometry, 'urn:ogc:def:coordinateOperation:EPSG::16031'), 10));

-- error: invalid pipeline definition
SELECT '3', ST_AsText(ST_TransformPipeline('SRID=4326;POINT(0 0)'::geometry, 'bad coordinate transform pipeline'));

-- error: code defines a CRS, not a coordinateOperation
SELECT '4', ST_AsText(ST_TransformPipeline('SRID=4326;POINT(0 0)'::geometry, 'EPSG:2193'));

-- assigning/checking SRID on results
SELECT '5', ST_SRID(ST_TransformPipeline('SRID=4326;POINT(174 -37)'::geometry, '+proj=pipeline +step +proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=tmerc +lat_0=0 +lon_0=173 +k=0.9996 +x_0=1600000 +y_0=10000000 +ellps=GRS80'));
SELECT '6', ST_SRID(ST_TransformPipeline('SRID=4326;POINT(174 -37)'::geometry, '+proj=pipeline +step +proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=tmerc +lat_0=0 +lon_0=173 +k=0.9996 +x_0=1600000 +y_0=10000000 +ellps=GRS80', 12345));

-- GDA2020 is complicated, check it works
SELECT '7', ST_AsText(ST_SnapToGrid(ST_TransformPipeline('SRID=4283;POINT(151.2 -33.8)'::geometry, 'urn:ogc:def:coordinateOperation:EPSG::8048'), 0.1));

-- testing inverse pipelines; also checks rad->deg on results
SELECT '8', ST_AsText(ST_SnapToGrid(ST_InverseTransformPipeline('SRID=32631;POINT(426857 5427937)'::geometry, 'urn:ogc:def:coordinateOperation:EPSG::16031'), 0.1));
SELECT '9', ST_SRID(ST_InverseTransformPipeline('SRID=32631;POINT(426857 5427937)'::geometry, 'urn:ogc:def:coordinateOperation:EPSG::16031', 12345));

