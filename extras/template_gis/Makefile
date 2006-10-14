include ../../Makefile.config

#
# PostGIS template_gis Makefile
#

SCRIPTS_IN:=$(wildcard *.in)
SCRIPTS:=$(SCRIPTS_IN:.in=)
PUBLIC_SCRIPTS:=mktemplate_gis rmtemplate_gis createdb.postgis
SONAME:=$(shell grep ^REL_MAJOR_VERSION ../../Version.config | cut -d= -f2)

SUBSTITUTE=-e s:@bindir@:$(bindir):g \
           -e s:@datadir@:$(datadir):g \
           -e s:@prefix@:$(prefix):g \
           -e s:@SONAME@:$(SONAME):g
SCRIPT_PREPARE=cat $(SCRIPT).in | sed $(SUBSTITUTE) > $(SCRIPT)
SCRIPT_TARGETDIR=$(DESTDIR)$(bindir)
SCRIPT_TARGET=$(SCRIPT_TARGETDIR)/$(SCRIPT)
SCRIPT_INSTALL=$(INSTALL_BIN) $(SCRIPT) $(SCRIPT_TARGET)
SCRIPT_LINKDIR=$(DESTDIR)$(prefix)/bin
SCRIPT_LINK=$(SCRIPT_LINKDIR)/$(SCRIPT)
SCRIPT_DOLINK=rm -f $(SCRIPT_LINK); \
              $(LN_S) $(SCRIPT_TARGET) $(SCRIPT_LINKDIR)
SCRIPT_UNINSTALL=rm -f $(SCRIPT_TARGET)
SCRIPT_UNLINK=rm -f $(SCRIPT_LINK)
DEFAULTS_DIR=$(DESTDIR)/etc/default
SCRIPT_DEFAULTS=$(DEFAULTS_DIR)/postgis

all: $(SCRIPTS_IN)
	$(foreach SCRIPT, $(SCRIPTS), $(SCRIPT_PREPARE);)

install: all
	@mkdir -p $(SCRIPT_TARGETDIR)
	@mkdir -p $(SCRIPT_LINKDIR)
	$(foreach SCRIPT, $(SCRIPTS), $(SCRIPT_INSTALL);)
	$(foreach SCRIPT, $(PUBLIC_SCRIPTS), $(SCRIPT_DOLINK);)
	@mkdir -p $(DEFAULTS_DIR)
	cp profile $(SCRIPT_DEFAULTS)
	rm -f $(SCRIPT_TARGETDIR)/profile

uninstall:
	$(foreach SCRIPT, $(PUBLIC_SCRIPTS), $(SCRIPT_UNLINK);)
	$(foreach SCRIPT, $(SCRIPTS), $(SCRIPT_UNINSTALL);)

purge: uninstall
	rm -f $(SCRIPT_DEFAULTS)

clean distclean:
	rm -f $(SCRIPTS)

.PHONY: all install uninstall purge clean distclean
