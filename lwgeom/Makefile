# Configuration Directives
include ../Makefile.config
include ../Version.config

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
SHLIB_LINK 	= $(DLFLAGS) 

#---------------------------------------------------------------
# Postgis version and build date
#---------------------------------------------------------------

POSTGIS_VERSION = $(REL_MAJOR_VERSION).$(REL_MINOR_VERSION) USE_GEOS=$(USE_GEOS) USE_PROJ=$(USE_PROJ) USE_STATS=$(USE_STATS)
POSTGIS_LIB_VERSION = $(REL_MAJOR_VERSION).$(REL_MINOR_VERSION).$(REL_MICRO_VERSION)
POSTGIS_BUILD_DATE := $(shell date -u "+%Y-%m-%d %H:%M:%S")
SCRIPTS_VERSION=$(POSTGIS_LIB_VERSION)


#---------------------------------------------------------------

CSTAR_FLAGS = -Wall -g -O2 -fPIC -fexceptions

override CFLAGS += -DUSE_VERSION=$(USE_VERSION)
override CFLAGS += -DPOSTGIS_LIB_VERSION='"$(POSTGIS_LIB_VERSION)"'
override CFLAGS += -DPOSTGIS_VERSION='"$(POSTGIS_VERSION)"'
override CFLAGS += -DPOSTGIS_SCRIPTS_VERSION='"$(SCRIPTS_VERSION)"'
override CFLAGS += -DPOSTGIS_BUILD_DATE='"$(POSTGIS_BUILD_DATE)"'

JTS_OBJ=lwgeom_nojts.o
ifeq ($(USE_GEOS),1)
	CSTAR_FLAGS += -DUSE_GEOS
	GEOS_RULES=
	CSTAR_FLAGS += -I$(GEOS_DIR)/include 
	GEOS_WRAPPER=
	JTS_OBJ=lwgeom_geos_c.o
	SHLIB_LINK += $(GEOS_LDFLAGS) -lgeos_c
endif

ifeq ($(USE_JTS),1)
	CXXFLAGS += -I$(JTS_INCLUDES)
	CSTAR_FLAGS += -DUSE_JTS
	JTS_WRAPPER=lwgeom_jts_wrapper.o
	JTS_OBJ=lwgeom_jts.o
	SHLIB_LINK += -lgcj -ljts -lstdc++ -L$(JTS_LIBDIR) 
endif

ifeq ($(USE_PROJ),1)
	override CFLAGS += -I$(PROJ_DIR)/include -DUSE_PROJ 
	SHLIB_LINK += -L$(PROJ_LIBDIR) -lproj
endif

override CFLAGS += $(PGBEINCLUDES) -DAUTOCACHE_BBOX=$(AUTOCACHE_BBOX) 

# memory debug for gcc 2.91, 2.95, 3.0 and 3.1
# for gcc >= 3.2.2 set GLIBCPP_FORCE_NEW at runtime instead
#override CXXFLAGS += -D__USE_MALLOC

#---------------------------------------------------------------
# Add index selectivity to C flags
#
ifeq ($(USE_STATS),1)
	override CFLAGS += -DUSE_STATS
endif

override CFLAGS += $(CSTAR_FLAGS)
override CXXFLAGS += $(CSTAR_FLAGS)

SA_OBJS=measures.o box2d.o ptarray.o lwgeom_api.o lwgeom.o lwpoint.o lwline.o lwpoly.o lwmpoint.o lwmline.o lwmpoly.o lwcollection.o $(GEOS_WRAPPER) $(JTS_WRAPPER) wktunparse.o lwgparse.o wktparse.tab.o lex.yy.o vsprintf.o

OBJS=$(SA_OBJS) liblwgeom.o lwgeom_pg.o lwgeom_debug.o lwgeom_spheroid.o lwgeom_ogc.o lwgeom_functions_analytic.o $(JTS_OBJ) lwgeom_inout.o lwgeom_estimate.o lwgeom_functions_basic.o lwgeom_gist.o lwgeom_btree.o lwgeom_transform.o stringBuffer.o lwgeom_box.o lwgeom_box3d.o lwgeom_box2dfloat4.o lwgeom_chip.o lwgeom_svg.o lwgeom_gml.o lwgeom_kml.o lwgeom_triggers.o lwgeom_dump.o lwgeom_functions_lrs.o long_xact.o lwcurve.o lwcompound.o lwcurvepoly.o lwmcurve.o lwmsurface.o lwgeom_sqlmm.o lwgeom_rtree.o

#OTHERS=y.output lex.yy.c wktparse.tab.c wktparse.tab.h lwpostgis.sql
OTHERS=y.output postgis_geos_version.h

