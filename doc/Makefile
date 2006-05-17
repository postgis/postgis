include ../Makefile.config
include ../Version.config

LAST_RELEASE_VERSION = $(SO_MAJOR_VERSION).$(SO_MINOR_VERSION).$(SO_MICRO_VERSION)

JW_COMMONOPTS = -f docbook -b html -e no-valid

XSLTPROC_COMMONOPTS = \
	--param section.autolabel 1 \
	--param section.label.includes.component.label 1 \
	--param chunk.section.depth 0 

XSLTPROC_HTMLOPTS = \
	--stringparam html.stylesheet style.css \

HTML_DOCBOOK_XSL=$(XSLBASE)/html/docbook.xsl 
FO_DOCBOOK_XSL=$(XSLBASE)/fo/docbook.xsl

all: $(DOC_RULE)

requirements_not_met:
	@echo
	@echo "Docs building requires 'xsltproc' or 'jw'."
	@echo "Configuration tool could not find any, either install"
	@echo "them or refer to online manual:"
	@echo
	@echo "  http://postgis.refractions.net/docs"
	@echo

postgis-out.xml: postgis.xml ../Version.config
	cat $< | sed "s/@@LAST_RELEASE_VERSION@@/$(LAST_RELEASE_VERSION)/g" > $@

chunked-html: postgis-out.xml
	$(XSLTPROC) $(XSLTPROC_COMMONOPTS) $(XSLTPROC_HTMLOPTS) \
		--output html/ \
		--stringparam saxon.character.representation decimal \
		$(HTML_DOXBOOK_XSL) \
		$<

html/postgis.html: postgis-out.xml
	$(XSLTPROC) $(XSLTPROC_COMMONOPTS) \
		--output html/postgis.html \
		$(FO_DOCBOOK_XSL) \
		$<

postgis.fo: postgis-out.xml
	$(XSLTPROC) $(XSLTPROC_COMMONOPTS) \
		--output $@ \
		$(XSLBASE)/fo/docbook.xsl \
		$<

html: html/postgis.html

postgis.pdf: postgis-out.xml
	@if test x"$(DB2PDF)" = x; then \
	   echo "Error: db2pdf not found, can't build posgis.pdf"; \
	   echo "       try installing docbook-utils package"; \
	   false; \
	else \
	   $(DB2PDF) $< && mv postgis-out.pdf postgis.pdf; \
	fi

# this is broken
_postgis.pdf: postgis.fo
	@if test x"$(PDFXMLTEX)" = x; then \
	   echo "Error: pdfxmltex not found, can't build posgis.pdf"; \
	   false; \
	else \
	   $(PDFXMLTEX) $<;  \
	fi

jw: postgis-out.xml
	$(JW) $(JW_COMMONOPTS) -o html/ postgis-out.xml

clean:
	@rm -f \
		postgis-out.xml \
		postgis.aux \
		postgis.fo \
		postgis.pdf \
		postgis.out \
		postgis.log

maintainer-clean: clean
	@rm -f html/*.html

install: html/postgis.html man/shp2pgsql.1 man/pgsql2shp.1
	@mkdir -p $(DESTDIR)$(docdir)/postgis
	@mkdir -p $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) html/postgis.html $(DESTDIR)$(docdir)/postgis/postgis.html
	$(INSTALL_DATA) ../README.postgis $(DESTDIR)$(docdir)/postgis/README.postgis
	$(INSTALL_DATA) man/pgsql2shp.1 $(DESTDIR)$(mandir)/man1/pgsql2shp.1
	$(INSTALL_DATA) man/shp2pgsql.1 $(DESTDIR)$(mandir)/man1/shp2pgsql.1

uninstall:
	rm -f $(DESTDIR)$(docdir)/postgis/postgis.html
	rm -f $(DESTDIR)$(docdir)/postgis/README.postgis
	rm -f $(DESTDIR)$(mandir)/man1/shp2pgsql.1
	rm -f $(DESTDIR)$(mandir)/man1/pgsql2shp.1

.PHONY: html 
