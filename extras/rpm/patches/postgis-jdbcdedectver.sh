#!/bin/bash
JDBC_VERSION_RPM=`rpm -ql postgresql-jdbc| grep 'jdbc2.jar$'|awk -F '/' '{print $5}'`
sed 's/postgresql.jar/'${JDBC_VERSION_RPM}'/g' $MAKEFILE_DIR/Makefile > $MAKEFILE_DIR/Makefile.new
mv -f $MAKEFILE_DIR/Makefile.new $MAKEFILE_DIR/Makefile
