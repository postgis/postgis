\set VERBOSITY terse

set client_min_messages to ERROR;

\i :top_builddir/topology/test/load_large_topology.sql

SELECT 'unexpected_large_city_data_invalidities', *
FROM ValidateTopology ('large_city_data');

-- Test correctness of edge linking
-- See https://trac.osgeo.org/postgis/ticket/3042
--set client_min_messages to NOTICE;
BEGIN;
-- Break edge linking for all edges around node 14
UPDATE large_city_data.edge_data
SET
    next_left_edge = - next_left_edge
where
    edge_id in (
        1000000000009,
        1000000000010,
        1000000000020
    );

UPDATE large_city_data.edge_data
SET
    next_right_edge = - next_right_edge
where
    edge_id = 1000000000019;
-- Break edge linking of dangling edges, including one around last node (3)
UPDATE large_city_data.edge_data
SET
    next_left_edge = - next_left_edge,
    next_right_edge = - next_right_edge
where
    edge_id in (1000000000003, 1000000000025);

set client_min_messages to WARNING;

SELECT '#3042', *
FROM ValidateTopology ('large_city_data')
UNION
SELECT '#3042', '---', null, null
ORDER BY 1, 2, 3, 4;

ROLLBACK;

-- Test correctness of side-labeling
-- See https://trac.osgeo.org/postgis/ticket/4944
BEGIN;
-- Swap side-label of face-binding edge
UPDATE large_city_data.edge_data
SET
    left_face = right_face,
    right_face = left_face
WHERE
    edge_id = 1000000000019;
--set client_min_messages to DEBUG;
SELECT '#4944', *
FROM ValidateTopology ('large_city_data')
UNION
SELECT '#4944', '---', null, null
ORDER BY 1, 2, 3, 4;

ROLLBACK;

-- Test face with multiple shells is caught
-- See https://trac.osgeo.org/postgis/ticket/4945
BEGIN;

UPDATE large_city_data.edge_data
SET
    right_face = 1000000000003
WHERE
    edge_id IN (1000000000008, 1000000000017);

UPDATE large_city_data.edge_data
SET
    left_face = 1000000000003
WHERE
    edge_id IN (1000000000011, 1000000000015);
-- To reduce the noise,
-- set face 3 mbr to include the mbr of face 5
-- and delete face 5
UPDATE large_city_data.face
SET
    mbr = (
        SELECT ST_Envelope(ST_Collect(mbr))
        FROM large_city_data.face
        WHERE
            face_id IN (1000000000003, 1000000000005)
    )
WHERE
    face_id = 1000000000003;

DELETE FROM large_city_data.face WHERE face_id = 1000000000005;

SELECT '#4945', *
FROM ValidateTopology ('large_city_data')
UNION
SELECT '#4945', '---', null, null
ORDER BY 1, 2, 3, 4;

ROLLBACK;

-- Test outer face labeling
-- See https://trac.osgeo.org/postgis/ticket/4684

-- 1. Set wrong outer face for isolated inside edges
BEGIN;

UPDATE large_city_data.edge_data
SET
    left_face = 1000000000002,
    right_face = 1000000000002
WHERE
    edge_id = 1000000000025;

SELECT '#4830.1', (
        ValidateTopology ('large_city_data')
    ).*
UNION
SELECT '#4830.1', '---', null, null
ORDER BY 1, 2, 3, 4;

ROLLBACK;
-- 2. Set wrong outer face for isolated outside edge
BEGIN;

UPDATE large_city_data.edge_data
SET
    left_face = 1000000000002,
    right_face = 1000000000002
WHERE
    edge_id IN (1000000000004, 1000000000005);

SELECT '#4830.2', (
        ValidateTopology ('large_city_data')
    ).*
UNION
SELECT '#4830.2', '---', null, null
ORDER BY 1, 2, 3, 4;

ROLLBACK;
-- 3. Set wrong outer face for face hole
BEGIN;

UPDATE large_city_data.edge_data
SET
    right_face = 1000000000009
WHERE
    edge_id = 1000000000026;

SELECT '#4830.3', (
        ValidateTopology ('large_city_data')
    ).*
UNION
SELECT '#4830.3', '---', null, null
ORDER BY 1, 2, 3, 4;

ROLLBACK;
-- 4. Set universal outer face for isolated edge inside a face
BEGIN;

UPDATE large_city_data.edge_data
SET
    left_face = 0,
    right_face = 0
WHERE
    edge_id = 1000000000025;

SELECT '#4830.4', (
        ValidateTopology ('large_city_data')
    ).*
UNION
SELECT '#4830.4', '---', null, null
ORDER BY 1, 2, 3, 4;

ROLLBACK;
-- 5. Set universal outer face for face hole
BEGIN;

UPDATE large_city_data.edge_data
SET
    right_face = 0
