#!/bin/sh

ant 1>&2

cat <<EOF

set search_path to public;

SELECT sqlj.replace_jar('file://${PWD}/postgis_pljava.jar', 'postgis_pljava_jar',  false);

EOF

cat functions.sql
