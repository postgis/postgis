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
all: sql/$(EXTENSION)--$(EXTVERSION).sql sql/$(EXTENSION)--2.0.0a3--$(EXTVERSION).sql sql/$(EXTENSION)--2.0.0a4--$(EXTVERSION).sql sql/$(EXTENSION)--2.0.0a5--$(EXTVERSION).sql sql/$(EXTENSION)--2.0.0a6--$(EXTVERSION).sql

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
	
#can't use the one built because can't drop the postgis_version function
#leave out all casts, operators, types, tables, schema, and aggregates
sql_bits/postgis_upgrade_minor.sql: ../../postgis/postgis.sql 
		 sed -e '/^\(CREATE\|ALTER\) \(CAST\|OPERATOR\|TYPE\|TABLE\|SCHEMA\|DOMAIN\|TRIGGER\|AGGREGATE\).*;/d' \
	 	 	-e '/^\(CREATE\|ALTER\) \(CAST\|OPERATOR\|TYPE\|TABLE\|SCHEMA\|DOMAIN\|TRIGGER\|AGGREGATE\)/,/\;/d' \
	 	 	-e 's/BEGIN;//g' -e 's/COMMIT;//g' \
	 	 $< > $@

	
sql_bits/raster_comments.sql: ../../doc/raster_comments.sql
	cp $< $@

sql_bits/postgis_raster_upgrade_minor.sql: ../postgis_extension_helper.sql sql_bits/remove_from_extension.sql.in ../../postgis/postgis_drop_before.sql.in.c  sql_bits/postgis_upgrade_minor.sql 	../../raster/rt_pg/rtpostgis_upgrade_20_minor.sql  ../../doc/raster_comments.sql  ../../doc/postgis_comments.sql ../postgis_extension_helper_uninstall.sql
	cat $^ > $@

sql/$(EXTENSION)--2.0.0a3--$(EXTVERSION).sql: sql_bits/postgis_raster_upgrade_minor.sql
	cp $< $@
	
sql/$(EXTENSION)--2.0.0a4--$(EXTVERSION).sql: sql_bits/postgis_raster_upgrade_minor.sql
	cp $< $@
	
sql/$(EXTENSION)--2.0.0a5--$(EXTVERSION).sql: sql_bits/postgis_raster_upgrade_minor.sql
	cp $< $@
	
sql/$(EXTENSION)--2.0.0a6--$(EXTVERSION).sql: sql_bits/postgis_raster_upgrade_minor.sql
	cp $< $@
	
# sql_bits/rtpostgis--unpackaged.sql:  ../../raster/rt_pg/rtpostgis.sql	 
# 	sed -e 's/^[\t]*//' \
# 		-e :a -e '$!N; s/,\n/,/; ta' \
# 	$< > $@
#remove leading white space and tabs
#remove line break from a line if it ends in ,

sql_bits/rtpostgis-filtered.sql:  ../../raster/rt_pg/rtpostgis.sql
		sed \
		-e 's/^[\t]*//' \
		$< > $@
		
sql_bits/rtpostgis--unpackaged.sql:  ../../raster/rt_pg/rtpostgis.sql
		sed \
		-e 's/^[\t]*//' \
		-e '$!N; s/(\n/(/g' \
		-e '/^CREATE \(OR REPLACE FUNCTION\|TRIGGER\|TYPE\|TABLE\|VIEW\)/!d;' \
		-e 's/OR REPLACE//g' \
		-e 's/CREATE\(.*\)/ALTER EXTENSION $(EXTENSION) ADD\1;/' \
		-e 's/DEFAULT [^()]\+//g' \
		-e 's/\(BEFORE\|AS\|RETURNS\)\(.*\)/;/' \
		-e 's/(;/;/' \
		-e 's/\\(;/;/' \
		-e 's/;;/;/g' $< > $@
		
sql_bits/postgis--unpackaged.sql:   ../../postgis/postgis.sql
	sed -e '/^CREATE \(OR REPLACE\|TRIGGER\|TYPE\|TABLE\|VIEW\)/!d;' \
		-e 's/OR REPLACE//g' \
		-e 's/CREATE\(.*\)/ALTER EXTENSION $(EXTENSION) ADD\1;/' \
		-e 's/DEFAULT [\.0-9a-zA-Z]\+//g' \
		-e 's/\(BEFORE\|AS\|RETURNS\)\(.*\)/;/' \
		-e 's/(;/;/' \
		-e 's/\\(;/;/' \
		-e 's/;;/;/g' $< > $@

DATA = $(wildcard sql/*--*.sql) sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN += sql/$(EXTENSION)--$(EXTVERSION).sql
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
