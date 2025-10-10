#!/bin/sh

# Fix malformed entities
perl -pi -e 's/&([#a-z_0-9]+)[^#a-z0-9;]+;/&\1;/g' $@

# Fix malformed CDATA
perl -pi -e 's/<[^[>\\]*!\[CDATA\[/<!\[CDATA\[/g' $@

# Fix malformed URIs (-C is for unicode)
perl -C -pi -e 's/(https?)[[:blank:]]*:/\1:/g' $@

# Fix smart quotes
perl -pi -e 's/”/\\"/g' $@
perl -pi -e 's/“/\\"/g' $@
perl -pi -e 's/«/\\"/g' $@
perl -pi -e 's/»/\\"/g' $@

