#!/bin/sh

cd $(dirname $0)

SUPPORTED_LANGUAGES=$(grep ^translations ../doc/Makefile.in | cut -d= -f2)
SUPPORTED_LANGUAGES="ja de fr" # restrict built translations

TARGETS="check-xml html cheatsheets check-cheatsheets pdf"

cat docs.yml | sed '/DO NOT EDIT/q' > docs.yml.new

cat docs.yml.new |
  sed '/# Common docs path/,/# END OF COMMON/!d;/# END OF COMMON/q' |
  sed 's/^/  /' > docs.yml.common_paths

exec >> docs.yml.new
for target in ${TARGETS}
do
  echo "### TARGET ${target}"
  for lang in ${SUPPORTED_LANGUAGES};
  do
    group=localized-${target}
    sed "s/@LANG@/${lang}/;s/@TARGET@/${target}/;s/@GROUP@/${group}/" docs-localized.yml.in
    # Add common docs paths
    cat docs.yml.common_paths
  done
done

mv -b docs.yml.new docs.yml
rm docs.yml.common_paths
