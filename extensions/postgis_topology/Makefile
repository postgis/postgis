EXTENSION    = postgis_topology
EXTVERSION   = $(shell grep default_version $(EXTENSION).control | sed -e "s/default_version[[:space:]]*=[[:space:]]*'\([^']*\)'/\1/")

DATA         = $(filter-out $(wildcard sql/*--*.sql),$(wildcard sql/*.sql))

PG_CONFIG    = pg_config

PG91         = $(shell $(PG_CONFIG) --version | grep -qE " 8\.| 9\.0" && echo no || echo yes)
SQL_BITS     = $(wildcard sql_bits/*.sql)
EXTRA_CLEAN += sql/$(EXTENSION).sql ${SQL_BITS}


ifeq ($(PG91),yes)
all: sql/$(EXTENSION)--$(EXTVERSION).sql sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql

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
		-e 's/DEFAULT [a-zA-Z]\+//g' \
		-e 's/\(BEFORE\|AS\)\(.*\)/;/' \
		-e 's/(;/;/' \
		-e 's/\\(;/;/' \
		-e 's/;;/;/g' $< > $@
DATA = $(wildcard sql/*--*.sql) sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN += sql/$(EXTENSION)--$(EXTVERSION).sql sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
