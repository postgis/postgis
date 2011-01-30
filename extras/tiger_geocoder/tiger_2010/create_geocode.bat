set PGPORT=5432
set PGHOST=localhost
set PGUSER=postgres
set PGPASSWORD=yourpasswordhere
set THEDB=geocoder
set PGBIN=C:\Program Files\PostgreSQL\8.4\bin
set PGCONTRIB=C:\Program Files\PostgreSQL\8.4\share\contrib
"%PGBIN%\psql" -d "%THEDB%" -f "%PGCONTRIB%\fuzzystrmatch.sql"
"%PGBIN%\psql"  -d "%THEDB%" -c "CREATE SCHEMA tiger"
"%PGBIN%\psql"  -d "%THEDB%" -f "tables\lookup_tables_2010.sql"
"%PGBIN%\psql"  -d "%THEDB%" -c "CREATE SCHEMA tiger_data"
"%PGBIN%\psql"  -d "%THEDB%" -f "tiger_loader.sql"
"%PGBIN%\psql"  -d "%THEDB%" -f "create_geocode.sql"
pause

