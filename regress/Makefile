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
	rm -f $(TMPDIR)/regress_out
	rm -f $(TMPDIR)/lwgeom_regress_out
	rm -f $(TMPDIR)/regress_index_out
	rm -f $(TMPDIR)/regress_ogc_out
