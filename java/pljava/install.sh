#!/bin/sh

cat <<EOF

set search_path to public;

SELECT sqlj.install_jar('file://${PWD}/postgis_pljava.jar', 'postgis_pljava_jar',  false);
SELECT sqlj.install_jar('file://${PWD}/lib/jts-1.7.1.jar', 'jts_171_jar',  false);


-- Set the class path on the schema you are using.
SELECT sqlj.set_classpath('public', 'postgis_pljava_jar:jts_171_jar');

EOF

cat functions.sql
