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
# * Copyright 2008 Mark Cave-Ayland
# *
# **********************************************************************/

CXX = clang++ -std=c++11
CPPFLAGS =  -I/usr/include   -I/usr/include/libxml2 -I/usr/include -DHAVE_SFCGAL   -D_FORTIFY_SOURCE=2
CXXFLAGS =  -Wall -Wmissing-prototypes -std=gnu99 -fstrict-aliasing -march=native -mtune=native -O3 -pipe -fstack-protector-strong -fno-plt -Wall -Wextra -g3 -gdwarf-4 -fno-omit-frame-pointer -Wno-unused-parameter -Werror -fno-math-errno -fno-signed-zeros
LDFLAGS = -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
LIBTOOL = $(SHELL) $(top_builddir)/libtool


all: libwagyu.la

install:

uninstall:

# Make all objects depend upon postgis_config.h and postgis_svn_revision.h
$(LT_OBJS): ../postgis_config.h ../postgis_svn_revision.h $(SA_HEADERS)

../postgis_svn_revision.h:
	$(MAKE) -C .. postgis_svn_revision.h

#libwagyu.la: $(LT_OBJS)
#	$(LIBTOOL) --tag=CC --mode=link $(CXX) -rpath $(libdir) $(LT_OBJS) \
#             -release $(SOVER) -version-info $(VERSION_INFO) $(LDFLAGS) -o $@
libwagyu.la:
	touch libwagyu.la

clean:
	rm -f libwagyu.la
	rm -rf .libs

distclean: clean
	rm -f Makefile

check:

