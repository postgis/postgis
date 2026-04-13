SELECT 'CG_Orientation_1', CG_Orientation(CG_ForceLHR('POLYGON((0 0,0 1,1 1,1 0,0 0))'));
SELECT 'CG_Orientation_2', CG_Orientation(ST_ForceRHR('POLYGON((0 0,0 1,1 1,1 0,0 0))'));
SELECT 'CG_Orientation_1', CG_Orientation(CG_ForceLHR('POLYGON((0 0,0 1,1 1,1 0,0 0))'));
SELECT 'CG_Orientation_2', CG_Orientation(ST_ForceRHR('POLYGON((0 0,0 1,1 1,1 0,0 0))'));
