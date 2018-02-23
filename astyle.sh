#!/bin/bash

# Run astyle on the code base ready for release

# Find all "pure" C files in the codebase
#   - not .in.c used for .sql generation
#   - not lex.yy.c or wktparse.tab.c as these are generated files
CFILES=`find . -name '*.c' -o -name '*.h' -not \( -name '*_parse.c' -o -name '*_lex.c' \)`

# Run the standard format on the files, and do not
# leave .orig files around for altered files.
clang-format-7 -i --style=file $CFILES
