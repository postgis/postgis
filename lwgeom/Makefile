# Configuration Directives
include ../Makefile.config

#---------------------------------------------------------------
subdir=contrib/postgis

#---------------------------------------------------------------
# Default missing CXX variable to c++
# 
ifeq ($(CXX),) 
	CXX = c++
endif

#---------------------------------------------------------------
# Shared library parameters.
#
NAME=lwgeom
SO_MAJOR_VERSION=1
SO_MINOR_VERSION=0
SO_MICRO_VERSION=0RC1
SCRIPTS_VERSION=0.1.0
ifeq (${USE_VERSION},71) 
	MODULE_FILENAME = $(LPATH)/$(shlib)
	MODULE_INSTALLDIR = $(libdir)
else
	MODULE_FILENAME = $(LPATH)/$(shlib)
	MODULE_INSTALLDIR = $(pkglibdir)
endif

#---------------------------------------------------------------
# Postgis version and build date
#---------------------------------------------------------------

POSTGIS_VERSION = $(SO_MAJOR_VERSION).$(SO_MINOR_VERSION) USE_GEOS=$(USE_GEOS) USE_PROJ=$(USE_PROJ) USE_STATS=$(USE_STATS)
POSTGIS_LIB_VERSION = $(SO_MAJOR_VERSION).$(SO_MINOR_VERSION).$(SO_MICRO_VERSION)
POSTGIS_BUILD_DATE := $(shell date -u "+%F %H:%M:%S")


#---------------------------------------------------------------

override CFLAGS += -g -fexceptions
override CFLAGS += -fPIC
#override CFLAGS += -I$(srcdir) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' 
override CFLAGS += -DUSE_VERSION=$(USE_VERSION)
override CFLAGS += -DPOSTGIS_LIB_VERSION='"$(POSTGIS_LIB_VERSION)"'
override CFLAGS += -DPOSTGIS_SCRIPTS_VERSION='"$(SCRIPTS_VERSION)"'
override CFLAGS += -DPOSTGIS_BUILD_DATE='"$(POSTGIS_BUILD_DATE)"'

ifeq ($(USE_GEOS),1)
	override CFLAGS += -I$(GEOS_DIR)/include -DUSE_GEOS
	GEOS_RULES=detect_geos_version
	GEOS_WRAPPER=lwgeom_geos_wrapper.o
endif

ifeq ($(USE_PROJ),1)
	override CFLAGS += -I$(PROJ_DIR)/include -DUSE_PROJ 
endif

override CFLAGS += -I$(PROJ_DIR)/include -DAUTOCACHE_BBOX=$(AUTOCACHE_BBOX) 

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
 
SA_OBJS=measures.o box2d.o ptarray.o lwgeom_api.o lwgeom.o lwpoint.o lwline.o lwpoly.o lwmpoint.o lwmline.o lwmpoly.o lwcollection.o $(GEOS_WRAPPER) wktunparse.o lwgparse.o wktparse.tab.o lex.yy.o vsprintf.o
OBJS=$(SA_OBJS) liblwgeom.o lwgeom_pg.o lwgeom_debug.o lwgeom_spheroid.o lwgeom_ogc.o lwgeom_functions_analytic.o lwgeom_geos.o lwgeom_inout.o lwgeom_estimate.o lwgeom_functions_basic.o lwgeom_gist.o lwgeom_btree.o lwgeom_transform.o stringBuffer.o lwgeom_box.o lwgeom_box3d.o lwgeom_box2dfloat4.o lwgeom_chip.o lwgeom_svg.o lwgeom_gml.o lwgeom_triggers.o lwgeom_dump.o

OTHERS=y.output lex.yy.c wktparse.tab.c wktparse.tab.h lwpostgis.sql


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
	$(FLEX) -Plwg_parse_yy -i -f -o'lex.yy.c' wktparse.lex 

lwgeom_geos_wrapper.o: lwgeom_geos_wrapper.cpp

lwgeom_geos.o: lwgeom_geos.c profile.h

lwgeom_functions_basic.o: lwgeom_functions_basic.c profile.h

all: $(GEOS_RULES) $(shlib) lwpostgis.sql

# Shared library stuff

lwpostgis.sql: lwpostgis.sql.in
	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | sed -e 's:@MODULE_FILENAME@:$(MODULE_FILENAME):g;s:@POSTGIS_VERSION@:$(POSTGIS_VERSION):g;s:@POSTGIS_SCRIPTS_VERSION@:$(SCRIPTS_VERSION):g;s/@POSTGIS_BUILD_DATE@/$(POSTGIS_BUILD_DATE)/g' | grep -v ^# > $@
	cp lwpostgis.sql ..

install: all installdirs install-lwgeom-lib install-lwgeom-scripts

install-lwgeom-scripts:
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

detect_geos_version: 
	sh ../geos_version.sh $(GEOS_DIR) > postgis_geos_version.h

installdirs:
	$(mkinstalldirs) $(libdir)

uninstall-lwgeom-scripts:
	rm -f $(DESTDIR)$(datadir)/lwpostgis.sql

uninstall: uninstall-lib uninstall-lwgeom-scripts

clean distclean maintainer-clean: clean-lib
	rm -f $(OBJS) 
	rm -f $(OTHERS)
	rm -f liblwgeom_sa.o
	rm -f liblwgeom_sa.so
	rm -f test test.o

liblwgeom_sa.o: liblwgeom.c
	$(CC) -DSTANDALONE -o $@ -c $<

liblwgeom_sa.so: $(SA_OBJS) liblwgeom_sa.o 
	$(CC) -shared -o $@ $^ 

liblwgeom_sa.a: $(SA_OBJS) liblwgeom_sa.o 
	$(AR) -rc $@ $^ 


test: liblwgeom_sa.so liblwgeom_sa.a test.c
	#$(CC) -o test -L. -llwgeom_sa $(SHLIB_LINK) test.c
	$(CC) -Wall -lm -g -o test test.c liblwgeom_sa.a 

tsort:
	lorder $(SA_OBJS) | tsort