WHERE
    edge_id = 1000000000026;

SELECT '#4830.5', (
        ValidateTopology ('large_city_data')
    ).*
UNION
SELECT '#4830.5', '---', null, null
ORDER BY 1, 2, 3, 4;

ROLLBACK;

-- Test ability to call twice in a transaction
-- in presence of mixed face labeling
-- See https://trac.osgeo.org/postgis/ticket/5017
BEGIN;

SELECT '#5017.0', ( ValidateTopology ('large_city_data') );

SELECT '#5017.1', ( ValidateTopology ('large_city_data') );

update large_city_data.edge_data
SET
    left_face = 1000000000008
WHERE
    edge_id = 1000000000010;

SELECT '#5017.2', ( ValidateTopology ('large_city_data') ).error;

SELECT '#5017.3', ( ValidateTopology ('large_city_data') ).error;

ROLLBACK;

-- Test dangling edgerings are never considered shells
-- See https://trac.osgeo.org/postgis/ticket/5105
BEGIN;

SELECT NULL FROM CreateTopology ('t5105');

SELECT '#5105.0', TopoGeo_addLineString (
        't5105', '
LINESTRING(
  29.262792863298348 71.22115103790775,
  29.26598031986849  71.22202978558047,
  29.275379947735576 71.22044935739267,
  29.29461024331857  71.22741507590429,
  29.275379947735576 71.22044935739267,
  29.26598031986849  71.22202978558047,
  29.262792863298348 71.22115103790775
)'
    );

SELECT '#5105.1', TopoGeo_addLineString (
        't5105', ST_Translate (geom, 0, -2)
    )
FROM t5105.edge
WHERE
    edge_id = 1000000000001;

SELECT '#5105.edges_count', count(*) FROM t5105.edge;

SELECT '#5105.faces_count', count(*)
FROM t5105.face
WHERE
    face_id > 0;

SELECT '#5105.unexpected_invalidities', *
FROM ValidateTopology ('t5105');
-- TODO: add some areas to the endpoints of the dangling edges above
--       to form O-O figures
ROLLBACK;

-- See https://trac.osgeo.org/postgis/ticket/5403
BEGIN;

SET search_path TO public;

SELECT NULL FROM topology.CreateTopology ('t5403');

SELECT '#5403.0', topology.TopoGeo_addPolygon (
        't5403', 'POLYGON((0 0, 5 10,10 0,0 0),(2 2,8 2,5 8,2 2))'
    );

SELECT '#5403.1', *
FROM topology.ValidateTopology (
        't5403', ST_MakeEnvelope(2, 2, 4, 4)
    );

ROLLBACK;

-- See https://trac.osgeo.org/postgis/ticket/5548
BEGIN;

SELECT NULL FROM topology.CreateTopology ('t5548');

SELECT '#5548.0', topology.TopoGeo_addPolygon (
        't5548', 'POLYGON((0 0, 5 10, 10 0,0 0))'
    );

DELETE FROM t5548.edge;

DELETE FROM t5548.node;

SELECT '#5548.1', *
FROM topology.ValidateTopology (
        't5548', ST_MakeEnvelope(2, 2, 4, 4)
    );

ROLLBACK;

-- See https://trac.osgeo.org/postgis/ticket/5766
BEGIN;

SELECT NULL FROM topology.CreateTopology ('t5766');

SELECT NULL
FROM topology.TopoGeo_addPolygon (
        't5766', 'POLYGON((0 0, 1 2, 2 0,0 0))'
    );

UPDATE t5766.face
SET
    mbr = ST_MakeEnvelope(0, 0, 2, 2)
WHERE
    face_id = 0;
-- bbox matching leftmost ring
SELECT '#5766.1', 'no bbox', (array_agg(v))[1] FROM topology.ValidateTopology('t5766') v;

SELECT '#5766.1', 'overlapping bbox', (array_agg(v))[1] FROM topology.ValidateTopology('t5766', 'POINT(1 1)') v;

SELECT '#5766.1', 'disjoint bbox', (array_agg(v))[1] FROM topology.ValidateTopology('t5766', 'POINT(3 0)') v;

UPDATE t5766.face
SET
    mbr = ST_MakeEnvelope(0, 0, 1, 2)
WHERE
    face_id = 0;
-- bbox not matching leftmost ring
SELECT '#5766.2', 'no bbox', (array_agg(v))[1] FROM topology.ValidateTopology('t5766') v;

SELECT '#5766.2', 'overlapping bbox', (array_agg(v))[1] FROM topology.ValidateTopology('t5766', 'POINT(1 1)') v;

SELECT '#5766.2', 'disjoint bbox', (array_agg(v))[1] FROM topology.ValidateTopology('t5766', 'POINT(3 0)') v;

ROLLBACK;

SELECT NULL FROM topology.DropTopology ('large_city_data');
