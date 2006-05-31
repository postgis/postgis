include ../../Makefile.config

#
# PostGIS template_gis Makefile
#

SCRIPTS_IN:=$(wildcard *.in)
SCRIPTS:=$(SCRIPTS_IN:.in=)
PUBLIC_SCRIPTS:=mktemplate_gis rmtemplate_gis createdb.postgis

SUBSTITUTE=-e s:@bindir@:$(bindir):g \
           -e s:@datadir@:$(datadir):g \
           -e s:@prefix@:$(prefix):g
SCRIPT_PREPARE=cat $(SCRIPT).in | sed $(SUBSTITUTE) > $(SCRIPT)
SCRIPT_TARGET=$(DESTDIR)$(bindir)/$(SCRIPT)
SCRIPT_INSTALL=$(INSTALL_BIN) $(SCRIPT) $(SCRIPT_TARGET)
SCRIPT_LINK=$(DESTDIR)$(prefix)/bin/$(SCRIPT)
SCRIPT_DOLINK=rm -f $(SCRIPT_LINK); \
              $(LN_S) -T $(SCRIPT_TARGET) $(SCRIPT_LINK)
SCRIPT_UNINSTALL=rm -f $(SCRIPT_TARGET)
SCRIPT_UNLINK=rm -f $(SCRIPT_LINK)
DEFAULTS_DIR=$(DESTDIR)/etc/default
SCRIPT_DEFAULTS=$(DEFAULTS_DIR)/postgis

all: $(SCRIPTS_IN)
	$(foreach SCRIPT, $(SCRIPTS), $(SCRIPT_PREPARE);)

install: all
	@mkdir -p $(DESTDIR)$(bindir)
	@mkdir -p $(DESTDIR)$(prefix)/bin
	$(foreach SCRIPT, $(SCRIPTS), $(SCRIPT_INSTALL);)
	$(foreach SCRIPT, $(PUBLIC_SCRIPTS), $(SCRIPT_DOLINK);)
	@mkdir -p $(DEFAULTS_DIR)
	cp profile $(SCRIPT_DEFAULTS)

uninstall:
	$(foreach SCRIPT, $(PUBLIC_SCRIPTS), $(SCRIPT_UNLINK);)
	$(foreach SCRIPT, $(SCRIPTS), $(SCRIPT_UNINSTALL);)

purge: uninstall
	rm -f $(SCRIPT_DEFAULTS)

clean distclean:
	rm -f $(SCRIPTS)
