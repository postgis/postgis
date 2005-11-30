TMPDIR?=/tmp

test: test-common test-index test-lwgeom test-ogc test-proj

test-lwgeom:
	@./run_lwgeom_regress

test-common:
	@./run_regress

test-index:
	@./run_index_regress

test-ogc:
	@./run_ogc_regress

test-proj:
	@./run_proj_regress

clean:
