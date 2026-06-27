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

EXPORTER=$1
TMPDIR=${TMPDIR:-/tmp}
WORKDIR=$(mktemp -d "${TMPDIR}/pgtopo_export_test.XXXXXX")

cleanup()
{
	rm -rf "${WORKDIR}"
}
trap cleanup 0

BINDIR="${WORKDIR}/bin"
mkdir -p "${BINDIR}"

cat > "${BINDIR}/psql" <<'SH'
#!/bin/sh
query=$(cat)
printf '%s\n' "$query" >> "${PGTOPO_EXPORT_TEST_QUERIES}"

if printf '%s\n' "$query" | grep -q 'FROM[[:space:]]*topology.topology'; then
	printf '0\t0\t0\n'
elif printf '%s\n' "$query" | grep -q 'actual_feature_column'; then
	printf '1\tfeatures\tparcels\trenamed_topo\t3\t0\t\\N\n'
fi
SH
chmod +x "${BINDIR}/psql"

cat > "${BINDIR}/pg_dump" <<'SH'
#!/bin/sh
outfile=
while test "$#" -gt 0; do
	if test "$1" = "-f"; then
		shift
		outfile=$1
	fi
	shift
done
test -n "$outfile" || exit 1
printf 'fake layer dump\n' > "$outfile"
SH
chmod +x "${BINDIR}/pg_dump"

export PATH="${BINDIR}:$PATH"
export PGTOPO_EXPORT_TEST_QUERIES="${WORKDIR}/queries.sql"

"${EXPORTER}" -f "${WORKDIR}/topology.tgz" fake_db topo5284 > "${WORKDIR}/stdout"

tar xzf "${WORKDIR}/topology.tgz" -C "${WORKDIR}"

grep -q "COALESCE(actual_feature_column.feature_column, l.feature_column)" "${PGTOPO_EXPORT_TEST_QUERIES}"
grep -q "pg_catalog.pg_get_constraintdef(con.oid)" "${PGTOPO_EXPORT_TEST_QUERIES}"
grep -Fq "([^0-9]|$)" "${PGTOPO_EXPORT_TEST_QUERIES}"
if grep -Fq "= ' || l.layer_id || '%'" "${PGTOPO_EXPORT_TEST_QUERIES}"; then
	echo "pgtopo_export still uses prefix matching for layer ids" >&2
	exit 1
fi
grep -q "renamed_topo" "${WORKDIR}/pgtopo_export/layers"

echo "ok - pgtopo_export records renamed TopoGeometry columns from constraints"
