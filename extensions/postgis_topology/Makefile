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
sql_bits/topology.sql: ../../topology/topology.sql
	sed -e 's/BEGIN;//g' -e 's/COMMIT;//g'  $< > $@
	
sql_bits/topology_comments.sql: ../../doc/topology_comments.sql
	cp $< $@
	
#grep all lines that start with CREATE OR REPLACE FUNCTION
#then replace CREATE OR REPLACE .. with ALTER EXTENSION..;
#then remove default values
sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql: ../../topology/topology.sql
	sed -e '/^CREATE OR REPLACE FUNCTION/!d;' -e 's/CREATE OR REPLACE FUNCTION\(.*\)/ALTER EXTENSION $(EXTENSION) ADD FUNCTION\1;/' -e 's/DEFAULT [a-zA-Z]\+//g' $< > $@
DATA = $(wildcard sql/*--*.sql) sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN += sql/$(EXTENSION)--$(EXTVERSION).sql sql/$(EXTENSION)--unpackaged--$(EXTVERSION).sql
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
