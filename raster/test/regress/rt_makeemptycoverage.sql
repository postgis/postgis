-- make an empty coverage with equal tiles (all 5x5)
select '1', st_metadata(st_makeemptycoverage(5, 5, 10, 10, 0, 0, 1, -1, 0, 0, 0));
-- make an empty coverage with unequal coverage (4x4, 3x4, 4x3, 3x3)
select '2', st_metadata(st_makeemptycoverage(4, 4, 7, 7, 0, 0, 1, -1, 0, 0, 0));
-- make an empty coverage with equal coverage and scaled
select '3', st_metadata(st_makeemptycoverage(5, 5, 10, 10, 0, 0, 2, -2, 0, 0, 0));
-- make an empty coverage with unequal coverage and scaled
select '4', st_metadata(st_makeemptycoverage(6, 6, 10, 10, 0, 0, 3, -3, 0, 0, 0));
-- make an empty coverage with equal coverage and rotated
select '5', st_metadata(st_makeemptycoverage(5, 5, 10, 10, 0, 0, 0.707106781186548, -0.707106781186548, -0.707106781186548, -0.707106781186548, 0));
