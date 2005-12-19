#-----------------------------------------------------
#
# Configuration directives are in Makefile.config
#
#-----------------------------------------------------

all: Makefile.config liblwgeom loaderdumper utils 

install: all liblwgeom-install loaderdumper-install 

uninstall: liblwgeom-uninstall loaderdumper-uninstall docs-uninstall

clean: Makefile.config liblwgeom-clean loaderdumper-clean docs-clean test-clean 
	rm -f lwpostgis.sql lwpostgis_upgrade.sql

distclean: clean
	rm -Rf autom4te.cache
	rm -f config.log config.cache config.status Makefile.config
	rm -f config.h

maintainer-clean: Makefile.config
	@echo '------------------------------------------------------'
	@echo 'This command is intended for maintainers to use; it'
	@echo 'deletes files that may need special tools to rebuild.'
	@echo '------------------------------------------------------'
	$(MAKE) -C doc maintainer-clean
	$(MAKE) -C lwgeom maintainer-clean
	$(MAKE) -C jdbc2 maintainer-clean
	$(MAKE) distclean
	rm -f configure

test: 
	$(MAKE) -C regress test

test-clean:
	$(MAKE) -C regress clean

liblwgeom: Makefile.config
	$(MAKE) -C lwgeom 

liblwgeom-clean:
	$(MAKE) -C lwgeom clean

liblwgeom-install:
	$(MAKE) -C lwgeom install

liblwgeom-uninstall:
	$(MAKE) -C lwgeom uninstall

loaderdumper: Makefile.config
	$(MAKE) -C loader

loaderdumper-clean:
	$(MAKE) -C loader clean

loaderdumper-install:
	$(MAKE) -C loader install

loaderdumper-uninstall:
	$(MAKE) -C loader uninstall

docs: Makefile.config
	$(MAKE) -C doc 

docs-clean: Makefile.config
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

Makefile.config: Makefile.config.in config.status 
	./config.status

config.h: config.h.in config.status
	./config.status

.PHONY: utils
