---
--- Tests for ST_Buffer with parameters (needs GEOS-3.2 or higher)
---
---

-- Ouput is snapped to grid to account for small floating numbers
-- differences between architectures
SELECT 'point quadsegs=2', ST_AsText(ST_SnapToGrid(st_buffer('POINT(0 0)', 1, 'quad_segs=2'), 1.0e-6));
SELECT 'line quadsegs=2', ST_AsText(ST_SnapToGrid(st_buffer('LINESTRING(0 0, 10 0)', 2, 'quad_segs=2'), 1.0e-6));
SELECT 'line quadsegs=2 endcap=flat', ST_AsText(ST_SnapToGrid(st_buffer('LINESTRING(0 0, 10 0)', 2, 'quad_segs=2 endcap=flat'), 1.0e-6));
SELECT 'line quadsegs=2 endcap=butt', ST_AsText(ST_SnapToGrid(st_buffer('LINESTRING(0 0, 10 0)', 2, 'quad_segs=2 endcap=butt'), 1.0e-6));
SELECT 'line quadsegs=2 endcap=square', ST_AsText(ST_SnapToGrid(st_buffer('LINESTRING(0 0, 10 0)', 2, 'quad_segs=2 endcap=square'), 1.0e-6));
SELECT 'line join=mitre mitre_limit=1.0 side=both', ST_AsText(ST_SnapToGrid(ST_Buffer('LINESTRING(50 50,150 150,150 50)',10,'join=mitre mitre_limit=1.0 side=both'), 1.0e-6));
SELECT 'line side=left',ST_AsText(ST_SnapToGrid(ST_Buffer('LINESTRING(50 50,150 150,150 50)',10,'side=left'),1.0e-6));
SELECT 'line side=right',ST_AsText(ST_SnapToGrid(ST_Buffer('LINESTRING(50 50,150 150,150 50)',10,'side=right'),1.0e-6));
SELECT 'line side=left join=mitre',ST_AsText(ST_SnapToGrid(ST_Buffer('LINESTRING(50 50,150 150,150 50)',10,'side=left join=mitre'),1.0e-6));
SELECT 'poly quadsegs=2 join=round', ST_AsText(ST_SnapToGrid(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=round'), 1.0e-6));
SELECT 'poly quadsegs=2 join=bevel', ST_AsText(ST_SnapToGrid(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=bevel'), 1.0e-6));
SELECT 'poly quadsegs=2 join=mitre', ST_AsText(ST_SnapToGrid(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=mitre'), 1.0e-6));
SELECT 'poly quadsegs=2 join=mitre mitre_limit=1', ST_AsText(ST_SnapToGrid(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=mitre mitre_limit=1'), 1.0e-6));
SELECT 'poly quadsegs=2 join=miter miter_limit=1', ST_AsText(ST_SnapToGrid(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=miter miter_limit=1'), 1.0e-6));
SELECT 'poly boundary rhr side=left', ST_AsText(ST_SnapToGrid(ST_Buffer(ST_ForceRHR(ST_Boundary('POLYGON ((20 20, 20 40, 40 40, 40 40, 40 20, 20 20))')),10,'join=mitre side=left'),1.0e-6));

