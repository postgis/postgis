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

IMPORTER=$1
TMPDIR=${TMPDIR:-/tmp}
WORKDIR=$(mktemp -d "${TMPDIR}/pgtopo_import_test.XXXXXX")

cleanup()
{
	rm -rf "${WORKDIR}"
}
trap cleanup 0

DUMPDIR="${WORKDIR}/pgtopo_export"
mkdir -p "${DUMPDIR}"

printf '1\n' > "${DUMPDIR}/pgtopo_dump_version"
printf '0\t0\t0\n' > "${DUMPDIR}/topology"
printf '\\N\n' > "${DUMPDIR}/face"
printf '\\N\n' > "${DUMPDIR}/node"
printf '\\N\n' > "${DUMPDIR}/edge_data"
printf '\\N\n' > "${DUMPDIR}/relation"
printf '1\tfeatures\tparcels\trenamed_topo\t3\t0\t\\N\n' > "${DUMPDIR}/layers"

(
	cd "${WORKDIR}"
	tar czf topology.tgz pgtopo_export
)

"${IMPORTER}" -f "${WORKDIR}/topology.tgz" restored_topology > "${WORKDIR}/restore.sql"

grep -q "FROM pg_catalog.pg_constraint con" "${WORKDIR}/restore.sql"
grep -q "con.conname LIKE 'check_topogeom_%'" "${WORKDIR}/restore.sql"
grep -q "att.attname = 'renamed_topo'" "${WORKDIR}/restore.sql"
grep -q "ALTER TABLE %I.%I DROP CONSTRAINT %I" "${WORKDIR}/restore.sql"

if grep -q 'DROP CONSTRAINT IF EXISTS "check_topogeom_renamed_topo"' "${WORKDIR}/restore.sql"; then
	echo "pgtopo_import still drops only the canonical constraint name" >&2
	exit 1
fi

echo "ok - pgtopo_import drops renamed TopoGeometry constraints by catalog lookup"
