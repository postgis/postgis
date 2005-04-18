include ../Makefile.config

#
# PostGIS Loader Makefile
#
SHELL = /bin/sh

subdir = contrib/postgis/loader
EXE = 

UNAME = $(shell uname)
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
	EXE = .exe
endif
ifeq ($(findstring MINGW,$(UNAME)),MINGW)
	EXE = .exe
endif

OBJS = shpopen.o dbfopen.o getopt.o

#---------------------------------------------------------------

ifeq ($(USE_ICONV),1)
	override CFLAGS += -DUSE_ICONV
endif

override CFLAGS := -g -Wall -I.. $(CFLAGS) -DUSE_VERSION=$(USE_VERSION)

all: shp2pgsql$(EXE) pgsql2shp$(EXE)

pgsql2shp.o: pgsql2shp.c
	$(CC) $(CFLAGS) $(PGFEINCLUDES) -c $<

pgsql2shp$(EXE): $(OBJS) pgsql2shp.o PQunescapeBytea.o
	$(CC) $(CFLAGS) $(PGFELIBS) $(OBJS) PQunescapeBytea.o pgsql2shp.o $(LDFLAGS) -o $@ 

shp2pgsql$(EXE): $(OBJS) shp2pgsql.o
	$(CC) $(CFLAGS) $(OBJS) shp2pgsql.o $(LDFLAGS) -o $@ 

install: all
	@mkdir -p $(bindir)
	$(INSTALL_BIN) pgsql2shp$(EXE) $(bindir)/pgsql2shp$(EXE)
	$(INSTALL_BIN) shp2pgsql$(EXE) $(bindir)/shp2pgsql$(EXE)

uninstall:
	rm -f $(bindir)/pgsql2shp$(EXE)
	rm -f $(bindir)/shp2pgsql$(EXE)

clean:
	rm -f $(OBJS) shp2pgsql.o pgsql2shp.o shp2pgsql$(EXE) pgsql2shp$(EXE)

