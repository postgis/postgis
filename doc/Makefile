COMMONOPTS = -f docbook -b html -e no-valid
LAST_RELEASE_VERSION = 0.8


postgis-out.xml: postgis.xml
	cat $< | sed "s/@@LAST_RELEASE_VERSION@@/$(LAST_RELEASE_VERSION)/g" > $@

html: postgis-out.xml
	xsltproc \
		--param shade.verbatim 1 \
		--param chunk.section.depth 0 \
		--stringparam html.stylesheet style.css \
		/usr/share/sgml/docbook/xsl-stylesheets/html/docbook.xsl \
		$<

clean:

