#
# PostGIS Makefile
#
subdir = contrib/postgis

# Root of the pgsql source tree 
#top_builddir = ../..
top_builddir = /data3/postgresql-7.1.2/src

include $(top_builddir)/src/Makefile.global

test_db = geom_regress

# override libdir to install shlib in contrib not main directory
libdir := $(libdir)/contrib

# shared library parameters
NAME=postgis
SO_MAJOR_VERSION=0
SO_MINOR_VERSION=3

#override CPPFLAGS := -I$(srcdir) $(CPPFLAGS)
# Altered for Cynwin
override CPPFLAGS := -g  -I$(srcdir) $(CPPFLAGS) -DFRONTEND -DSYSCONFDIR='"$(sysconfdir)"'
override DLLLIBS := $(BE_DLLLIBS) $(DLLLIBS)

OBJS=postgis_debug.o postgis_ops.o postgis_fn.o postgis_inout.o

# Add libraries that libpq depends (or might depend) on into the
# shared library link.  (The order in which you list them here doesn't
# matter.)
SHLIB_LINK = $(filter -L%, $(LDFLAGS))

all: all-lib $(NAME).sql

# Shared library stuff
include $(top_srcdir)/src/Makefile.shlib

#$(NAME).sql: $(NAME).sql.in
#	sed -e 's:@MODULE_FILENAME@:$(libdir)/$(shlib):g' < $< > $@

$(NAME).sql: $(NAME).sql.in
	sed -e 's:@MODULE_FILENAME@:/data1/Refractions/Projects/PostGIS/work_dave/postgis/$(shlib):g' < $< > $@

install: all installdirs install-lib
	$(INSTALL_DATA) $(srcdir)/README.$(NAME)  $(docdir)/contrib
	$(INSTALL_DATA) $(NAME).sql $(datadir)/contrib

installdirs:
	$(mkinstalldirs) $(docdir)/contrib $(datadir)/contrib $(libdir)

uninstall: uninstall-lib
	@rm -f $(docdir)/contrib/README.$(NAME) $(datadir)/contrib/$(NAME).sql

clean distclean maintainer-clean: clean-lib
	@rm -f $(OBJS) $(NAME).sql

test: all
	csh regress/regress.csh $(test_db)
