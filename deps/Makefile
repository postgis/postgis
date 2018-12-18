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

CC=clang
CXX=clang++ -std=c++11
CFLAGS=-I../liblwgeom  -Wall -Wmissing-prototypes -std=gnu99 -fstrict-aliasing -march=native -mtune=native -O3 -pipe -fstack-protector-strong -fno-plt -Wall -Wextra -g3 -gdwarf-4 -fno-omit-frame-pointer -Wno-unused-parameter -Werror -fno-math-errno -fno-signed-zeros
CXXFLAGS=-I../liblwgeom  -Wall -Wmissing-prototypes -fstrict-aliasing -march=native -mtune=native -O3 -pipe -fstack-protector-strong -fno-plt -Wall -Wextra -g3 -gdwarf-4 -fno-omit-frame-pointer -Wno-unused-parameter -Werror
LDFLAGS = -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
top_builddir = ..
libdir = ${exec_prefix}/lib
LIBTOOL = $(SHELL) $(top_builddir)/libtool

all: libwagyu.la

libwagyu.la:
	$(MAKE) -C wagyu $@

install:

uninstall:

check:

clean:
	$(MAKE) -C wagyu $@

distclean: clean
	$(MAKE) -C wagyu $@
	rm -f Makefile
