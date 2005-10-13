# Configuration Directives
include ../Makefile.config
include ../Version.config

#---------------------------------------------------------------
# Postgis version and build date
#---------------------------------------------------------------

all: topology.sql 

topology.sql: topology.sql.in
	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | grep -v '^#' > $@

install: all install-topology-scripts

uninstall: uninstall-topology-scripts

install-topology-scripts:
	@mkdir -p $(DESTDIR)$(datadir)
	$(INSTALL_DATA) topology.sql $(DESTDIR)$(datadir)/topology.sql

uninstall-topology-scripts:
	rm -f $(DESTDIR)$(datadir)/postgis/topology.sql

clean distclean: 
	rm -f topology.sql

