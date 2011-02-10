# $Id$
export PGPORT=5432
export PGHOST=localhost
export PGUSER=postgres
export PGPASSWORD=yourpasswordhere
export THEDB=geocoder
PSQL_CMD="psql"
PGCONTRIB=/usr/share/contrib
${PSQL_CMD} -d "%THEDB%" -f "${PGCONTRIB}/fuzzystrmatch.sql"
${PSQL_CMD} -d "%THEDB%" -c "CREATE SCHEMA tiger"
${PSQL_CMD} -d "%THEDB%" -f "tables/lookup_tables_2010.sql"
${PSQL_CMD} -d "%THEDB%" -c "CREATE SCHEMA tiger_data"
${PSQL_CMD} -d "%THEDB%" -f "tiger_loader.sql"
${PSQL_CMD} -d "%THEDB%" -f "create_geocode.sql"