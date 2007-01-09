TMPDIR?=/tmp

include ../Makefile.config

TESTS = \
	loader/Point \
	loader/PointM \
	loader/PointZ \
	loader/MultiPoint \
	loader/MultiPointM \
	loader/MultiPointZ \
	loader/Arc \
	loader/ArcM \
	loader/ArcZ \
	loader/Polygon \
	loader/PolygonM \
	loader/PolygonZ \
	regress \
	regress_index \
	regress_index_nulls \
	lwgeom_regress \
	regress_lrs \
	removepoint \
	setpoint \
	simplify \
	snaptogrid \
	affine \
	wkt \
	measures \
	long_xact \
	ctors

ifeq ($(USE_GEOS),1)
	TESTS += regress_ogc regress_bdpoly
endif

ifeq ($(USE_PROJ),1)
	TESTS += regress_proj
endif

all: test 

test check: lwpostgis.sql ../lwgeom/liblwgeom.so ../loader/pgsql2shp ../loader/shp2pgsql
	@USE_VERSION="$(USE_VERSION)" ./run_test $(TESTS)

lwpostgis.sql: ../lwgeom/lwpostgis.sql.in 
	$(MAKE) -C ../lwgeom ../regress/lwpostgis.sql

../lwgeom/liblwgeom.so:
	$(MAKE) -C ../lwgeom all-shared-lib

../loader/pgsql2shp:
	$(MAKE) -C ../loader pgsql2shp

../loader/shp2pgsql:
	$(MAKE) -C ../loader shp2pgsql

cleanup:
	@sleep 1
	@dropdb postgis_reg > /dev/null

clean:
	rm -f lwpostgis.sql
