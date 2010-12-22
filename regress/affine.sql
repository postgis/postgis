-- Tests for affine transformations

-- ST_Translate
select 'ST_Translate', asewkt(ST_Translate('POINT(0 0)'::geometry, 5, 12));
select 'ST_Translate', asewkt(ST_Translate('POINT(0 0 0)'::geometry, -3, -7, 3));

-- ST_Scale
select 'ST_Scale', asewkt(ST_Scale('POINT(1 1)'::geometry, 5, 5));
select 'ST_Scale', asewkt(ST_Scale('POINT(1 1)'::geometry, 3, 2));
select 'ST_Scale', asewkt(ST_Scale('POINT(10 20 -5)'::geometry, 4, 2, -8));

-- ST_RotateZ
select 'ST_RotateZ', asewkt(SnapToGrid(ST_RotateZ('POINT(1 1)'::geometry, pi()), 0.1));
select 'ST_RotateZ', asewkt(SnapToGrid(ST_RotateZ('POINT(1 1)'::geometry, pi()/2), 0.1));
select 'ST_RotateZ', asewkt(SnapToGrid(ST_RotateZ('POINT(1 1)'::geometry, pi()+pi()/2), 0.1));
select 'ST_RotateZ', asewkt(SnapToGrid(ST_RotateZ('POINT(1 1)'::geometry, 2*pi()), 0.1));

-- ST_RotateY
select 'ST_RotateY', asewkt(SnapToGrid(ST_RotateY('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'ST_RotateY', asewkt(SnapToGrid(ST_RotateY('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'ST_RotateY', asewkt(SnapToGrid(ST_RotateY('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'ST_RotateY', asewkt(SnapToGrid(ST_RotateY('POINT(1 1 1)'::geometry, 2*pi()), 0.1));

-- ST_RotateX
select 'ST_RotateX', asewkt(SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'ST_RotateX', asewkt(SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'ST_RotateX', asewkt(SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'ST_RotateX', asewkt(SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, 2*pi()), 0.1));

-- ST_TransScale
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 1, 1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(2 2)'::geometry,1, 1, 1, 1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,-1, -1, -1, -1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,0, 1, 1, 1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 0, 1, 1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 0, 1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 1, 0), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,2, 1, 1, 1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 2, 1, 1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 2, 1), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,1, 1, 1, 2), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1)'::geometry,2, 3, 5, 7), 0.1));
select 'ST_TransScale', asewkt(snapToGrid(ST_TransScale('POINT(1 1 1)'::geometry,2, 3, 5, 7), 0.1));

-- postgis-users/2006-May/012119.html
select 'transl_bbox', box2d(ST_Translate('LINESTRING(0 0, 1 1)'::geometry, 1, 0, 0));
select 'ST_Scale_bbox', box2d(ST_Scale('LINESTRING(1 0, 2 1)'::geometry, 2, 0));
select 'ST_Scale_bbox', box2d(ST_TransScale('LINESTRING(1 0, 2 1)'::geometry, 2, 1, 1, 1));

select 'ST_RotZ_bbox', box2d(SnapToGrid(ST_RotateZ('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));
select 'ST_RotY_bbox', box2d(SnapToGrid(ST_RotateY('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));

-- Repeat all tests with the new function names.
-- ST_Translate
select 'ST_Translate', ST_asewkt(ST_Translate('POINT(0 0)'::geometry, 5, 12));
select 'ST_Translate', ST_asewkt(ST_Translate('POINT(0 0 0)'::geometry, -3, -7, 3));

-- ST_Scale
select 'ST_Scale', ST_asewkt(ST_Scale('POINT(1 1)'::geometry, 5, 5));
select 'ST_Scale', ST_asewkt(ST_Scale('POINT(1 1)'::geometry, 3, 2));
select 'ST_Scale', ST_asewkt(ST_Scale('POINT(10 20 -5)'::geometry, 4, 2, -8));

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

-- ST_RotateX
select 'ST_RotateX', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'ST_RotateX', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'ST_RotateX', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'ST_RotateX', ST_asewkt(ST_SnapToGrid(ST_RotateX('POINT(1 1 1)'::geometry, 2*pi()), 0.1));

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

-- postgis-users/2006-May/012119.html
select 'transl_bbox', box2d(ST_Translate('LINESTRING(0 0, 1 1)'::geometry, 1, 0, 0));
select 'ST_Scale_bbox', box2d(ST_Scale('LINESTRING(1 0, 2 1)'::geometry, 2, 0));
select 'ST_Scale_bbox', box2d(ST_TransScale('LINESTRING(1 0, 2 1)'::geometry, 2, 1, 1, 1));

select 'ST_RotZ_bbox', box2d(ST_SnapToGrid(ST_RotateZ('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));
select 'ST_RotY_bbox', box2d(ST_SnapToGrid(ST_RotateY('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));
