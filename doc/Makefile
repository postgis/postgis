COMMONOPTS = -f docbook -b html -e no-valid
LAST_RELEASE_VERSION = 1.0.0

XSLBASE?=/usr/share/sgml/docbook/xsl-stylesheets


postgis-out.xml: postgis.xml
	cat $< | sed "s/@@LAST_RELEASE_VERSION@@/$(LAST_RELEASE_VERSION)/g" > $@

chunked-html: postgis-out.xml
	xsltproc \
		--param shade.verbatim 1 \
		--param chunk.section.depth 0 \
		--param section.autolabel 1 \
		--param section.label.includes.component.label 1 \
		--stringparam html.stylesheet style.css \
		--stringparam saxon.character.representation decimal \
		$(XSLBASE)/html/chunk.xsl \
		$<

html: postgis-out.xml
	xsltproc \
		--output html/postgis.html \
		--param shade.verbatim 1 \
		--param section.autolabel 1 \
		--param section.label.includes.component.label 1 \
		--param chunk.section.depth 0 \
		--stringparam html.stylesheet style.css \
		$(XSLBASE)/html/docbook.xsl \
		$<

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

