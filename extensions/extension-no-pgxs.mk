# Shared rules for SQL-only extension bundles.
#
# These Makefiles used to include PostgreSQL's PGXS makefile only to inherit
# generic data-file install and clean targets. Keeping those rules local removes
# that dependency from generated extension SQL bundles while preserving the
# installed file layout expected by PostgreSQL.

datadir ?= $(shell $(PG_CONFIG) --sharedir)
datamoduledir ?= extension
docdir ?= $(shell $(PG_CONFIG) --docdir)
docmoduledir ?= extension
srcdir ?= .
INSTALL ?= install -c
INSTALL_DATA ?= $(INSTALL) -m 644
MKDIR_P ?= mkdir -p

.PHONY: all install installdirs uninstall clean

all: $(DATA_built)

install: all installdirs
ifneq (,$(EXTENSION))
	$(INSTALL_DATA) $(addprefix $(srcdir)/, $(addsuffix .control, $(EXTENSION))) '$(DESTDIR)$(datadir)/extension/'
endif
ifneq (,$(DATA)$(DATA_built))
	$(INSTALL_DATA) $(addprefix $(srcdir)/, $(DATA)) $(DATA_built) '$(DESTDIR)$(datadir)/$(datamoduledir)/'
endif
ifdef DOCS
ifdef docdir
	$(INSTALL_DATA) $(addprefix $(srcdir)/, $(DOCS)) '$(DESTDIR)$(docdir)/$(docmoduledir)/'
endif
endif

installdirs:
ifneq (,$(EXTENSION))
	$(MKDIR_P) '$(DESTDIR)$(datadir)/extension'
endif
ifneq (,$(DATA)$(DATA_built))
	$(MKDIR_P) '$(DESTDIR)$(datadir)/$(datamoduledir)'
endif
ifdef DOCS
ifdef docdir
	$(MKDIR_P) '$(DESTDIR)$(docdir)/$(docmoduledir)'
endif
endif

uninstall:
ifneq (,$(EXTENSION))
	rm -f $(addprefix '$(DESTDIR)$(datadir)/extension'/, $(notdir $(addsuffix .control, $(EXTENSION))))
endif
ifneq (,$(DATA)$(DATA_built))
	rm -f $(addprefix '$(DESTDIR)$(datadir)/$(datamoduledir)'/, $(notdir $(DATA) $(DATA_built)))
endif
ifdef DOCS
ifdef docdir
	rm -f $(addprefix '$(DESTDIR)$(docdir)/$(docmoduledir)'/, $(DOCS))
endif
endif

clean:
ifdef DATA_built
	rm -f $(DATA_built)
endif
ifdef EXTRA_CLEAN
	rm -rf $(EXTRA_CLEAN)
endif
