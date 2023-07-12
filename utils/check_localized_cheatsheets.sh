#!/bin/sh

test -n "$3" || {
  echo "Usage: $0 <source_file> <source_lang> [<target_lang>, ...]" >&2
  exit 1
};

SOURCE_FILE="$1"
shift
SOURCE_LANG="$1"
shift
TARGET_LANGS="$@"

test -e "${SOURCE_FILE}" || {
  echo "No such file or directory: ${SOURCE_FILE}" >&2
  exit 1
}

# Read sup tags from source language cheatsheet
for tag in $(xmllint --html --xpath '/html/body/span/sup/text()' "${SOURCE_FILE}"); do
  count=$(grep -c "<sup> *$tag *</sup>" "${SOURCE_FILE}")
  echo "Occurrences of tag $tag in source file: $count"
  for lang in ${TARGET_LANGS}; do
    TR_FILE=$(echo "${SOURCE_FILE}" | sed "s/-${SOURCE_LANG}/-${lang}/");
    test -e "${TR_FILE}" || {
      echo "No such file or directory: ${TR_FILE}" >&2
      exit 1
    }
    count_lang=$(grep -c "<sup> *$tag *</sup>" "${TR_FILE}")
    echo "Occurrences of tag $tag in $lang file: $count_lang"
    if [ $count_lang != $count ]; then
      echo "Tag tag $tag occurs $count times in ${SOURCE_FILE} and $count_lang times in ${TR_FILE}" >&2
      exit 1
    fi
  done
done
