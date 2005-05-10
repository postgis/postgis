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

ifeq ($(findstring MINGW,$(HOST_OS)),MINGW)
SHLIB = lib$(NAME).$(SO_MAJOR_VERSION).$(SO_MINOR_VERSION)$(DLSUFFIX)
else
SHLIB = lib$(NAME)$(DLSUFFIX).$(SO_MAJOR_VERSION).$(SO_MINOR_VERSION)
endif

SHLIB_MAJOR     = lib$(NAME)$(DLSUFFIX).$(SO_MAJOR_VERSION)
SHLIB_BARE      = lib$(NAME)$(DLSUFFIX)
SHLIB_LINK 	= $(DLFLAGS) 

MODULE_FILENAME = $(LPATH)/$(SHLIB)

#---------------------------------------------------------------
# Postgis version and build date
#---------------------------------------------------------------

POSTGIS_VERSION = $(SO_MAJOR_VERSION).$(SO_MINOR_VERSION) USE_GEOS=$(USE_GEOS) USE_PROJ=$(USE_PROJ) USE_STATS=$(USE_STATS)
POSTGIS_LIB_VERSION = $(SO_MAJOR_VERSION).$(SO_MINOR_VERSION).$(SO_MICRO_VERSION)
POSTGIS_BUILD_DATE := $(shell date -u "+%Y-%m-%d %H:%M:%S")


#---------------------------------------------------------------

override CFLAGS += -g -O2 -fexceptions
override CFLAGS += -fPIC
#override CFLAGS += -I$(srcdir) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' 
override CFLAGS += -DUSE_VERSION=$(USE_VERSION)
override CFLAGS += -DPOSTGIS_LIB_VERSION='"$(POSTGIS_LIB_VERSION)"'
override CFLAGS += -DPOSTGIS_SCRIPTS_VERSION='"$(SCRIPTS_VERSION)"'
override CFLAGS += -DPOSTGIS_BUILD_DATE='"$(POSTGIS_BUILD_DATE)"'

JTS_OBJ=lwgeom_nojts.o
ifeq ($(USE_GEOS),1)
	override CFLAGS += -I$(GEOS_DIR)/include -DUSE_GEOS
	GEOS_RULES=detect_geos_version
	GEOS_WRAPPER=lwgeom_geos_wrapper.o
	JTS_OBJ=lwgeom_geos.o
	SHLIB_LINK += -lstdc++ -L$(GEOS_DIR)/lib -lgeos
endif

ifeq ($(USE_JTS),1)
	override CFLAGS += -I$(JTS_INCLUDES) -DUSE_JTS
	JTS_WRAPPER=lwgeom_jts_wrapper.o
	JTS_OBJ=lwgeom_jts.o
	SHLIB_LINK += -lgcj -ljts -lstdc++ -L$(JTS_LIBDIR) 
endif

ifeq ($(USE_PROJ),1)
	override CFLAGS += -I$(PROJ_DIR)/include -DUSE_PROJ 
	SHLIB_LINK += -L$(PROJ_DIR)/lib -lproj
endif

override CFLAGS += $(PGBEINCLUDES) -DAUTOCACHE_BBOX=$(AUTOCACHE_BBOX) 

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
 
SA_OBJS=measures.o box2d.o ptarray.o lwgeom_api.o lwgeom.o lwpoint.o lwline.o lwpoly.o lwmpoint.o lwmline.o lwmpoly.o lwcollection.o $(GEOS_WRAPPER) $(JTS_WRAPPER) wktunparse.o lwgparse.o wktparse.tab.o lex.yy.o vsprintf.o

OBJS=$(SA_OBJS) liblwgeom.o lwgeom_pg.o lwgeom_debug.o lwgeom_spheroid.o lwgeom_ogc.o lwgeom_functions_analytic.o $(JTS_OBJ) lwgeom_inout.o lwgeom_estimate.o lwgeom_functions_basic.o lwgeom_gist.o lwgeom_btree.o lwgeom_transform.o stringBuffer.o lwgeom_box.o lwgeom_box3d.o lwgeom_box2dfloat4.o lwgeom_chip.o lwgeom_svg.o lwgeom_gml.o lwgeom_triggers.o lwgeom_dump.o

#OTHERS=y.output lex.yy.c wktparse.tab.c wktparse.tab.h lwpostgis.sql
OTHERS=y.output lwpostgis.sql ../lwpostgis.sql postgis_geos_version.h


