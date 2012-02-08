all: 
	$(MAKE) -C postgis all
	$(MAKE) -C postgis_topology all

clean distclean:
	$(MAKE) -C postgis clean
	$(MAKE) -C postgis_topology clean

install:
	$(MAKE) -C postgis install
	$(MAKE) -C postgis_topology install

uninstall:
	$(MAKE) -C postgis uninstall
	$(MAKE) -C postgis_topology uninstall

check:
	@echo "Nothing to check"

