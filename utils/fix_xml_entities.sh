#!/bin/sh

cd $(dirname $0)/..

perl -CSDL -pi \
	-e 's/\&([_a-z]*)[\s\h][\s\h]*;/&\1;/g' \
	$(find doc/po/ -name *.po)