#---------------------------------------------------------------
# Makefile targets

all: $(GEOS_RULES) $(SHLIB) ../lwpostgis.sql

$(SHLIB): $(OBJS)
ifeq ($(findstring MINGW,$(HOST_OS)),MINGW)
	dlltool --export-all  --output-def lwgeom.def $(OBJS)
	dllwrap  -o $@ --dllname $@ --def lwgeom.def $(OBJS) $(SHLIB_LINK) $(PGBELIBS)
	dlltool --dllname $@  --def lwgeom.def --output-lib liblwgeom.a
else
	$(CC) $(SHLIB_LINK) -o $(SHLIB) $(OBJS)
endif

wktparse.tab.c: wktparse.y
	$(YACC) -vd -p lwg_parse_yy wktparse.y
	mv -f y.tab.c wktparse.tab.c
	mv -f y.tab.h wktparse.tab.h


lex.yy.c: wktparse.lex wktparse.tab.c
	$(FLEX) -Plwg_parse_yy -i -f -o'lex.yy.c' wktparse.lex 

lwgeom_jts_wrapper.o: lwgeom_jts_wrapper.cpp

lwgeom_geos_wrapper.o: lwgeom_geos_wrapper.cpp

lwgeom_geos.o: lwgeom_geos.c profile.h

lwgeom_jts.o: lwgeom_jts.c profile.h

lwgeom_nojts.o: lwgeom_nojts.c

lwgeom_functions_basic.o: lwgeom_functions_basic.c profile.h

# Shared library stuff

../lwpostgis.sql: lwpostgis.sql
	cp lwpostgis.sql ..

lwpostgis.sql: lwpostgis.sql.in
	cpp -P -traditional-cpp -DUSE_VERSION=$(USE_VERSION) $< | sed -e 's:@MODULE_FILENAME@:$(MODULE_FILENAME):g;s:@POSTGIS_VERSION@:$(POSTGIS_VERSION):g;s:@POSTGIS_SCRIPTS_VERSION@:$(SCRIPTS_VERSION):g;s/@POSTGIS_BUILD_DATE@/$(POSTGIS_BUILD_DATE)/g' | grep -v '^#' > $@

install: all install-lwgeom-lib install-lwgeom-scripts

install-lwgeom-scripts:
	@mkdir -p $(datadir)
	$(INSTALL_DATA) lwpostgis.sql $(datadir)/lwpostgis.sql
	$(INSTALL_DATA) ../spatial_ref_sys.sql $(datadir)/spatial_ref_sys.sql

#- This has been copied from postgresql and adapted
install-lwgeom-lib: $(SHLIB)
	@mkdir -p $(MODULE_INSTALLDIR)
	$(INSTALL_LIB) $< $(MODULE_INSTALLDIR)/$(SHLIB)

ifneq ($(PORTNAME), win)
ifneq ($(SHLIB), $(SHLIB_MAJOR))
	cd $(MODULE_INSTALLDIR) && \
	rm -f $(SHLIB_MAHOR) && \
	ln -sf $(SHLIB) $(SHLIB_MAJOR)
endif
ifneq ($(SHLIB), lib$(NAME)$(DLSUFFIX))
	cd $(MODULE_INSTALLDIR) && \
	rm -f $(SHLIB_BARE) && \
	ln -sf $(SHLIB) $(SHLIB_BARE)
endif
endif # not win

uninstall-lib: 
	rm -f $(MODULE_INSTALLDIR)/$(SHLIB)
	rm -f $(MODULE_INSTALLDIR)/$(SHLIB_MAJOR)
	rm -f $(MODULE_INSTALLDIR)/$(SHLIB_BARE)

#----------------------------------------------------------

detect_geos_version: 
	sh ../geos_version.sh $(GEOS_DIR) > postgis_geos_version.h

uninstall-lwgeom-scripts:
	rm -f $(datadir)/postgis/lwpostgis.sql
	rm -f $(datadir)/postgis/spatial_ref_sys.sql

uninstall: uninstall-lib uninstall-lwgeom-scripts

clean-lib:
	rm -f $(SHLIB) $(SHLIB_MAJOR) $(SHLIB_BARE)

clean distclean: clean-lib
	rm -f *.o *.so *.a test $(OTHERS)

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
