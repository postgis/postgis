COMMONOPTS = -f docbook -b html -e no-valid
LAST_RELEASE_VERSION = 0.8


postgis-out.xml: postgis.xml
	cat $< | sed "s/@@LAST_RELEASE_VERSION@@/$(LAST_RELEASE_VERSION)/g" > $@

html: postgis-out.xml
	xsltproc \
		--stringparam shade.verbatim 1 \
		--stringparam html.stylesheet style.css \
		/usr/share/sgml/docbook/xsl-stylesheets/html/chunk.xsl \
		$<

clean:

