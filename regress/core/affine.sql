-- Tests for affine transformations
-- Repeat all tests with the new function names.
-- ST_Translate
select 'ST_Translate', ST_asewkt(ST_Translate('POINT(0 0)'::geometry, 5, 12));
select 'ST_Translate', ST_asewkt(ST_Translate('POINT(0 0 0)'::geometry, -3, -7, 3));
select 'ST_TranslateM', ST_asewkt(ST_Translate(ST_GeomFromEWKT('POINTM(0 0 11)'), 5, 12));
select 'ST_TranslateZM', ST_asewkt(ST_Translate(ST_GeomFromEWKT('POINT(0 0 7 11)'), 5, 12, 3));

-- ST_Scale
select 'ST_Scale', ST_asewkt(ST_Scale('POINT(1 1)'::geometry, 5, 5));
select 'ST_Scale', ST_asewkt(ST_Scale('POINT(1 1)'::geometry, 3, 2));
select 'ST_Scale', ST_asewkt(ST_Scale('POINT(10 20 -5)'::geometry, 4, 2, -8));
SELECT 'ST_ScaleBOX', p && ST_Scale(p,ST_MakePoint(-10,1)) FROM (
  select 'POINT(1 1)'::geometry p ) foo;
select 'ST_ScaleM1', ST_asewkt(ST_Scale('POINT(10 20 -5 3)'::geometry, ST_MakePoint(4, 2, -8)));
select 'ST_ScaleM2', ST_asewkt(ST_Scale('POINT(-2 -1 3 2)'::geometry, ST_MakePointM(-2, 3, 4)));
select 'ST_ScaleM3', ST_asewkt(ST_Scale('POINT(10 20 -5 3)'::geometry, ST_MakePoint(-3, 2, -1, 3)));
select 'ST_ScaleOrigin', st_astext(st_scale('LINESTRING(1 1, 2 2)'::geometry, 'POINT(2 2)'::geometry, 'POINT(1 1)'::geometry));

-- ST_Rotate
select 'ST_Rotate', ST_asewkt(ST_SnapToGrid(ST_Rotate('POINT(1 1)'::geometry, pi()/2, 10.0, 20.0), 0.1));
select 'ST_Rotate', ST_asewkt(ST_SnapToGrid(ST_Rotate('POINT(1 1)'::geometry, -pi()/2, -1.0, 2.0), 0.1));
select 'ST_Rotate', ST_asewkt(ST_SnapToGrid(ST_Rotate('POINT(1 1)'::geometry, pi()/2, 'POINT(10 10)'::geometry), 0.1));
select 'ST_Rotate', ST_asewkt(ST_SnapToGrid(ST_Rotate('POINT(1 1)'::geometry, pi()/2, ST_Centroid('LINESTRING(0 0, 1 0)'::geometry)), 0.1));

-- ST_RotateZ
select 'ST_RotateZ', ST_asewkt(ST_SnapToGrid(ST_RotateZ('POINT(1 1)'::geometry, pi()), 0.1));
select 'ST_RotateZ', ST_asewkt(ST_SnapToGrid(ST_RotateZ('POINT(1 1)'::geometry, pi()/2), 0.1));
select 'ST_RotateZ', ST_asewkt(ST_SnapToGrid(ST_RotateZ('POINT(1 1)'::geometry, pi()+pi()/2), 0.1));
select 'ST_RotateZ', ST_asewkt(ST_SnapToGrid(ST_RotateZ('POINT(1 1)'::geometry, 2*pi()), 0.1));

-- ST_RotateY
select 'ST_RotateY', ST_asewkt(ST_SnapToGrid(ST_RotateY('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'ST_RotateY', ST_asewkt(ST_SnapToGrid(ST_RotateY('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'ST_RotateY', ST_asewkt(ST_SnapToGrid(ST_RotateY('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'ST_RotateY', ST_asewkt(ST_SnapToGrid(ST_RotateY('POINT(1 1 1)'::geometry, 2*pi()), 0.1));
select 'ST_RotateYOrigin', ST_asewkt(ST_SnapToGrid(ST_RotateY('POINT(1 2 3)'::geometry, pi()/2, 'POINT(1 1 1)'::geometry), 0.1));
select 'ST_RotateYOrigin2D', ST_asewkt(ST_SnapToGrid(ST_RotateY('POINT(1 1)'::geometry, pi()/2, 'POINT(1 1 1)'::geometry), 0.1));

-- ST_RotateX
select 'ST_RotateX', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'ST_RotateX', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'ST_RotateX', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'ST_RotateX', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, 2*pi()), 0.1));
select 'ST_RotateXOrigin', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 2 3)'::geometry, pi()/2, 'POINT(1 1 1)'::geometry), 0.1));

-- ST_RotateXYZ
select 'ST_RotateXYZ', ST_asewkt(ST_SnapToGrid(ST_RotateXYZ('POINT(1 0 0)'::geometry, 0, pi()/2, pi()/2), 0.1));
select 'ST_RotateXYZ2D', ST_asewkt(ST_SnapToGrid(ST_RotateXYZ('POINT(0 1)'::geometry, pi()/2, pi()/2, 0), 0.1));
select 'ST_RotateXYZOrigin', ST_asewkt(ST_SnapToGrid(ST_RotateXYZ('POINT(1 2 3)'::geometry, pi()/2, 0, 0, 'POINT(1 1 1)'::geometry), 0.1));
select 'ST_RotateXYZOrigin2D', ST_asewkt(ST_SnapToGrid(ST_RotateXYZ('POINT(1 1)'::geometry, pi()/2, 0, 0, 'POINT(1 1 1)'::geometry), 0.1));
BEGIN;
SET search_path TO pg_catalog;
select 'ST_RotateXYZSchemaOrigin', :schema ST_asewkt(:schema ST_RotateXYZ('POINT(1 0)':: :schema geometry, 0, 0, 0));
ROLLBACK;

-- ST_TransScale
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 1, 1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(2 2)'::geometry,1, 1, 1, 1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,-1, -1, -1, -1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,0, 1, 1, 1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 0, 1, 1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 0, 1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 1, 0), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,2, 1, 1, 1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 2, 1, 1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 2, 1), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 1, 2), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1)'::geometry,2, 3, 5, 7), 0.1));
select 'ST_TransScale', ST_asewkt(ST_snapToGrid(ST_TransScale('POINT(1 1 1)'::geometry,2, 3, 5, 7), 0.1));

-- https://trac.osgeo.org/postgis/ticket/3159
select '#3159', st_summary(st_affine(st_makepoint(1,1),1,0,0,1,0,0));

-- postgis-users/2006-May/012119.html
select 'transl_bbox', box2d(ST_Translate('LINESTRING(0 0, 1 1)'::geometry, 1, 0, 0));
select 'ST_Scale_bbox', box2d(ST_Scale('LINESTRING(1 0, 2 1)'::geometry, 2, 0));
select 'ST_Scale_bbox', box2d(ST_TransScale('LINESTRING(1 0, 2 1)'::geometry, 2, 1, 1, 1));

select 'ST_RotZ_bbox', box2d(ST_SnapToGrid(ST_RotateZ('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));
select 'ST_RotY_bbox', box2d(ST_SnapToGrid(ST_RotateY('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));
