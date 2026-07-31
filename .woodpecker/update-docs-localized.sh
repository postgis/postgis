#!/bin/sh

cd $(dirname $0)

SUPPORTED_LANGUAGES=$(grep ^translations ../doc/Makefile.in | cut -d= -f2)
#SUPPORTED_LANGUAGES="ja de fr zh_Hans" # restrict built translations

# TODO: Put back pdf localized building once woodpecker docker image works with it
#  These are false errors as debbie builds the pdfs fine and tests anyway
#TARGETS="check-xml html cheatsheets check-cheatsheets pdf"
TARGETS="check-xml html cheatsheets check-cheatsheets"

cat docs.yml | sed '/DO NOT EDIT/q' > docs.yml.new

exec >> docs.yml.new
for target in ${TARGETS}
do
  echo "### TARGET ${target}"
  previous_step=
  for lang in ${SUPPORTED_LANGUAGES};
  do
    case ${target} in
      check-xml)
        if test -n "${previous_step}"; then
          depends_on="[ prepare, ${previous_step} ]"
        else
          depends_on=prepare
        fi
        ;;
      html)
        if test -n "${previous_step}"; then
          depends_on="[ check-xml-${lang}, ${previous_step} ]"
        else
          depends_on=check-xml-${lang}
        fi
        ;;
      cheatsheets)
        if test -n "${previous_step}"; then
          depends_on="[ check-xml-${lang}, ${previous_step} ]"
        else
          depends_on=check-xml-${lang}
        fi
        ;;
      pdf)
        depends_on="[ build-images, check-xml-${lang} ]"
        ;;
      check-cheatsheets)
        if test -n "${previous_step}"; then
          depends_on="[ cheatsheets-${lang}, build-cheatsheets, ${previous_step} ]"
        else
          depends_on="[ cheatsheets-${lang}, build-cheatsheets ]"
        fi
        ;;
      *)
        echo "Unexpected target ${target}" >&2
        exit 1
    esac
    sed "s/@LANG@/${lang}/;s/@TARGET@/${target}/;s/@DEP@/${depends_on}/" docs-localized.yml.in
    previous_step=${target}-${lang}
  done
done

sed -i '${/^$/d;}' docs.yml.new
mv -b docs.yml.new docs.yml
