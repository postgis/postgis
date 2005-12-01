TMPDIR?=/tmp

all: test cleanup

test: test-common test-index test-lwgeom test-ogc test-lrs test-proj

test-lwgeom:
	@./run_test lwgeom_regress

test-index:
	@./run_test regress_index

test-ogc:
	@./run_test regress_ogc

test-lrs:
	@./run_test regress_lrs

test-common:
	@./run_test regress

test-proj:
	@./run_test regress_proj


cleanup:
	dropdb postgis_reg

clean:
