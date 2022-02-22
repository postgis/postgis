-- See https://trac.osgeo.org/postgis/ticket/5102
SELECT topology.CopyTopology('upgrade_test', 'upgrade_test_copy');

SELECT topology.DropTopology('upgrade_test');
SELECT topology.DropTopology('upgrade_test_copy');

