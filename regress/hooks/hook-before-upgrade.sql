CREATE TABLE upgrade_test(g1 geometry, g2 geography);
INSERT INTO upgrade_test(g1,g2) VALUES
('POINT(0 0)', 'LINESTRING(0 0, 1 1)'),
('POINT(1 0)', 'LINESTRING(0 1, 1 1)');
