#!/bin/bash

# Run astyle on the code base ready for release

# If astyle doesn't exist, exit the script and do nothing
echo "FOO" | astyle > /dev/null
RET=$?
if [ $RET -ne 0 ]; then
	echo "Could not find astyle - aborting." 
	exit
fi

# Find all "pure" C files in the codebase
#   - not .in.c used for .sql generation
#   - not lex.yy.c or wktparse.tab.c as these are generated files
CFILES=`find . -name '*.c' -not \( -name '*.in.c' -o -name 'wktparse.tab.c' -o -name 'lex.yy.c' \)`

# Run the standard format on the files, and do not 
# leave .orig files around for altered files.
astyle --style=ansi --indent=tab --suffix=none $CFILES
