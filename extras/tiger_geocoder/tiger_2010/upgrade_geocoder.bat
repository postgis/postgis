REM $Id: create_geocode.bat 6774 2011-02-01 13:55:02Z robe $
set PGPORT=5432
set PGHOST=localhost
set PGUSER=postgres
set PGPASSWORD=yourpasswordhere
set THEDB=geocoder
set PGBIN=C:\Program Files\PostgreSQL\8.4\bin
set PGCONTRIB=C:\Program Files\PostgreSQL\8.4\share\contrib
"%PGBIN%\psql"  -d "%THEDB%" -f "tiger_loader.sql"
"%PGBIN%\psql"  -d "%THEDB%" -f "upgrade_geocode.sql"
pause

