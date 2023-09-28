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
    -e '/^#\. Tag: function$/,/^$/d' \
    -e '/^#\. Tag: option$/,/^$/d' \
    -e '/^#\. Tag: varname$/,/^$/d' \
    -e '/^#\. Tag: code$/,/^$/d' \
    -e '/^#\. Tag: command$/,/^$/d' \
    -e '/^#\. Tag: literallayout$/,/^$/d' \
    -i $1

