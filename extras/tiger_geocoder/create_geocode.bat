set PGPORT=5432
set PGHOST=localhost
set PGUSER=postgres
set PGPASSWORD=yourpasswordhere
set THEDB=geocoder
set PGBIN=C:\Program Files\PostgreSQL\12\bin
set PGCONTRIB=C:\Program Files\PostgreSQL\12\share\contrib
REM "%PGBIN%\psql" -d "%THEDB%" -f "%PGCONTRIB%\fuzzystrmatch.sql"
REM If you are using PostgreSQL 9.1 or above, use the extension syntax instead as shown below
"%PGBIN%\psql"  -d "%THEDB%" -c "CREATE EXTENSION fuzzystrmatch;" 
"%PGBIN%\psql"  -d "%THEDB%" -c "CREATE SCHEMA tiger;"
REM unremark this next line and edit if you want the search paths set as part of the install
REM "%PGBIN%\psql" -d "%THEDB%" -c "ALTER DATABASE %THEDB% SET search_path=public, tiger;"
"%PGBIN%\psql"  -d "%THEDB%" -f "create_geocode.sql"
REM "%PGBIN%\psql"  -d "%THEDB%" -f "tables\lookup_tables_2011.sql"
"%PGBIN%\psql"  -d "%THEDB%" -c "CREATE SCHEMA tiger_data;"
"%PGBIN%\psql"  -d "%THEDB%" -f "tiger_loader_2019.sql;"
pause

