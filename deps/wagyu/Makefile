#/**********************************************************************
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.net
# *
# * PostGIS is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, either version 2 of the License, or
# * (at your option) any later version.
# *
# * PostGIS is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
# *
# **********************************************************************
# *
# * Copyright 2018 Raúl Marín
# *
# **********************************************************************/

CXX = clang++ -std=c++11
CPPFLAGS =  -I/usr/include   -I/usr/include/libxml2 -I/usr/include -DHAVE_SFCGAL   -D_FORTIFY_SOURCE=2
CXXFLAGS=-I../../liblwgeom -Iinclude  -Wall -Wmissing-prototypes -fstrict-aliasing -march=native -mtune=native -O3 -pipe -fstack-protector-strong -fno-plt -Wall -Wextra -g3 -gdwarf-4 -fno-omit-frame-pointer -Wno-unused-parameter -Werror
LDFLAGS = -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
top_builddir = ../..
libdir = ${exec_prefix}/lib
LIBTOOL = $(SHELL) $(top_builddir)/libtool

WAGYU_OBJS = \
	lwgeom_wagyu.o

WAGYU_LT_OBJS = $(WAGYU_OBJS:.o=.lo)



all: libwagyu.la


libwagyu.la: $(WAGYU_LT_OBJS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -rpath $(libdir) $(WAGYU_LT_OBJS) $(LDFLAGS) -o $@



$(WAGYU_LT_OBJS): %.lo: %.cpp ../../liblwgeom.h
	$(LIBTOOL) --mode=compile $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

../../liblwgeom.h:
	$(MAKE) -C ../../liblwgeom liblwgeom.h


clean:
	rm -f libwagyu.la @WAGYU_OBJS@ @WAGYU_LT_OBJS@
	rm -rf .libs

distclean: clean
	rm -f Makefile


install:

uninstall:

check:

