#-----------------------------------------------------
#
# Configuration directives are in Makefile.config
#
#-----------------------------------------------------

all: liblwgeom loaderdumper

install: all liblwgeom-install loaderdumper-install

uninstall: liblwgeom-uninstall loaderdumper-uninstall

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
