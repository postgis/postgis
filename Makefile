#-----------------------------------------------------
#
# Configuration directives are in Makefile.config
#
#-----------------------------------------------------

include Makefile.config

all: liblwgeom loaderdumper lwpostgis.sql

install: all liblwgeom-install loaderdumper-install install-lwgeom-scripts

uninstall: liblwgeom-uninstall loaderdumper-uninstall uninstall-lwgeom-scripts

clean: liblwgeom-clean loaderdumper-clean
	rm -f lwpostgis.sql

liblwgeom: 
	$(MAKE) -C lwgeom

liblwgeom-clean:
	$(MAKE) -C lwgeom clean

liblwgeom-install:
	$(MAKE) -C lwgeom install

liblwgeom-uninstall:
	$(MAKE) -C lwgeom uninstall

loaderdumper:
	$(MAKE) -C loader

loaderdumper-clean:
	$(MAKE) -C loader clean

loaderdumper-install:
	$(MAKE) -C loader install

loaderdumper-uninstall:
	$(MAKE) -C loader uninstall

lwpostgis.sql: lwpostgis.sql.in
	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | sed -e 's:@MODULE_FILENAME@:$(MODULE_FILENAME):g;s:@POSTGIS_VERSION@:$(POSTGIS_VERSION):g;s:@POSTGIS_SCRIPTS_VERSION@:$(SCRIPTS_VERSION):g' > $@

install-lwgeom-scripts:
	$(INSTALL_DATA) lwpostgis.sql $(DESTDIR)$(datadir)

uninstall-lwgeom-scripts:
	rm -f $(DESTDIR)$(datadir)/lwpostgis.sql

