#!/usr/bin/env bash
PGPORT=5432
PGHOST=localhost
PGUSER=postgres
PGPASSWORD=yourpasswordhere
THEDB=geocoder
PSQL_CMD= /usr/lib/postgresql/12/bin/psql
PGCONTRIB=/usr/share/postgresql/12/contrib
#if you are on 9.1+ use the CREATE EXTENSION syntax instead
${PSQL_CMD} -d "${THEDB}" -f "${PGCONTRIB}/fuzzystrmatch.sql"
#${PSQL_CMD} -d "${THEDB}" -c "CREATE EXTENSION fuzzystrmatch" 
${PSQL_CMD} -d "${THEDB}" -c "CREATE SCHEMA tiger"
#unremark this next line and edit if you want the search paths set as part of the install
#${PSQL_CMD} -d "${THEDB}" -c "ALTER DATABASE ${THEDB} SET search_path=public, tiger;"
#${PSQL_CMD} -d "${THEDB}" -f "tables/lookup_tables_2011.sql"
${PSQL_CMD} -d "${THEDB}" -c "CREATE SCHEMA tiger_data"
${PSQL_CMD} -d "${THEDB}" -f "create_geocode.sql"
${PSQL_CMD} -d "${THEDB}" -f "tiger_loader_2019.sql"
