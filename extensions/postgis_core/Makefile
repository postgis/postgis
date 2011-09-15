EXTENSION    = postgis_core
EXTVERSION   = $(shell grep default_version $(EXTENSION).control | \
               sed -e "s/default_version[[:space:]]*=[[:space:]]*'\([^']*\)'/\1/")

DATA         = $(filter-out $(wildcard sql/*--*.sql),$(wildcard sql/*.sql))

#DOCS         = $(wildcard ../../doc/html/*.html)
PG_CONFIG    = pg_config

PG91         = $(shell $(PG_CONFIG) --version | grep -qE " 8\.| 9\.0" && echo no || echo yes)
SQL_BITS     = $(wildcard sql_bits/*.sql)
EXTRA_CLEAN += sql/$(EXTENSION).sql ${SQL_BITS}

sql/$(EXTENSION).sql: $(SQL_BITS)
	cat $^ > $@

EXTRA_CLEAN += sql/$(EXTENSION).sql ${SQL_BITS}

ifeq ($(PG91),yes)
all: sql/$(EXTENSION)--$(EXTVERSION).sql

sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	cp $< $@
	
sql/$(EXTENSION).sql: sql_bits/postgis.sql sql_bits/spatial_ref_sys.sql
	cat $^ > $@
	
sql_bits/spatial_ref_sys.sql: ../../spatial_ref_sys.sql	
	sed -e 's/BEGIN;//g' -e 's/COMMIT;//g'  $< > $@
	#cp $< $@
#strip BEGIN/COMMIT since these are not allowed in extensions
sql_bits/postgis.sql: ../../postgis/postgis.sql
	sed -e 's/BEGIN;//g' -e 's/COMMIT;//g'  $< > $@

DATA = $(wildcard sql/*--*.sql) sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN += sql/$(EXTENSION)--$(EXTVERSION).sql
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
