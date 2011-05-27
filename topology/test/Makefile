DATABASE=postgis_topo_regress

PSQL=psql

all: 
	@echo "Use 'make check' to run all tests"

# TODO: make edit_topology.sql into a proper test
edit:
	$(PSQL) -X -f edit_topology.sql $(DATABASE)


topo_predicates.sql: predicates.sql.in
	@cpp -P -traditional-cpp predicates.sql.in | sed -e 's:@COLUMN@:feature:g;s:@SCHEMA@:topology.:g' > topo_predicates.sql

load_topology.sql: load_topology.sql.in
	@cpp -P -traditional-cpp load_topology.sql.in | sed -e 's:@SRID@:-1:g' > load_topology.sql

load_topology-4326.sql: load_topology.sql.in
	@cpp -P -traditional-cpp load_topology.sql.in | sed -e 's:@SRID@:4326:g' > load_topology-4326.sql

clean distclean:
	rm -f topo_predicates.sql load_topology.sql load_topology-4326.sql

TESTS = regress/legacy_validate.sql regress/legacy_predicate.sql \
	regress/legacy_invalid.sql regress/sqlmm.sql \
	regress/legacy_query.sql regress/addnode.sql \
	regress/addedge.sql regress/addface.sql \
	regress/addface2.5d.sql \
	regress/polygonize.sql \
	regress/st_addisoedge.sql \
	regress/st_addisonode.sql \
	regress/st_addedgemodface.sql \
	regress/st_addedgenewfaces.sql \
	regress/st_changeedgegeom.sql \
	regress/st_getfacegeometry.sql \
	regress/st_getfaceedges.sql \
	regress/st_modedgeheal.sql \
	regress/st_newedgeheal.sql \
	regress/topoelement.sql \
	regress/topoelementarray_agg.sql \
	regress/topo2.5d.sql \
	regress/droptopology.sql \
	regress/copytopology.sql \
	regress/createtopogeom.sql \
	regress/createtopology.sql \
	regress/gml.sql \
	regress/getnodebypoint.sql \
	regress/getedgebypoint.sql \
	regress/getfacebypoint.sql \
	regress/getringedges.sql

check: topo_predicates.sql load_topology.sql load_topology-4326.sql
	$(MAKE) -C ../../regress postgis.sql staged-install
	@USE_VERSION=$(POSTGIS_PGSQL_VERSION) ../../regress/run_test $(RUNTESTFLAGS) --topology $(TESTS)
