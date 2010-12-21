-- Tests for affine transformations

-- translate
select 'translate', asewkt(translate('POINT(0 0)'::geometry, 5, 12));
select 'translate', asewkt(translate('POINT(0 0 0)'::geometry, -3, -7, 3));

-- scale
select 'scale', asewkt(scale('POINT(1 1)'::geometry, 5, 5));
select 'scale', asewkt(scale('POINT(1 1)'::geometry, 3, 2));
select 'scale', asewkt(scale('POINT(10 20 -5)'::geometry, 4, 2, -8));

-- rotateZ
select 'rotateZ', asewkt(SnapToGrid(rotateZ('POINT(1 1)'::geometry, pi()), 0.1));
select 'rotateZ', asewkt(SnapToGrid(rotateZ('POINT(1 1)'::geometry, pi()/2), 0.1));
select 'rotateZ', asewkt(SnapToGrid(rotateZ('POINT(1 1)'::geometry, pi()+pi()/2), 0.1));
select 'rotateZ', asewkt(SnapToGrid(rotateZ('POINT(1 1)'::geometry, 2*pi()), 0.1));

-- rotateY
select 'rotateY', asewkt(SnapToGrid(rotateY('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'rotateY', asewkt(SnapToGrid(rotateY('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'rotateY', asewkt(SnapToGrid(rotateY('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'rotateY', asewkt(SnapToGrid(rotateY('POINT(1 1 1)'::geometry, 2*pi()), 0.1));

-- rotateX
select 'rotateX', asewkt(SnapToGrid(rotateX('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'rotateX', asewkt(SnapToGrid(rotateX('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'rotateX', asewkt(SnapToGrid(rotateX('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'rotateX', asewkt(SnapToGrid(rotateX('POINT(1 1 1)'::geometry, 2*pi()), 0.1));

-- transscale
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(2 2)'::geometry,1, 1, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,-1, -1, -1, -1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,0, 1, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,1, 0, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 0, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 1, 0), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,2, 1, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,1, 2, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 2, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 1, 2), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)'::geometry,2, 3, 5, 7), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1 1)'::geometry,2, 3, 5, 7), 0.1));

-- postgis-users/2006-May/012119.html
select 'transl_bbox', box2d(translate('LINESTRING(0 0, 1 1)'::geometry, 1, 0, 0));
select 'scale_bbox', box2d(scale('LINESTRING(1 0, 2 1)'::geometry, 2, 0));
select 'tscale_bbox', box2d(transscale('LINESTRING(1 0, 2 1)'::geometry, 2, 1, 1, 1));

select 'rotZ_bbox', box2d(SnapToGrid(rotateZ('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));
select 'rotY_bbox', box2d(SnapToGrid(rotateY('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));

-- Repeat all tests with the new function names.
-- translate
select 'translate', ST_asewkt(ST_translate('POINT(0 0)'::geometry, 5, 12));
select 'translate', ST_asewkt(ST_translate('POINT(0 0 0)'::geometry, -3, -7, 3));

-- scale
select 'scale', ST_asewkt(ST_scale('POINT(1 1)'::geometry, 5, 5));
select 'scale', ST_asewkt(ST_scale('POINT(1 1)'::geometry, 3, 2));
select 'scale', ST_asewkt(ST_scale('POINT(10 20 -5)'::geometry, 4, 2, -8));

-- rotateZ
select 'rotateZ', ST_asewkt(ST_SnapToGrid(rotateZ('POINT(1 1)'::geometry, pi()), 0.1));
select 'rotateZ', ST_asewkt(ST_SnapToGrid(rotateZ('POINT(1 1)'::geometry, pi()/2), 0.1));
select 'rotateZ', ST_asewkt(ST_SnapToGrid(rotateZ('POINT(1 1)'::geometry, pi()+pi()/2), 0.1));
select 'rotateZ', ST_asewkt(ST_SnapToGrid(rotateZ('POINT(1 1)'::geometry, 2*pi()), 0.1));

-- rotateY
select 'rotateY', ST_asewkt(ST_SnapToGrid(rotateY('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'rotateY', ST_asewkt(ST_SnapToGrid(rotateY('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'rotateY', ST_asewkt(ST_SnapToGrid(rotateY('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'rotateY', ST_asewkt(ST_SnapToGrid(rotateY('POINT(1 1 1)'::geometry, 2*pi()), 0.1));

-- rotateX
select 'rotateX', ST_asewkt(ST_SnapToGrid(rotateX('POINT(1 1 1)'::geometry, pi()), 0.1));
select 'rotateX', ST_asewkt(ST_SnapToGrid(rotateX('POINT(1 1 1)'::geometry, pi()/2), 0.1));
select 'rotateX', ST_asewkt(ST_SnapToGrid(rotateX('POINT(1 1 1)'::geometry, pi()+pi()/2), 0.1));
select 'rotateX', ST_asewkt(ST_SnapToGrid(rotateX('POINT(1 1 1)'::geometry, 2*pi()), 0.1));

-- transscale
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 1, 1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(2 2)'::geometry,1, 1, 1, 1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,-1, -1, -1, -1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,0, 1, 1, 1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,1, 0, 1, 1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 0, 1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 1, 0), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,2, 1, 1, 1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,1, 2, 1, 1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 2, 1), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,1, 1, 1, 2), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1)'::geometry,2, 3, 5, 7), 0.1));
select 'transscale', ST_asewkt(ST_snapToGrid(transscale('POINT(1 1 1)'::geometry,2, 3, 5, 7), 0.1));

-- postgis-users/2006-May/012119.html
select 'transl_bbox', box2d(ST_translate('LINESTRING(0 0, 1 1)'::geometry, 1, 0, 0));
select 'scale_bbox', box2d(ST_scale('LINESTRING(1 0, 2 1)'::geometry, 2, 0));
select 'tscale_bbox', box2d(ST_transscale('LINESTRING(1 0, 2 1)'::geometry, 2, 1, 1, 1));

select 'rotZ_bbox', box2d(ST_SnapToGrid(rotateZ('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));
select 'rotY_bbox', box2d(ST_SnapToGrid(rotateY('LINESTRING(0 0, 1 0)'::geometry, pi()), 0.1));
