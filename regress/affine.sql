-- Tests for affine transformations

-- translate
select 'translate', asewkt(translate('POINT(0 0)', 5, 12));
select 'translate', asewkt(translate('POINT(0 0 0)', -3, -7, 3));

-- scale
select 'scale', asewkt(scale('POINT(1 1)', 5, 5));
select 'scale', asewkt(scale('POINT(1 1)', 3, 2));
select 'scale', asewkt(scale('POINT(10 20 -5)', 4, 2, -8));

-- rotateZ
select 'rotateZ', asewkt(SnapToGrid(rotateZ('POINT(1 1)', pi()), 0.1));
select 'rotateZ', asewkt(SnapToGrid(rotateZ('POINT(1 1)', pi()/2), 0.1));
select 'rotateZ', asewkt(SnapToGrid(rotateZ('POINT(1 1)', pi()+pi()/2), 0.1));
select 'rotateZ', asewkt(SnapToGrid(rotateZ('POINT(1 1)', 2*pi()), 0.1));

-- rotateY
select 'rotateY', asewkt(SnapToGrid(rotateY('POINT(1 1 1)', pi()), 0.1));
select 'rotateY', asewkt(SnapToGrid(rotateY('POINT(1 1 1)', pi()/2), 0.1));
select 'rotateY', asewkt(SnapToGrid(rotateY('POINT(1 1 1)', pi()+pi()/2), 0.1));
select 'rotateY', asewkt(SnapToGrid(rotateY('POINT(1 1 1)', 2*pi()), 0.1));

-- rotateX
select 'rotateX', asewkt(SnapToGrid(rotateX('POINT(1 1 1)', pi()), 0.1));
select 'rotateX', asewkt(SnapToGrid(rotateX('POINT(1 1 1)', pi()/2), 0.1));
select 'rotateX', asewkt(SnapToGrid(rotateX('POINT(1 1 1)', pi()+pi()/2), 0.1));
select 'rotateX', asewkt(SnapToGrid(rotateX('POINT(1 1 1)', 2*pi()), 0.1));

-- transscale
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',1, 1, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(2 2)',1, 1, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',-1, -1, -1, -1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',0, 1, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',1, 0, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',1, 1, 0, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',1, 1, 1, 0), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',2, 1, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',1, 2, 1, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',1, 1, 2, 1), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',1, 1, 1, 2), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1)',2, 3, 5, 7), 0.1));
select 'transscale', asewkt(snapToGrid(transscale('POINT(1 1 1)',2, 3, 5, 7), 0.1));

-- postgis-users/2006-May/012119.html
select 'transl_bbox', box2d(translate('LINESTRING(0 0, 1 1)'::geometry, 1, 0, 0));
select 'scale_bbox', box2d(scale('LINESTRING(1 0, 2 1)'::geometry, 2, 0));
select 'tscale_bbox', box2d(transscale('LINESTRING(1 0, 2 1)'::geometry, 2, 1, 1, 1));

select 'rotZ_bbox', box2d(SnapToGrid(rotateZ('LINESTRING(0 0, 1 0)', pi()), 0.1));
select 'rotY_bbox', box2d(SnapToGrid(rotateY('LINESTRING(0 0, 1 0)', pi()), 0.1));
