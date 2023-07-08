#!/bin/sh

cd $(dirname $0)

SUPPORTED_LANGUAGES=$(grep ^translations ../doc/Makefile.in | cut -d= -f2)

exec > docs.yml.new

cat docs.yml | sed '/DO NOT EDIT/q'
echo
echo "# Supported languages $SUPPORTED_LANGUAGES"
echo


for lang in ${SUPPORTED_LANGUAGES};
do
  sed "s/@LANG@/$lang/" docs-localized.yml.in
done

mv -b docs.yml.new docs.yml
