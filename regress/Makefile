TMPDIR?=/tmp

include ../Makefile.config

TESTS=regress regress_index lwgeom_regress regress_lrs removepoint setpoint simplify

ifeq ($(USE_GEOS),1)
	TESTS += regress_ogc regress_bdpoly
endif

ifeq ($(USE_PROJ),1)
	TESTS += regress_proj
endif

all: test cleanup

test:
	@./run_test $(TESTS)

cleanup:
	@sleep 1; dropdb postgis_reg > /dev/null

clean:
