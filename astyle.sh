#!/bin/bash

# Run astyle on the code base ready for release

# Find all "pure" C files in the codebase
#   - not .in.c used for .sql generation
#   - not lex.yy.c or wktparse.tab.c as these are generated files
CFILES=`find . -name '*.c' -not \( -name '*.in.c' -o -name 'wktparse.tab.c' -o -name 'lex.yy.c' \)`

for THISFILE in $CFILES;
do
	echo "Running astyle on $THISFILE..."

	# Rename the old version temporarily...
	mv $THISFILE $THISFILE.astyle

	# Run astyle
	astyle --style=ansi --indent=tab < $THISFILE.astyle > $THISFILE

	# Remove backup copy
	rm $THISFILE.astyle 
done

