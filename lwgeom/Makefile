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
	PGSQL_SRC = ../../..
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
				USE_VERSION=80
			endif
		endif
	endif
endif

#---------------------------------------------------------------
# Shared library parameters.
#
NAME=lwgeom
SO_MAJOR_VERSION=1
SO_MINOR_VERSION=0
SO_MICRO_VERSION=0
SCRIPTS_VERSION=1.0.0
ifeq (${USE_VERSION},71) 
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
POSTGIS_LIB_VERSION = $(SO_MAJOR_VERSION).$(SO_MINOR_VERSION).$(SO_MICRO_VERSION)

#---------------------------------------------------------------

override CFLAGS += -g 

override CFLAGS += -I$(srcdir) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' 
override CFLAGS += -DUSE_VERSION=$(USE_VERSION)
override CFLAGS += -DPOSTGIS_LIB_VERSION='"$(POSTGIS_LIB_VERSION)"'
override CFLAGS += -DPOSTGIS_SCRIPTS_VERSION='"$(SCRIPTS_VERSION)"'

ifeq ($(USE_GEOS),1)
	override CFLAGS += -I$(GEOS_DIR)/include -DUSE_GEOS
	#GEOS_RULES=detect_geos_version
	GEOS_WRAPPER=../postgis_geos_wrapper.o
endif

ifeq ($(USE_PROJ),1)
	override CFLAGS += -I$(PROJ_DIR)/include -DUSE_PROJ 
endif

override DLLLIBS += $(BE_DLLLIBS) 

override CXXFLAGS := $(CFLAGS)
# memory debug for gcc 2.91, 2.95, 3.0 and 3.1
# for gcc >= 3.2.2 set GLIBCPP_FORCE_NEW at runtime instead
#override CXXFLAGS += -D__USE_MALLOC

#---------------------------------------------------------------
# Add index selectivity to C flags
#
ifeq ($(USE_STATS),1)
	override CFLAGS += -DUSE_STATS
endif
 
OBJS=lwgeom_api.o lwgeom_functions_analytic.o lwgeom_geos.o lwgeom_inout.o lwgeom_estimate.o lwgeom_functions_basic.o lwgeom_gist.o lwgeom_btree.o lwgeom_transform.o stringBuffer.o lwgeom_box2dfloat4.o lex.yy.o wktparse.tab.o lwgparse.o wktunparse.o $(GEOS_WRAPPER)

OTHERS=y.output lex.yy.c wktparse.tab.c wktparse.tab.h lwgeom.sql lwpostgis.sql


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

wktparse.tab.c: wktparse.y
	$(YACC) -vd -p lwg_parse_yy wktparse.y
	mv -f y.tab.c wktparse.tab.c
	mv -f y.tab.h wktparse.tab.h


lex.yy.c: wktparse.lex wktparse.tab.c
	$(FLEX) -Plwg_parse_yy -if -o'lex.yy.c' wktparse.lex 

lwgeom_geos_wrapper.o: lwgeom_geos_wrapper.cpp

all: all-lib lwpostgis.sql

# Shared library stuff

#lwgeom.sql: lwgeom.sql.in
#	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | sed -e 's:@MODULE_FILENAME@:$(MODULE_FILENAME):g;s:@POSTGIS_VERSION@:$(POSTGIS_VERSION):g;s:@POSTGIS_SCRIPTS_VERSION@:$(SCRIPTS_VERSION):g' > $@

lwpostgis.sql: lwpostgis.sql.in
	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | sed -e 's:@MODULE_FILENAME@:$(MODULE_FILENAME):g;s:@POSTGIS_VERSION@:$(POSTGIS_VERSION):g;s:@POSTGIS_SCRIPTS_VERSION@:$(SCRIPTS_VERSION):g' > $@

install: all installdirs install-lwgeom-lib
	#$(INSTALL_DATA) lwgeom.sql $(DESTDIR)$(datadir)
	$(INSTALL_DATA) lwpostgis.sql $(DESTDIR)$(datadir)

#- This has been copied from postgresql and adapted
install-lwgeom-lib: $(shlib)
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
	@rm -f $(datadir)/contrib/lwgeom.sql

clean distclean maintainer-clean: clean-lib
	@rm -f $(OBJS) 
	rm -f $(OTHERS)

