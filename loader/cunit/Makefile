# **********************************************************************
# * $Id: Makefile.in
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.refractions.net
# * Copyright 2008 Paul Ramsey, Mark Cave-Ayland
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

CC=gcc
CFLAGS=-m32  -Wall -Wmissing-prototypes

CUNIT_LDFLAGS= -lcunit
CUNIT_CPPFLAGS=-m32 -I..

# GTK includes and libraries
GTK_CFLAGS = -mms-bitfields -Ic:/gtkbundle/include/gtk-2.0 -Ic:/gtkbundle/lib/gtk-2.0/include -Ic:/gtkbundle/include/atk-1.0 -Ic:/gtkbundle/include/cairo -Ic:/gtkbundle/include/pango-1.0 -Ic:/gtkbundle/include/glib-2.0 -Ic:/gtkbundle/lib/glib-2.0/include -Ic:/gtkbundle/include/freetype2 -Ic:/gtkbundle/include -Ic:/gtkbundle/include/libpng12   
GTK_LIBS = -Lc:/gtkbundle/lib -lgtk-win32-2.0 -lgdk-win32-2.0 -latk-1.0 -lgio-2.0 -lgdk_pixbuf-2.0 -lpangowin32-1.0 -lgdi32 -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lintl   
GTK_WIN32_FLAGS = -mwindows

OBJS=	\
	cu_list.o \
	cu_tester.o

LOADER_OBJS= \
	../structure.o

# If we couldn't find the cunit library then display a helpful message
ifeq ($(CUNIT_LDFLAGS),)
all: requirements_not_met_cunit
check: requirements_not_met_cunit
else

$(LOADER_OBJS):
	make -C ..

# Build the unit tester
all: cu_tester

# Build and run the unit tester
check: cu_tester
	@./cu_tester

endif

# Build the main unit test executable
cu_tester: $(LOADER_OBJS) $(OBJS) 
	#$(CC) $(GTK_WIN32_FLAGS) $^ -o $@ $(GTK_LIBS) $(ICONV_LDFLAGS) $(PGSQL_FE_LDFLAGS) -lm $(CUNIT_LDFLAGS)
	$(CC)  $^ -o $@ -lm $(CUNIT_LDFLAGS)

# Command to build each of the .o files
$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) $(CUNIT_CPPFLAGS) $(GTK_CFLAGS) -c -o $@ $<

# Clean target
clean:
	@rm $(OBJS)
	@rm cu_tester.exe

# Requirements message
requirements_not_met_cunit:
	@echo
	@echo "WARNING:"
	@echo
	@echo "configure was unable to find CUnit which is required for unit testing."
	@echo "In order to enable unit testing, you must install CUnit and then re-run configure."
	@echo
