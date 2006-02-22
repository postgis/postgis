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

