COMMONOPTS = -f docbook -b html -e no-valid

html: html/index.html

html/index.html: postgis.xml
	jw $(COMMONOPTS) -o html/ $<

clean:
	rm -f html/*.html
