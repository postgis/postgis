#-----------------------------------------------------
#
# Configuration directives are in postgis_config.h 
#
#-----------------------------------------------------

all: postgis loaderdumper utils 

install: all postgis-install loaderdumper-install 

uninstall: postgis-uninstall loaderdumper-uninstall docs-uninstall 

clean: liblwgeom-clean postgis-clean loaderdumper-clean docs-clean test-clean 
	rm -f postgis.sql postgis_upgrade.sql

distclean: clean
	rm -Rf autom4te.cache
	rm -f config.log config.cache config.status 
	rm -f postgis_config.h

maintainer-clean: 
	@echo '------------------------------------------------------'
	@echo 'This command is intended for maintainers to use; it'
	@echo 'deletes files that may need special tools to rebuild.'
	@echo '------------------------------------------------------'
	$(MAKE) -C doc maintainer-clean
	$(MAKE) -C lwgeom maintainer-clean
	$(MAKE) -C jdbc2 maintainer-clean
	$(MAKE) distclean
	rm -f configure

test check: 
	$(MAKE) -C regress test

test-clean:
	$(MAKE) -C regress clean

liblwgeom:
	$(MAKE) -C liblwgeom 

liblwgeom-clean:
	$(MAKE) -C liblwgeom clean

postgis: liblwgeom
	$(MAKE) -C lwgeom 

postgis-clean:
	$(MAKE) -C lwgeom clean

postgis-install:
	$(MAKE) -C lwgeom install

postgis-uninstall:
	$(MAKE) -C lwgeom uninstall

loaderdumper:
	$(MAKE) -C loader

loaderdumper-clean:
	$(MAKE) -C loader clean

loaderdumper-install:
	$(MAKE) -C loader install

loaderdumper-uninstall:
	$(MAKE) -C loader uninstall

templategis:
	$(MAKE) -C extras/template_gis

templategis-clean:
	$(MAKE) -C extras/template_gis clean

templategis-install:
	$(MAKE) -C extras/template_gis install

templategis-uninstall:
	$(MAKE) -C extras/template_gis uninstall

docs: 
	$(MAKE) -C doc 

docs-clean:
	$(MAKE) -C doc clean


docs-install:
	$(MAKE) -C doc install

docs-uninstall:
	$(MAKE) -C doc uninstall

utils:
	$(MAKE) -C utils

configure: configure.in
	./autogen.sh

config.status: configure
	./configure

.PHONY: utils liblwgeom
