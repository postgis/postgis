-- No M value
select '2d',asewkt(locate_along_measure('POINT(1 2)', 1));
select '3dz',asewkt(locate_along_measure('POINT(1 2 3)', 1));

-- Points
select 'PNTM_1',asewkt(locate_along_measure('POINTM(1 2 3)', 1));
select 'PNTM_2',asewkt(locate_along_measure('POINTM(1 2 3)', 3));
select 'PNTM_3',asewkt(locate_between_measures('POINTM(1 2 3)', 2, 3));
select 'PNTM_4',asewkt(locate_between_measures('POINTM(1 2 3)', 3, 4));
select 'PNTM_5',asewkt(locate_between_measures('POINTM(1 2 4.00001)', 3, 4));

-- Multipoints
select 'MPNT_1',asewkt(locate_between_measures('MULTIPOINTM(1 2 2)', 2, 5));
select 'MPNT_2', asewkt(locate_between_measures('MULTIPOINTM(1 2 8, 2 2 5, 2 1 0)', 2, 5));
select 'MPNT_3', asewkt(locate_between_measures('MULTIPOINTM(1 2 8, 2 2 5.1, 2 1 0)', 2, 5));
select 'MPNT_4', asewkt(locate_between_measures('MULTIPOINTM(1 2 8, 2 2 5, 2 1 0)', 4, 8));


-- Linestrings
select 'LINEZM_1', asewkt(locate_between_measures('LINESTRING(0 10 100 0, 0 0 0 10)', 2, 18));
select 'LINEZM_2', asewkt(locate_between_measures('LINESTRING(0 10 0 0, 0 0 100 10)', 2, 18));
select 'LINEZM_3', asewkt(locate_between_measures('LINESTRING(0 10 100 0, 0 0 0 10, 10 0 0 0)', 2, 18));
select 'LINEZM_4', asewkt(locate_between_measures('LINESTRING(0 10 100 0, 0 0 0 20, 10 0 0 0)', 2, 18));
select 'LINEZM_5', asewkt(locate_between_measures('LINESTRING(0 10 100 0, 0 0 0 20, 0 10 10 40, 10 0 0 0)', 2, 18));

