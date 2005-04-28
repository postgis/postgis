#-----------------------------------------------------
#
# Configuration directives are in Makefile.config
#
#-----------------------------------------------------

all: Makefile.config liblwgeom loaderdumper utils docs 

install: all liblwgeom-install loaderdumper-install docs-install

uninstall: liblwgeom-uninstall loaderdumper-uninstall docs-uninstall

clean: Makefile.config liblwgeom-clean loaderdumper-clean test-clean 
	rm -f lwpostgis.sql

distclean: clean
	rm -Rf autom4te.cache
	rm -f config.log config.cache config.status Makefile.config

maintainer-clean:
	@echo '------------------------------------------------------'
	@echo 'This command is intended for maintainers to use; it'
	@echo 'deletes files that may need special tools to rebuild.'
	@echo '------------------------------------------------------'
	$(MAKE) distclean
	$(MAKE) -C lwgeom maintainer-clean
	$(MAKE) -C jdbc2 maintainer-clean
	rm -f configure

test: liblwgeom
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

.PHONY: utils
