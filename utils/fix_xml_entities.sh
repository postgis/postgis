#!/bin/sh

cd $(dirname $0)/..

perl -CSDL -pi \
	-e 's/\&([_a-zA-Z]*)[\s\h][\s\h]*;/&\1;/g;' \
	-e 's/&#x([0-9A-F]*)[\s\h][\s\h]*;/&#x\1;/g' \
	$(find doc/po/ -name *.po)
