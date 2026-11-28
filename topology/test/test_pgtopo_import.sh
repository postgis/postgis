#!/bin/sh

#
# PostGIS - Spatial Types for PostgreSQL
# http://postgis.net
#
# Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
#
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
#

set -eu

# Run against a server where postgis and postgis_topology are available.
EXPORTER=$1
IMPORTER=$2
WORKDIR=$(mktemp -d "${PGIS_REG_TMPDIR:-${TMPDIR:-/tmp}}/pgtopo_import_test.XXXXXX")
SOURCE_DB=
TARGET_DB=

cleanup()
{
	test -z "${SOURCE_DB}" || dropdb "${SOURCE_DB}"
	test -z "${TARGET_DB}" || dropdb "${TARGET_DB}"
	rm -rf "${WORKDIR}"
}
trap cleanup 0

createdb "pgtopo_import_source_$$"
SOURCE_DB="pgtopo_import_source_$$"
createdb "pgtopo_import_target_$$"
TARGET_DB="pgtopo_import_target_$$"

psql -Xq -v ON_ERROR_STOP=1 "${SOURCE_DB}" <<'SQL'
CREATE EXTENSION postgis;
CREATE EXTENSION postgis_topology;
SELECT topology.CreateTopology('topo5284');
CREATE TABLE public.features (id integer PRIMARY KEY);
SELECT topology.AddTopoGeometryColumn('topo5284', 'public', 'features', 'old_topo', 'POINT');
INSERT INTO public.features VALUES
  (1, topology.toTopoGeom('POINT(1 2)'::geometry, 'topo5284', 1));
ALTER TABLE public.features RENAME COLUMN old_topo TO "new.topo";
SQL

psql -Xq -v ON_ERROR_STOP=1 "${TARGET_DB}" <<'SQL'
CREATE EXTENSION postgis;
CREATE EXTENSION postgis_topology;
SELECT topology.CreateTopology('occupied');
SQL

"${EXPORTER}" -f "${WORKDIR}/topology.tgz" "${SOURCE_DB}" topo5284
"${IMPORTER}" -f "${WORKDIR}/topology.tgz" topo5284 > "${WORKDIR}/restore.sql"
psql -Xq -v ON_ERROR_STOP=1 "${TARGET_DB}" -f "${WORKDIR}/restore.sql"

result=$(psql -XAt -v ON_ERROR_STOP=1 "${TARGET_DB}" <<'SQL'
SELECT feature_column, topology_id FROM topology.layer;
SELECT ("new.topo").topology_id, ST_AsText("new.topo"::geometry) FROM public.features;
SELECT conname FROM pg_constraint
WHERE conrelid = 'public.features'::regclass AND contype = 'c';
SQL
)
test "${result}" = 'new.topo|2
2|MULTIPOINT((1 2))
check_topogeom_new.topo'
