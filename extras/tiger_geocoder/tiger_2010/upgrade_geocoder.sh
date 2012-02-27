#!/bin/bash
# $Id$
export PGPORT=5432
export PGHOST=localhost
export PGUSER=postgres
export PGPASSWORD=yourpasswordhere
THEDB=geocoder
PSQL_CMD=/usr/bin/psql
PGCONTRIB=/usr/share/postgresql/contrib
${PSQL_CMD} -d "${THEDB}" -f "tiger_loader.sql"
${PSQL_CMD} -d "${THEDB}" -f "upgrade_geocode.sql"