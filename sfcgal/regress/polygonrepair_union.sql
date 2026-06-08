-- UNION rule: keep all winding area (requires CGAL 6.1+)
SELECT 'bowtie_union', ST_AsText(CG_PolygonRepair('POLYGON((0 0,2 2,2 0,0 2,0 0))', 'UNION'));
