#!/bin/sh

cd $(dirname $0)/..

perl -CSDL -pi \
	-e 's/\&([_a-zA-Z]*)[\s\h][\s\h]*;/&\1;/g' \
	$(find doc/po/ -name *.po)

perl -CSDL -pi \
	-e 's/\&([_a-zA-Z]*)[\s\h][\s\h]*;/&\1;/g' \
	$(find doc/po/ -name *.po)
