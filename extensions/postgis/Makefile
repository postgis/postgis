EXTENSION    = postgis
EXTVERSION   = $(shell grep default_version $(EXTENSION).control | \
               sed -e "s/default_version[[:space:]]*=[[:space:]]*'\([^']*\)'/\1/")

DATA         = $(filter-out $(wildcard sql/*--*.sql),$(wildcard sql/*.sql))

#DOCS         = $(wildcard ../../doc/html/*.html)
PG_CONFIG    = pg_config

PG91         = $(shell $(PG_CONFIG) --version | grep -qE " 8\.| 9\.0" && echo no || echo yes)
SQL_BITS     = $(wildcard sql_bits/*.sql)
EXTRA_CLEAN += sql/$(EXTENSION).sql ${SQL_BITS}


ifeq ($(PG91),yes)
sql/$(EXTENSION).sql: sql_bits/postgis.sql sql_bits/postgis_comments.sql sql_bits/rtpostgis.sql sql_bits/raster_comments.sql sql_bits/spatial_ref_sys.sql
	cat $^ > $@
all: sql/$(EXTENSION)--$(EXTVERSION).sql sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql

sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	cp $< $@
	
sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql: sql_bits/rtpostgis--unpackaged.sql sql_bits/postgis--unpackaged.sql
	cat $^ > $@
	
sql_bits/spatial_ref_sys.sql: ../../spatial_ref_sys.sql	
	sed -e 's/BEGIN;//g' -e 's/COMMIT;//g'  $< > $@
	#cp $< $@
#strip BEGIN/COMMIT since these are not allowed in extensions
sql_bits/postgis.sql: ../../postgis/postgis.sql
	sed -e 's/BEGIN;//g' -e 's/COMMIT;//g'  $< > $@
	
sql_bits/postgis_comments.sql: ../../doc/postgis_comments.sql
	cp $< $@
	
#strip BEGIN/COMMIT since these are not allowed in extensions
sql_bits/rtpostgis.sql: ../../raster/rt_pg/rtpostgis.sql
	sed -e 's/BEGIN;//g' -e 's/COMMIT;//g'  $< > $@
	
sql_bits/raster_comments.sql: ../../doc/raster_comments.sql
	cp $< $@
	
# sql_bits/rtpostgis--unpackaged.sql:  ../../raster/rt_pg/rtpostgis.sql	 
# 	sed -e 's/^[\t]*//' \
# 		-e :a -e '$!N; s/,\n/,/; ta' \
# 	$< > $@
#remove leading white space and tabs
#remove line break from a line if it ends in ,
sql_bits/rtpostgis--unpackaged.sql:  ../../raster/rt_pg/rtpostgis.sql
		sed \
		-e 's/^[\t]*//' \
		-e '$!N; s/,\n/,/g' \
		-e '/^CREATE \(OR REPLACE.*\|TRIGGER\|TYPE\|TABLE\|VIEW\)/!d;' \
		-e 's/OR REPLACE//g' \
		-e 's/CREATE\(.*\)/ALTER EXTENSION $(EXTENSION) ADD\1;/' \
		-e 's/DEFAULT [\.0-9a-zA-Z]\+//g' \
		-e 's/\(BEFORE\|AS\|RETURNS\)\(.*\)/;/' \
		-e 's/(;/;/' \
		-e 's/\\(;/;/' \
		-e 's/;;/;/g' $< > $@
		
sql_bits/postgis--unpackaged.sql:   ../../postgis/postgis.sql
	sed -e '/^CREATE \(OR REPLACE\|TRIGGER\|TYPE\|TABLE\|VIEW\)/!d;' \
		-e 's/OR REPLACE//g' \
		-e 's/CREATE\(.*\)/ALTER EXTENSION $(EXTENSION) ADD\1;/' \
		-e 's/DEFAULT [\.0-9a-zA-Z]\+//g' \
		-e 's/\(BEFORE\|AS\)\(.*\)/;/' \
		-e 's/(;/;/' \
		-e 's/\\(;/;/' \
		-e 's/;;/;/g' $< > $@

DATA = $(wildcard sql/*--*.sql) sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN += sql/$(EXTENSION)--$(EXTVERSION).sql
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
