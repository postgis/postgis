#!/bin/bash

dropdb $1
createdb $1
createlang plpgsql $1
psql $1 </usr/share/pgsql/contrib/lwpostgis.sql
