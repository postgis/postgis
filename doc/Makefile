COMMONOPTS = -f docbook -b html -e no-valid

include ../Makefile.config
include ../Version.config

LAST_RELEASE_VERSION = $(SO_MAJOR_VERSION).$(SO_MINOR_VERSION).$(SO_MICRO_VERSION)

#XSLBASE?=/usr/share/sgml/docbook/xsl-stylesheets
#XSLBASE = /usr/share/xml/docbook/stylesheet/nwalsh


all: html

postgis-out.xml: postgis.xml ../Version.config
	cat $< | sed "s/@@LAST_RELEASE_VERSION@@/$(LAST_RELEASE_VERSION)/g" > $@

chunked-html: postgis-out.xml
	xsltproc \
		--output html/ \
		--param shade.verbatim 1 \
		--param chunk.section.depth 0 \
		--param section.autolabel 1 \
		--param section.label.includes.component.label 1 \
		--stringparam html.stylesheet style.css \
		--stringparam saxon.character.representation decimal \
		$(XSLBASE)/html/chunk.xsl \
		$<

html/postgis.html: postgis-out.xml
	xsltproc \
		--output html/postgis.html \
		--param shade.verbatim 1 \
		--param section.autolabel 1 \
		--param section.label.includes.component.label 1 \
		--param chunk.section.depth 0 \
		--stringparam html.stylesheet style.css \
		$(XSLBASE)/html/docbook.xsl \
		$<

html: html/postgis.html

postgis.fo: postgis-out.xml
	xsltproc \
		--output $@ \
		--param shade.verbatim 1 \
		--param section.autolabel 1 \
		--param section.label.includes.component.label 1 \
		--param chunk.section.depth 0 \
		$(XSLBASE)/fo/docbook.xsl \
		$<

postgis.pdf: postgis.fo
	pdfxmltex $< > /dev/null ; \
	pdfxmltex $< > /dev/null

jw: postgis-out.xml
	jw $(COMMONOPTS) -o html/ postgis-out.xml

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
	@mkdir -p $(DESTDIR)$(docdir)
	@mkdir -p $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) html/postgis.html $(DESTDIR)$(docdir)/postgis.html
	$(INSTALL_DATA) man/pgsql2shp.1 $(DESTDIR)$(mandir)/man1/pgsql2shp.1
	$(INSTALL_DATA) man/shp2pgsql.1 $(DESTDIR)$(mandir)/man1/shp2pgsql.1
	$(INSTALL_DATA) ../README.postgis $(DESTDIR)$(docdir)/README.postgis

uninstall:
	rm -f $(DESTDIR)$(docdir)/postgis/postgis.html
	rm -f $(DESTDIR)$(docdir)/postgis/README.postgis
	rm -f $(DESTDIR)$(mandir)/man1/shp2pgsql.1
	rm -f $(DESTDIR)$(mandir)/man1/pgsql2shp.1

.PHONY: html 
