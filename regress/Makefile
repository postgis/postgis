TMPDIR?=/tmp

include ../Makefile.config

all: test cleanup

test: test-common test-index test-lwgeom test-ogc test-lrs test-proj

test-lwgeom:
	@./run_test lwgeom_regress

test-index:
	@./run_test regress_index

test-ogc:
	@if test "$(USE_GEOS)" -eq 0 -a "$(USE_JTS)" -eq 0; then \
		echo "OGC tests skipped (no GEOS/JTS support compiled in)."; \
	else \
		./run_test regress_ogc; \
	fi

test-lrs:
	@./run_test regress_lrs

test-common:
	@./run_test regress

test-proj:
	@./run_test regress_proj


cleanup:
	@sleep 1; dropdb postgis_reg

clean:
