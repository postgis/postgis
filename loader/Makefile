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

OBJS = shpopen.o dbfopen.o getopt.o PQunescapeBytea.o

#---------------------------------------------------------------

ifeq ($(USE_ICONV),1)
	override CFLAGS += -DUSE_ICONV
endif

override CFLAGS := -g -Wall -I.. -I$(srcdir) -I$(top_builddir)/src/interfaces/libpq $(CFLAGS) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' -DUSE_VERSION=$(USE_VERSION)

all: shp2pgsql$(EXE) pgsql2shp$(EXE)

pgsql2shp$(EXE): $(OBJS) pgsql2shp.o
	$(CC) $(CFLAGS) $(OBJS) pgsql2shp.o $(libpq) $(LDFLAGS) -o $@ 

shp2pgsql$(EXE): $(OBJS) shp2pgsql.o
	$(CC) $(CFLAGS) $(OBJS) shp2pgsql.o $(LDFLAGS) -o $@ 

install: all
	$(INSTALL_PROGRAM) pgsql2shp$(EXE) $(DESTDIR)$(bindir)/pgsql2shp$(EXE)
	$(INSTALL_PROGRAM) shp2pgsql$(EXE) $(DESTDIR)$(bindir)/shp2pgsql$(EXE)

uninstall:
	rm -f $(DESTDIR)$(bindir)/pgsql2shp$(EXE)
	rm -f $(DESTDIR)$(bindir)/shp2pgsql$(EXE)

clean:
	rm -f $(OBJS) shp2pgsql.o pgsql2shp.o shp2pgsql$(EXE) pgsql2shp$(EXE)

