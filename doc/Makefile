COMMONOPTS = -f docbook -b html -e no-valid
LAST_RELEASE_VERSION = 0.8

html: html/index.html

postgis-out.xml: postgis.xml
	cat $< | sed "s/@@LAST_RELEASE_VERSION@@/$(LAST_RELEASE_VERSION)/g" > $@

html/index.html: postgis-out.xml
	jw $(COMMONOPTS) -o html/ $<

clean:
	#rm -f html/*.html postgis-out.xml
