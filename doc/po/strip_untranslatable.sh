#!/bin/sh

# This would be best done with ITS but it doesn't
# work very effectively at the moment
# See https://www.w3.org/TR/xml-i18n-bp/#relating-docbook-plus-its
# https://trac.osgeo.org/postgis/ticket/5074


sed -e '/^#\. Tag: programlisting$/,/^$/d' \
    -e '/^#\. Tag: screen$/,/^$/d' \
    -e '/^#\. Tag: funcsynopsis$/,/^$/d' \
    -e '/^#\. Tag: refname$/,/^$/d' \
    -e '/^#\. Tag: funcprototype$/,/^$/d' \
    -e '/^#\. Tag: option$/,/^$/d' \
    -i $1

