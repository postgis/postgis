\set VERBOSITY terse

set client_min_messages to ERROR;

\i :top_builddir/topology/test/load_topology.sql

SELECT useslargeids FROM topology.topology;

SELECT
    sequencename,
    data_type,
    last_value,
    max_value
FROM pg_sequences
WHERE
    schemaname = 'city_data'
    AND sequencename IN (
        'face_face_id_seq',
        'edge_data_edge_id_seq',
        'node_node_id_seq'
    )
ORDER BY sequencename;

SELECT attrelid::regclass::text, attname column_name, atttypid::regtype data_type
FROM pg_attribute
WHERE attrelid IN ('city_data.face'::regclass,'city_data.edge_data'::regclass,'city_data.node'::regclass,'city_data.relation'::regclass)
AND attnum > 0
AND not attisdropped
ORDER BY attrelid::regclass::text, attnum;

-- Perform upgrade
SELECT topology.upgradetopology ('city_data');

SELECT useslargeids FROM topology.topology;

SELECT
    sequencename,
    data_type,
    last_value,
    max_value
FROM pg_sequences
WHERE
    schemaname = 'city_data'
    AND sequencename IN (
        'face_face_id_seq',
        'edge_data_edge_id_seq',
        'node_node_id_seq'
    )
ORDER BY sequencename;

SELECT attrelid::regclass::text, attname column_name, atttypid::regtype data_type
FROM pg_attribute
WHERE attrelid IN ('city_data.face'::regclass,'city_data.edge_data'::regclass,'city_data.node'::regclass,'city_data.relation'::regclass)
AND attnum > 0
AND not attisdropped
ORDER BY attrelid::regclass::text, attnum;

SELECT topology.droptopology ('city_data');
