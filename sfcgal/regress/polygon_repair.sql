-- Test CG_PolygonRepair with default rule (EVEN_ODD)
SELECT 'Repair bowtie polygon default', ST_AsText(CG_PolygonRepair('POLYGON((0 0, 10 10, 10 0, 0 10, 0 0))'), 2);

-- Test CG_PolygonRepair with EVEN_ODD rule explicitly
SELECT 'Repair bowtie polygon EVEN_ODD', ST_AsText(CG_PolygonRepair('POLYGON((0 0, 10 10, 10 0, 0 10, 0 0))', 0), 2);

-- Test CG_PolygonRepair with overlapping rings
SELECT 'Repair overlapping polygon', ST_AsText(CG_PolygonRepair('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0), (2 2, 8 2, 8 8, 2 8, 2 2))'), 2);

-- Test CG_PolygonRepair with a simple valid polygon
SELECT 'Repair valid polygon', ST_AsText(CG_PolygonRepair('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'), 2);

-- Test CG_PolygonRepair with MultiPolygon
SELECT 'Repair multipolygon', ST_AsText(CG_PolygonRepair('MULTIPOLYGON(((0 0, 5 5, 5 0, 0 5, 0 0)), ((10 10, 15 10, 15 15, 10 15, 10 10)))'), 2);

-- Test CG_PolygonRepair with empty polygon
SELECT 'Repair empty polygon', ST_AsText(CG_PolygonRepair(ST_GeomFromText('POLYGON EMPTY', 4326)));
