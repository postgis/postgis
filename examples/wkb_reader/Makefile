#
# Edit the following variables to point to your PostgreSQL
# client libraries and include files
#
PG_INC=/usr/include/pgsql
PG_LIB=/usr/lib

#
# Below should be fine
#
CFLAGS=-I$(PG_INC) -L$(PG_LIB) -lpq

all: readwkb

readwkb: readwkb.c
