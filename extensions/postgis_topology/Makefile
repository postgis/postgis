EXTENSION    = postgis_topology
EXTVERSION   = $(shell grep default_version $(EXTENSION).control | sed -e "s/default_version[[:space:]]*=[[:space:]]*'\([^']*\)'/\1/")

DATA         = $(filter-out $(wildcard sql/*--*.sql),$(wildcard sql/*.sql))

PG_CONFIG    = pg_config

PG91         = $(shell $(PG_CONFIG) --version | grep -qE " 8\.| 9\.0" && echo no || echo yes)
SQL_BITS     = $(wildcard sql_bits/*.sql)
EXTRA_CLEAN += sql/*.sql ${SQL_BITS}


ifeq ($(PG91),yes)
all: sql/$(EXTENSION)--$(EXTVERSION).sql sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql sql/$(EXTENSION)--2.0.0a1--$(EXTVERSION).sql

sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	cp $< $@
	
sql/$(EXTENSION).sql: sql_bits/topology.sql sql_bits/topology_comments.sql
	cat $^ > $@
	
#strip BEGIN/COMMIT since these are not allowed in extensions
#strip CREATE SCHEMA since we force extension 
# to create schema by setting schema to topology in control
sql_bits/topology.sql: ../../topology/topology.sql
	sed -e 's/BEGIN;//g' -e 's/COMMIT;//g' -e '/^CREATE SCHEMA/d;'  $< > $@
	
sql_bits/topology_comments.sql: ../../doc/topology_comments.sql
	cp $< $@
	
#grep all lines that start with CREATE OR REPLACE FUNCTION, TRIGGER...
#then replace CREATE OR REPLACE .. with ALTER EXTENSION..;
#then remove default values and extra junk
sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql: ../../topology/topology.sql
	sed -e '/^CREATE \(OR REPLACE\|TRIGGER\|TYPE\|TABLE\|VIEW\)/!d;' \
		-e 's/OR REPLACE//g' \
		-e 's/CREATE\(.*\)/ALTER EXTENSION $(EXTENSION) ADD\1;/' \
		-e 's/DEFAULT [\.0-9a-zA-Z]\+//g' \
		-e 's/\(BEFORE\|AS\)\(.*\)/;/' \
		-e 's/(;/;/' \
		-e 's/\\(;/;/' \
		-e 's/;;/;/g' $< > $@

#upgrade script should have everything but table, schema, type creation/alter
#NOTE: we assume all object definitions end in ;
#first expression deletes all non-removable objects defined on same line
#second deletes all non-removable defined on multiple lines 
#  the end of the body of object we assume ends in ;
#aggregates are special 
#they can be dropped but we need to remove
#them from the extension first
sql/topology_upgrade_minor.sql:  sql_bits/topology.sql
	 sed -e '/^\(CREATE\|ALTER\) \(CAST\|TYPE\|TABLE\|SCHEMA\|DOMAIN\|TRIGGER\).*;/d' \
	 	 -e '/^\(CREATE\|ALTER\) \(CAST\|TYPE\|TABLE\|SCHEMA\|DOMAIN\|TRIGGER\)/,/\;/d' \
	 	 -e 's/^DROP AGGREGATE IF EXISTS\(.*\);/ALTER EXTENSION $(EXTENSION) DROP AGGREGATE \1;DROP AGGREGATE \1;/' \
	 	 $< > $@

sql/$(EXTENSION)--2.0.0a1--$(EXTVERSION).sql: sql/topology_upgrade_minor.sql
	cp $< $@
	
sql/$(EXTENSION)--2.0.0a2--$(EXTVERSION).sql: sql/topology_upgrade_minor.sql
	cp $< $@
	
DATA = $(wildcard sql/*--*.sql) sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN += sql/$(EXTENSION)--$(EXTVERSION).sql sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
