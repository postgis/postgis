COMMONOPTS = -f docbook -b html -e no-valid
LAST_RELEASE_VERSION = 0.8


postgis-out.xml: postgis.xml
	cat $< | sed "s/@@LAST_RELEASE_VERSION@@/$(LAST_RELEASE_VERSION)/g" > $@

chunked-html: postgis-out.xml
	xsltproc \
		--param shade.verbatim 1 \
		--param chunk.section.depth 0 \
		--stringparam html.stylesheet style.css \
		--stringparam saxon.character.representation decimal \
		/usr/share/sgml/docbook/xsl-stylesheets/html/chunk.xsl \
		$<

html: postgis-out.xml
	xsltproc \
		--output html/postgis.html \
		--param shade.verbatim 1 \
		--param chunk.section.depth 0 \
		--stringparam html.stylesheet style.css \
		/usr/share/sgml/docbook/xsl-stylesheets/html/docbook.xsl \
		$<

jw: postgis-out.xml
	jw $(COMMONOPTS) -o html/ postgis-out.xml

clean:

