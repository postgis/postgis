# Configuration Directives

#---------------------------------------------------------------
# Set USE_PROJ to 1 for Proj4 reprojection support
#
USE_PROJ=1
ifeq (${PROJ_DIR},) 
	PROJ_DIR=/usr/local
endif

#---------------------------------------------------------------
# Set USE_GEOS to 1 for GEOS spatial predicate and operator
# support
#
USE_GEOS=1
ifeq (${GEOS_DIR},) 
	GEOS_DIR=/usr/local
endif

#---------------------------------------------------------------
# Set USE_STATS to 1 for new GiST statistics collection support
# Note that this support requires additional columns in 
# GEOMETRY_COLUMNS, so see the list archives for info or
# install a fresh database using postgis.sql
#
USE_STATS=1

#---------------------------------------------------------------
subdir=contrib/postgis

#---------------------------------------------------------------
# Default the root of the PostgreSQL source tree 
# To use a non-standard location set the PGSQL_SRC environment
# variable to the appropriate location.
#
ifeq (${PGSQL_SRC},) 
	PGSQL_SRC = ../..
endif

#---------------------------------------------------------------
# Path to library (to be specified in CREATE FUNCTION queries)
# Defaults to $libdir.
# Set LPATH environment variable to change it.
#
ifeq (${LPATH},)
	LPATH := \$$libdir
endif

#---------------------------------------------------------------
top_builddir = ${PGSQL_SRC}
include $(top_builddir)/src/Makefile.global

#---------------------------------------------------------------
# Default missing CXX variable to c++
# 
ifeq ($(CXX),) 
	CXX = c++
endif

#---------------------------------------------------------------
# Test the version string and set the USE_VERSION macro
# appropriately.
#
ifneq ($(findstring 7.1,$(VERSION)),)
	USE_VERSION=71
else
	ifneq ($(findstring 7.2,$(VERSION)),)
		USE_VERSION=72
	else
		ifneq ($(findstring 7.3,$(VERSION)),)
			USE_VERSION=73
		else
			ifneq ($(findstring 7.4,$(VERSION)),)
				USE_VERSION=74
			else
				USE_VERSION=75
			endif
		endif
	endif
endif

#---------------------------------------------------------------
# Shared library parameters.
#
NAME=postgis
SO_MAJOR_VERSION=0
SO_MINOR_VERSION=8
ifeq (${USE_VERSION}, 71) 
	MODULE_FILENAME = $(LPATH)/$(shlib)
	MODULE_INSTALLDIR = $(libdir)
else
	MODULE_FILENAME = $(LPATH)/$(shlib)
	MODULE_INSTALLDIR = $(pkglibdir)
endif

#---------------------------------------------------------------
# Postgis version
#---------------------------------------------------------------

POSTGIS_VERSION = $(SO_MAJOR_VERSION).$(SO_MINOR_VERSION) USE_GEOS=$(USE_GEOS) USE_PROJ=$(USE_PROJ) USE_STATS=$(USE_STATS)

#---------------------------------------------------------------

override CFLAGS += -g -fexceptions 
override CFLAGS += -I$(srcdir) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' 
override CFLAGS += -DUSE_VERSION=$(USE_VERSION)

ifeq ($(USE_GEOS),1)
	override CFLAGS += -I$(GEOS_DIR)/include/geos -DUSE_GEOS
endif
ifeq ($(USE_PROJ),1)
	override CFLAGS += -I$(PROJ_DIR)/include -DUSE_PROJ 
endif

override DLLLIBS += $(BE_DLLLIBS) 

override CXXFLAGS := $(CFLAGS)
# memory debug for gcc 2.91, 2.95, 3.0 and 3.1
# for gcc >= 3.2.2 set GLIBCPP_FORCE_NEW at runtime instead
#override CXXFLAGS += -D__USE_MALLOC

# this seems to be needed by gcc3.3.2 / Solaris7 combination
# as reported by  Havard Tveite <havard.tveite@nlh.no>
override CXXFLAGS += -fPIC

#---------------------------------------------------------------
# Add index selectivity to C flags
#
ifeq ($(USE_STATS),1)
	override CFLAGS += -DUSE_STATS
endif
 
#---------------------------------------------------------------
# Select proper GiST support C file
#
ifeq ($(USE_VERSION),71) 
	GIST_SUPPORT=71
	GIST_ESTIMATE=
