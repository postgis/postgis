---
--- Tests for ST_Buffer with parameters
---
---

-- Ouput is snapped to grid to account for small floating numbers
-- differences between architectures
SELECT 'point quadsegs=2', ST_AsText(st_buffer('POINT(0 0)', 1, 'quad_segs=2'), 4);
SELECT 'line quadsegs=2', ST_AsText(st_buffer('LINESTRING(0 0, 10 0)', 2, 'quad_segs=2'), 3);
SELECT 'line quadsegs=2 endcap=flat', ST_AsText(st_buffer('LINESTRING(0 0, 10 0)', 2, 'quad_segs=2 endcap=flat'), 5);
SELECT 'line quadsegs=2 endcap=butt', ST_AsText(st_buffer('LINESTRING(0 0, 10 0)', 2, 'quad_segs=2 endcap=butt'), 5);
SELECT 'line quadsegs=2 endcap=square', ST_AsText(st_buffer('LINESTRING(0 0, 10 0)', 2, 'quad_segs=2 endcap=square'), 5);
SELECT 'line join=mitre mitre_limit=1.0 side=both', ST_AsText(ST_Buffer('LINESTRING(50 50,150 150,150 50)',10,'join=mitre mitre_limit=1.0 side=both'), 5);
SELECT 'line side=left',ST_AsText(ST_Buffer('LINESTRING(50 50,150 150,150 50)',10,'side=left'), 5);
SELECT 'line side=right',ST_AsText(ST_Buffer('LINESTRING(50 50,150 150,150 50)',10,'side=right'),5);
SELECT 'line side=left join=mitre',ST_AsText(ST_Buffer('LINESTRING(50 50,150 150,150 50)',10,'side=left join=mitre'),5);
SELECT 'poly quadsegs=2 join=round', ST_AsText(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=round'), 5);
SELECT 'poly quadsegs=2 join=bevel', ST_AsText(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=bevel'), 5);
SELECT 'poly quadsegs=2 join=mitre', ST_AsText(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=mitre'), 5);
SELECT 'poly quadsegs=2 join=mitre mitre_limit=1', ST_AsText(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=mitre mitre_limit=1'), 5);
SELECT 'poly quadsegs=2 join=miter miter_limit=1', ST_AsText(st_buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 2, 'quad_segs=2 join=miter miter_limit=1'), 5);
SELECT 'poly boundary rhr side=left', ST_AsText(ST_Buffer(ST_ForceRHR(ST_Boundary('POLYGON ((20 20, 20 40, 40 40, 40 40, 40 20, 20 20))')),10,'join=mitre side=left'),5);

