TMPDIR?=/tmp

test: test-common test-index test-lwgeom test-ogc

test-lwgeom:
	@./run_lwgeom_regress

test-common:
	@./run_regress

test-index:
	@./run_index_regress

test-ogc:
	@./run_ogc_regress

clean:
