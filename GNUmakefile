#-----------------------------------------------------
#
# Configuration directives are in postgis_config.h 
#
#-----------------------------------------------------

all: postgis loaderdumper utils
	@echo "PostGIS was built successfully. Ready to install." 

install: all postgis-install loaderdumper-install

uninstall: postgis-uninstall loaderdumper-uninstall docs-uninstall comments-uninstall

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
	$(MAKE) -C postgis maintainer-clean
	$(MAKE) -C java/jdbc maintainer-clean
	$(MAKE) distclean
	rm -f configure

test check: postgis
	$(MAKE) -C liblwgeom/cunit check
	$(MAKE) -C regress check

test-clean:
	$(MAKE) -C regress clean

liblwgeom:
	$(MAKE) -C liblwgeom 

liblwgeom-clean:
	$(MAKE) -C liblwgeom clean

postgis: liblwgeom
	$(MAKE) -C postgis 

postgis-clean:
	$(MAKE) -C postgis clean

postgis-install:
	$(MAKE) -C postgis install

postgis-uninstall:
	$(MAKE) -C postgis uninstall

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

comments:
	$(MAKE) -C doc comments

comments-install:
	$(MAKE) -C doc comments-install

comments-uninstall:
	$(MAKE) -C doc comments-uninstall

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

ChangeLog.svn:
	svn2cl --authors=authors.svn -i -o ChangeLog.svn

astyle:
	./astyle.sh

.PHONY: utils liblwgeom ChangeLog.svn
