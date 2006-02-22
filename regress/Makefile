TMPDIR?=/tmp

include ../Makefile.config

TESTS=regress regress_index lwgeom_regress regress_lrs removepoint setpoint simplify snaptogrid affine

ifeq ($(USE_GEOS),1)
	TESTS += regress_ogc regress_bdpoly
endif

ifeq ($(USE_PROJ),1)
	TESTS += regress_proj
endif

all: test 

test: lwpostgis.sql ../lwgeom/liblwgeom.so
	@./run_test $(TESTS)

lwpostgis.sql: ../lwgeom/lwpostgis.sql.in 
	$(MAKE) -C ../lwgeom ../regress/lwpostgis.sql

../lwgeom/liblwgeom.so:
	$(MAKE) -C ../lwgeom all-shared-lib

cleanup:
	@sleep 1
	@dropdb postgis_reg > /dev/null

clean:
	rm lwpostgis.sql
