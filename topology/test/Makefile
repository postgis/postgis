DATABASE=postgis_topo_regress

all: 
	@echo
	@echo "make test - create the database, run all tests except sqlmm"
	@echo
	@echo "make initdb - create the regress database"
	@echo "make inittopo - create topology routines"
	@echo
	@echo "make load - load 'city_data' topology and feature tables"
	@echo "make loadmore - define more TopoGeoms from the topology"
	@echo "make validate - validate 'city_data' loaded topology"
	@echo 
	@echo "make hier - define some hierarchical TopoGeoms"
	@echo 
	@echo "make edit - test sqlmm editing functions"
	@echo
	@echo "make cache - create geom caches for features tables"
	@echo "make topopred - run predicates on topogeoms"
	@echo "make geompred - run predicates on geometries"
	@echo
	@echo "make invalid - create an invalid topology, and validate it"
	@echo "make sqlmm - test SQL/MM functions (exceptions are expected)"
	@echo

pred:
	@make geompred | grep -v make > /tmp/geompred.out
	@make topopred | grep -v make > /tmp/topopred.out
	@diff /tmp/geompred.out /tmp/topopred.out
	@diff /tmp/geompred.out predicate.expected

test: inittopo load loadmore hier cache pred invalid edit

initdb:
	createdb $(DATABASE)
	createlang plpgsql $(DATABASE)
	psql -f ../../lwpostgis.sql $(DATABASE)

inittopo:
	psql -f ../topology.sql $(DATABASE)

load: 
	psql -f load_topology.sql $(DATABASE)

loadmore:
	psql -f more_features.sql $(DATABASE)

hier:
	psql -f hierarchy.sql $(DATABASE)

validate: 
	psql -f validate_topology.sql $(DATABASE)

cache:
	psql -f cache_geometries.sql $(DATABASE)

invalid: 
	psql -f invalid_topology.sql $(DATABASE)

sqlmm:
	psql -f sqlmm_topology.sql $(DATABASE)

topopred: topo_predicates.sql
	@psql -tf topo_predicates.sql $(DATABASE)

geompred: geom_predicates.sql 
	@psql -tf geom_predicates.sql $(DATABASE)

topo_predicates.sql: predicates.sql.in
	cpp -P -traditional-cpp predicates.sql.in | sed -e 's:@COLUMN@:feature:g;s:@SCHEMA@:topology.:g' > topo_predicates.sql

geom_predicates.sql: predicates.sql.in
	cpp -P -traditional-cpp predicates.sql.in | sed -e 's:@COLUMN@:the_geom:g;s:@SCHEMA@::g' > geom_predicates.sql

edit:
	psql -f edit_topology.sql

clean distclean:
	rm -f geom_predicates.sql topo_predicates.sql
