#!/bin/sh

cd $(dirname $0)

SUPPORTED_LANGUAGES=$(grep ^translations ../doc/Makefile.in | cut -d= -f2)

TARGETS="check html cheatsheets pdf"

cat docs.yml | sed '/DO NOT EDIT/q' > docs.yml.new

cat docs.yml.new | sed -n '/# Common docs path/,/^$/p' | grep -v '^$' | sed 's/^/  /' > docs.yml.common_paths

exec >> docs.yml.new
for target in ${TARGETS}
do
  for lang in ${SUPPORTED_LANGUAGES};
  do
    sed "s/@LANG@/${lang}/;s/@TARGET@/${target}/" docs-localized.yml.in
    # Add common docs paths
    cat docs.yml.common_paths
  done
done

mv -b docs.yml.new docs.yml
rm docs.yml.common_paths
