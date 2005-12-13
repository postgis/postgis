TMPDIR?=/tmp

include ../Makefile.config

TESTS=regress regress_index lwgeom_regress regress_lrs regress_proj removepoint setpoint

ifeq ($(USE_GEOS),1)
	TESTS += regress_ogc regress_bdpoly
endif

all: test cleanup

test:
	@./run_test $(TESTS)

cleanup:
	@sleep 1; dropdb postgis_reg

clean:
