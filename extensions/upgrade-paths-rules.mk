EXTDIR=$(DESTDIR)$(datadir)/$(datamoduledir)

TAG_UPGRADE=$(EXTENSION)--TEMPLATED--TO--ANY.sql

PG_SHAREDIR=$(shell $(PG_CONFIG) --sharedir)

POSTGIS_BUILD_DATE=$(shell date $${SOURCE_DATE_EPOCH:+-d @$$SOURCE_DATE_EPOCH} -u "+%Y-%m-%d %H:%M:%S")

install: install-upgrade-paths

#
# Install <extension>--<curversion>--ANY.sql
#     and <extension>--ANY--<curversion>.sql
# upgrade paths
#
# The <curversion>--ANY file will be a symlink ( or copy,
# on systems not supporting symlinks ) to an empty upgrade template
# file named <extension>--TEMPLATED--TO--ANY.sql and be overwritten
# by any future versions of PostGIS, keeping the overall number of
# upgrade paths down.
#
# See https://trac.osgeo.org/postgis/ticket/5092
#
install-upgrade-paths: sql/$(TAG_UPGRADE) sql/$(EXTENSION)--ANY--$(EXTVERSION).sql
	mkdir -p "$(EXTDIR)"
	$(INSTALL_DATA) "sql/$(EXTENSION)--ANY--$(EXTVERSION).sql" "$(EXTDIR)/$(EXTENSION)--ANY--$(EXTVERSION).sql"
	$(INSTALL_DATA) "sql/$(TAG_UPGRADE)" "$(EXTDIR)/$(TAG_UPGRADE)"
	ln -fs "$(TAG_UPGRADE)" "$(EXTDIR)/$(EXTENSION)--$(EXTVERSION)--ANY.sql"

install-extension-upgrades-from-known-versions:
	$(PERL) $(top_srcdir)/loader/postgis.pl \
		install-extension-upgrades \
		--extension $(EXTENSION) \
		--pg_sharedir $(DESTDIR)$(PG_SHAREDIR) \
		$(UPGRADEABLE_VERSIONS)

all: sql/$(TAG_UPGRADE)

sql/$(TAG_UPGRADE): $(MAKEFILE_LIST) | sql
	echo '-- Just tag extension $(EXTENSION) version as "ANY"' > $@
	echo '-- Installed by $(EXTENSION) $(EXTVERSION)' >> $@
	echo '-- Built on $(POSTGIS_BUILD_DATE)' >> $@

uninstall: uninstall-upgrade-paths

INSTALLED_UPGRADE_SCRIPTS = \
	$(wildcard $(EXTDIR)/$(EXTENSION)--*--$(EXTVERSION).sql) \
	$(wildcard $(EXTDIR)/$(EXTENSION)--*--ANY.sql) \
	$(NULL)

uninstall-upgrade-paths:
	rm -f $(INSTALLED_UPGRADE_SCRIPTS)
