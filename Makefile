# Configuration Directives
#---------------------------------------------------------------
# Set USE_PG72 to 1 for PostgreSQL version >= 7.2
USE_PG72=1
#---------------------------------------------------------------
# Set USE_PROJ to 1 for Proj4 reprojection support
USE_PROJ=0

#---------------------------------------------------------------
subdir=contrib/postgis

#---------------------------------------------------------------
# Default the root of the PostgreSQL source tree 
# To use a non-standard location set the PGSQL_SRC environment
# variable to the appropriate location.
ifeq (${PGSQL_SRC},) 
	top_builddir = ../..
	include $(top_builddir)/src/Makefile.global
	libdir := $(libdir)/contrib
else
	top_builddir = ${PGSQL_SRC}
	include $(top_builddir)/src/Makefile.global
	libdir := ${PWD}
endif

TEST_DB=geom_regress

#---------------------------------------------------------------
# shared library parameters
NAME=postgis
SO_MAJOR_VERSION=0
SO_MINOR_VERSION=7

#override CPPFLAGS := -I$(srcdir) $(CPPFLAGS)
# Altered for Cynwin
ifeq ($(USE_PROJ),1)
	override CPPFLAGS := -g  -I$(srcdir) $(CPPFLAGS) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' -DUSE_PROJ
else
	override CPPFLAGS := -g  -I$(srcdir) $(CPPFLAGS) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"' 
endif
override DLLLIBS := $(BE_DLLLIBS) $(DLLLIBS)

ifeq ($(USE_PG72),1)
	OBJS=postgis_debug.o postgis_ops.o postgis_fn.o postgis_inout.o postgis_proj.o postgis_chip.o postgis_transform.o postgis_gist_72.o
else
	OBJS=postgis_debug.o postgis_ops.o postgis_fn.o postgis_inout.o postgis_proj.o postgis_chip.o postgis_transform.o postgis_gist.o
endif

# Add libraries that libpq depends (or might depend) on into the
# shared library link.  (The order in which you list them here doesn't
# matter.)
SHLIB_LINK=$(filter -L%, $(LDFLAGS)) 
ifeq ($(USE_PROJ),1)
	SHLIB_LINK=$(filter -L%, $(LDFLAGS)) -lproj
else
	SHLIB_LINK=$(filter -L%, $(LDFLAGS)) 
endif

all: all-lib $(NAME).sql $(NAME).sql $(NAME)_undef.sql loaderdumper

loaderdumper:
	make -C loader

# Shared library stuff
include $(top_srcdir)/src/Makefile.shlib

$(NAME).sql: $(NAME).sql.in $(NAME)_gist_72.sql.in $(NAME)_gist.sql.in
	sed -e 's:@MODULE_FILENAME@:$(libdir)/$(shlib):g;s:@POSTGIS_VERSION@:$(SO_MAJOR_VERSION).$(SO_MINOR_VERSION):g' < $(NAME).sql.in > $@ 
	if [ $(USE_PG72) -eq 1 ]; then \
		sed -e 's:@MODULE_FILENAME@:$(libdir)/$(shlib):g;s:@POSTGIS_VERSION@:$(SO_MAJOR_VERSION).$(SO_MINOR_VERSION):g' < $(NAME)_gist_72.sql.in >> $(NAME).sql; \
	else \
		sed -e 's:@MODULE_FILENAME@:$(libdir)/$(shlib):g;s:@POSTGIS_VERSION@:$(SO_MAJOR_VERSION).$(SO_MINOR_VERSION):g' < $(NAME)_gist.sql.in >> $(NAME).sql; \
	fi

$(NAME)_undef.sql: $(NAME).sql
	perl create_undef.pl $< > $@ 

install: all installdirs install-lib
	$(INSTALL_DATA) $(srcdir)/README.$(NAME)  $(docdir)/contrib
	$(INSTALL_DATA) $(NAME).sql $(datadir)/contrib
	$(INSTALL_DATA) $(NAME)_undef.sql $(datadir)/contrib
	$(INSTALL_DATA) spatial_ref_sys.sql $(datadir)/contrib
	$(INSTALL_DATA) README.postgis $(datadir)/contrib
	make -C loader install

installdirs:
	$(mkinstalldirs) $(docdir)/contrib $(datadir)/contrib $(libdir)

uninstall: uninstall-lib
	@rm -f $(docdir)/contrib/README.$(NAME) $(datadir)/contrib/$(NAME).sql

clean distclean maintainer-clean: clean-lib
	@rm -f $(OBJS) $(NAME).sql $(NAME)_undef.sql
	make -C loader clean

test: all
	csh regress/regress.csh $(TEST_DB)