else
	ifeq ($(USE_VERSION),75) 
		GIST_SUPPORT=75
	else
		GIST_SUPPORT=72
	endif
	GIST_ESTIMATE=postgis_estimate.o
endif

ifeq ($(USE_GEOS),1)
	GEOS_WRAPPER=postgis_geos_wrapper.o
endif

OBJS=postgis_debug.o postgis_ops.o postgis_fn.o postgis_inout.o postgis_proj.o postgis_chip.o postgis_transform.o postgis_gist_$(GIST_SUPPORT).o $(GIST_ESTIMATE) postgis_geos.o $(GEOS_WRAPPER) postgis_algo.o

#---------------------------------------------------------------
# Add libraries that libpq depends (or might depend) on into the
# shared library link.  (The order in which you list them here doesn't
# matter.)

SHLIB_LINK = $(filter -L%, $(LDFLAGS)) 
ifeq ($(USE_GEOS),1)
	SHLIB_LINK += -lstdc++ -L$(GEOS_DIR)/lib -lgeos
endif
ifeq ($(USE_PROJ),1)
	SHLIB_LINK += -L$(PROJ_DIR)/lib -lproj
endif
SHLIB_LINK += $(BE_DLLLIBS) 

#---------------------------------------------------------------
# Makefile targets

include $(top_srcdir)/src/Makefile.shlib

postgis_geos_wrapper.o: postgis_geos_wrapper.cpp

all: all-lib postgis.sql postgis_undef.sql loaderdumper

loaderdumper:
	$(MAKE) -C loader

# Shared library stuff

postgis_old.sql: postgis_sql_common.sql.in postgis_sql_$(USE_VERSION)_end.sql.in postgis_sql_$(USE_VERSION)_start.sql.in 
	cat postgis_sql_$(USE_VERSION)_start.sql.in postgis_sql_common.sql.in postgis_sql_$(USE_VERSION)_end.sql.in | sed -e 's:@MODULE_FILENAME@:$(MODULE_FILENAME):g;s:@POSTGIS_VERSION@:$(POSTGIS_VERSION):g'  > $@ 

postgis.sql: postgis.sql.in
	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | sed -e 's:@MODULE_FILENAME@:$(MODULE_FILENAME):g;s:@POSTGIS_VERSION@:$(POSTGIS_VERSION):g' > $@

postgis_undef.sql: postgis.sql create_undef.pl
	perl create_undef.pl $< $(USE_VERSION) > $@ 

install: all installdirs install-postgis-lib
	$(INSTALL_DATA) postgis.sql $(DESTDIR)$(datadir)
	$(INSTALL_DATA) postgis_undef.sql $(DESTDIR)$(datadir)
	$(INSTALL_DATA) spatial_ref_sys.sql $(DESTDIR)$(datadir)
	$(INSTALL_DATA) README.postgis $(DESTDIR)$(datadir)
	$(MAKE) DESTDIR=$(DESTDIR) -C loader install

#- This has been copied from postgresql and adapted
install-postgis-lib: $(shlib)
	$(INSTALL_SHLIB) $< $(DESTDIR)$(MODULE_INSTALLDIR)/$(shlib)
ifneq ($(PORTNAME), win)
ifneq ($(shlib), lib$(NAME)$(DLSUFFIX).$(SO_MAJOR_VERSION))
	cd $(DESTDIR)$(MODULE_INSTALLDIR) && \
	rm -f lib$(NAME)$(DLSUFFIX).$(SO_MAJOR_VERSION) && \
	$(LN_S) $(shlib) lib$(NAME)$(DLSUFFIX).$(SO_MAJOR_VERSION)
endif
ifneq ($(shlib), lib$(NAME)$(DLSUFFIX))
	cd $(DESTDIR)$(MODULE_INSTALLDIR) && \
	rm -f lib$(NAME)$(DLSUFFIX) && \
	$(LN_S) $(shlib) lib$(NAME)$(DLSUFFIX)
endif

endif # not win
#----------------------------------------------------------

installdirs:
	$(mkinstalldirs) $(docdir)/contrib $(datadir)/contrib $(libdir)

uninstall: uninstall-lib
	@rm -f $(docdir)/contrib/README.postgis $(datadir)/contrib/postgis.sql

clean distclean maintainer-clean: clean-lib
	@rm -f $(OBJS) postgis.sql postgis_undef.sql
	$(MAKE) -C loader clean
	$(MAKE) -C doc clean

