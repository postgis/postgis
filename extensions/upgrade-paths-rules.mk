EXTDIR=$(DESTDIR)$(datadir)/$(datamoduledir)

install: install-upgrade-paths

# The "next" lines are a cludge to allow upgrading between different
# revisions of the same version
install-upgrade-paths:
	tpl='$(EXTENSION)--ANY--$(EXTVERSION).sql'; \
	$(INSTALL_DATA) sql/$${tpl} "$(EXTDIR)/$${tpl}"; \
	ln -fs "$${tpl}" $(EXTDIR)/$(EXTENSION)--$(EXTVERSION)--$(EXTVERSION)next.sql; \
	ln -fs "$${tpl}" $(EXTDIR)/$(EXTENSION)--$(EXTVERSION)next--$(EXTVERSION).sql; \
	for OLD_VERSION in $(UPGRADEABLE_VERSIONS); do \
		ln -fs "$${tpl}" $(EXTDIR)/$(EXTENSION)--$$OLD_VERSION--$(EXTVERSION).sql; \
	done

uninstall: uninstall-upgrade-paths

INSTALLED_UPGRADE_SCRIPTS = \
	$(wildcard $(EXTDIR)/*$(EXTVERSION).sql) \
	$(wildcard $(EXTDIR)/*$(EXTVERSION)next.sql) \
	$(NULL)

uninstall-upgrade-paths:
	rm -f $(INSTALLED_UPGRADE_SCRIPTS)
