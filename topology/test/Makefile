DATABASE=postgis_topo_regress

PSQL=psql

all: 
	@echo "Use 'make check' to run all tests"
#	@echo "make initdb - create the regress database"
#	@echo "make inittopo - create topology routines"
#	@echo
#	@echo "make load - load 'city_data' topology and feature tables"
#	@echo "make loadmore - define more TopoGeoms from the topology"
#	@echo "make validate - validate 'city_data' loaded topology"
#	@echo 
#	@echo "make hier - define some hierarchical TopoGeoms"
#	@echo 
#	@echo "make edit - test sqlmm editing functions"
#	@echo
#	@echo "make cache - create geom caches for features tables"
#	@echo "make topopred - run predicates on topogeoms"
#	@echo "make geompred - run predicates on geometries"
#	@echo
#	@echo "make invalid - create an invalid topology, and validate it"
#	@echo "make sqlmm - test SQL/MM functions (exceptions are expected)"
#	@echo

pred:
	@make geompred | grep -v make > /tmp/geompred.out
	@make topopred | grep -v make > /tmp/topopred.out
	@diff /tmp/geompred.out /tmp/topopred.out
	@diff /tmp/geompred.out predicate.expected

test: inittopo load loadmore hier cache pred invalid edit

initdb:
	createdb $(DATABASE)
	createlang plpgsql $(DATABASE)
	$(PSQL) -X -f ../../postgis/postgis.sql $(DATABASE)

inittopo:
	$(PSQL) -X -f ../topology.sql $(DATABASE)

load: 
	$(PSQL) -X -f load_topology.sql $(DATABASE)

loadmore:
	$(PSQL) -X -f more_features.sql $(DATABASE)

hier:
	$(PSQL) -X -f hierarchy.sql $(DATABASE)

validate: 
	$(PSQL) -X -f validate_topology.sql $(DATABASE)

cache:
	$(PSQL) -X -f cache_geometries.sql $(DATABASE)

invalid: 
	$(PSQL) -X -f invalid_topology.sql $(DATABASE)

topopred: topo_predicates.sql
	@$(PSQL) -X -tf topo_predicates.sql $(DATABASE)

geompred: geom_predicates.sql 
	@$(PSQL) -X -tf geom_predicates.sql $(DATABASE)

topo_predicates.sql: predicates.sql.in
	@cpp -P -traditional-cpp predicates.sql.in | sed -e 's:@COLUMN@:feature:g;s:@SCHEMA@:topology.:g' > topo_predicates.sql

geom_predicates.sql: predicates.sql.in
	@cpp -P -traditional-cpp predicates.sql.in | sed -e 's:@COLUMN@:the_geom:g;s:@SCHEMA@::g' > geom_predicates.sql

edit:
	$(PSQL) -X -f edit_topology.sql $(DATABASE)

clean distclean:
	rm -f geom_predicates.sql topo_predicates.sql

TESTS = regress/legacy_validate.sql regress/legacy_predicate.sql \
	regress/legacy_invalid.sql regress/sqlmm.sql \
	regress/legacy_query.sql regress/addnode.sql \
	regress/addedge.sql regress/addface.sql \
	regress/st_getfacegeometry.sql \
	regress/topoelement.sql \
	regress/topoelementarray_agg.sql \
	regress/droptopology.sql

check: topo_predicates.sql
	$(MAKE) -C ../../regress postgis.sql staged-install
	@USE_VERSION=$(POSTGIS_PGSQL_VERSION) ../../regress/run_test --topology $(TESTS)