all: $(GEOS_RULES) ../lwpostgis.sql ../regress/lwpostgis.sql ../lwpostgis_upgrade.sql all-shared-lib 

enable_shared=yes
include Makefile.shlib

MODULE_FILENAME = $(LPATH)/lib$(NAME)
REGRESS_MODULE_FILENAME = $(shell $(PWDREGRESS))/$(shlib)

#---------------------------------------------------------------
# Makefile targets

wktparse.tab.c: wktparse.y
	$(YACC) -vd -p lwg_parse_yy wktparse.y
	mv -f y.tab.c wktparse.tab.c
	mv -f y.tab.h wktparse.tab.h


lex.yy.c: wktparse.lex wktparse.tab.c
	$(FLEX) -Plwg_parse_yy -i -f -o'lex.yy.c' wktparse.lex 

lwgeom_jts_wrapper.o: lwgeom_jts_wrapper.cpp

lwgeom_geos_c.o: lwgeom_geos_c.c profile.h

lwgeom_jts.o: lwgeom_jts.c profile.h

lwgeom_nojts.o: lwgeom_nojts.c

lwgeom_functions_basic.o: lwgeom_functions_basic.c profile.h

# Shared library stuff

../lwpostgis_upgrade.sql: ../lwpostgis.sql ../utils/postgis_proc_upgrade.pl
	$(PERL) ../utils/postgis_proc_upgrade.pl ../lwpostgis.sql > ../lwpostgis_upgrade.sql

../lwpostgis.sql: lwpostgis.sql.in long_xact.sql.in sqlmm.sql.in sqldefines.h 
	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | sed -e 's:@MODULE_FILENAME@:$(MODULE_FILENAME):g;s:@POSTGIS_VERSION@:$(POSTGIS_VERSION):g;s:@POSTGIS_SCRIPTS_VERSION@:$(SCRIPTS_VERSION):g;s/@POSTGIS_BUILD_DATE@/$(POSTGIS_BUILD_DATE)/g' | grep -v '^#' > $@

../regress/lwpostgis.sql: lwpostgis.sql.in long_xact.sql.in sqlmm.sql.in sqldefines.h
	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | sed -e 's#@MODULE_FILENAME@#$(REGRESS_MODULE_FILENAME)#g;s#@POSTGIS_VERSION@#$(POSTGIS_VERSION)#g;s#@POSTGIS_SCRIPTS_VERSION@#$(SCRIPTS_VERSION)#g;s/@POSTGIS_BUILD_DATE@/$(POSTGIS_BUILD_DATE)/g' | grep -v '^#' > $@

install: all install-lib-shared install-lwgeom-scripts

install-lwgeom-scripts:
	@mkdir -p $(DESTDIR)$(datadir)
	$(INSTALL_DATA) ../lwpostgis.sql $(DESTDIR)$(datadir)/lwpostgis.sql
	$(INSTALL_DATA) ../lwpostgis_upgrade.sql $(DESTDIR)$(datadir)/lwpostgis_upgrade.sql
	$(INSTALL_DATA) ../spatial_ref_sys.sql $(DESTDIR)$(datadir)/spatial_ref_sys.sql

#----------------------------------------------------------

detect_geos_version: 
	sh ../geos_version.sh $(GEOS_DIR) > postgis_geos_version.h

uninstall-lwgeom-scripts:
	rm -f $(DESTDIR)$(datadir)/lwpostgis.sql
	rm -f $(DESTDIR)$(datadir)/lwpostgis_upgrade.sql
	rm -f $(DESTDIR)$(datadir)/spatial_ref_sys.sql

uninstall: uninstall-lib uninstall-lwgeom-scripts

clean-lwgeom-lib: clean-lib

clean distclean: clean-lwgeom-lib
	rm -f *.o *.so *.a ../lwpostgis.sql ../lwpostgis_upgrade.sql test $(OTHERS)

maintainer-clean: clean
	rm  -f lex.yy.c wktparse.tab.c wktparse.tab.h

liblwgeom_sa.o: liblwgeom.c
	$(CC) -DSTANDALONE -o $@ -c $<

liblwgeom_sa.so: $(SA_OBJS) liblwgeom_sa.o 
	$(CC) -shared -o $@ $^ 

liblwgeom_sa.a: $(SA_OBJS) liblwgeom_sa.o 
	$(AR) -rc $@ $^ 


test: liblwgeom_sa.so liblwgeom_sa.a test.c
	$(CC) -Wall -lm -g -o test test.c liblwgeom_sa.a 

tsort:
	lorder $(SA_OBJS) | tsort
