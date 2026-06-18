#!/bin/sh

set -e

TMPDIR="${TMPDIR:-/tmp}/check_create_upgrade_replaced_function_errors.$$"
trap 'rm -rf "${TMPDIR}"' EXIT

mkdir -p "${TMPDIR}"

cat > "${TMPDIR}/postgis.sql" <<'SQL'
-- INSTALL VERSION: 3.7.0dev
-- Availability: 1.0.0
-- Changed: 3.1.0 add a default argument
-- Replaces ST_Test(geometry) deprecated in 3.1.0
CREATE OR REPLACE FUNCTION ST_Test(geometry, float8 DEFAULT 0.0) RETURNS geometry
AS 'MODULE_PATHNAME', 'ST_Test'
LANGUAGE 'c' IMMUTABLE STRICT PARALLEL SAFE;
SQL

"${PERL:-perl}" "${srcdir:-.}/create_upgrade.pl" "${TMPDIR}/postgis.sql" > "${TMPDIR}/upgrade.sql"

# The duplicate-function branch turns a PostgreSQL rename collision from a
# partially completed upgrade into an actionable PostGIS diagnostic.
grep -Fq \
	"PostGIS upgrade cannot rename replaced function st_test(geometry): leftover deprecated function st_test_deprecated_by_postgis_301(geometry) already exists" \
	"${TMPDIR}/upgrade.sql"

grep -Fq \
	"retry SELECT postgis_extensions_upgrade()" \
	"${TMPDIR}/upgrade.sql"
