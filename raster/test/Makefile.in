#############################################################################
# $Id$
#
# Copyright (c) 2009 Mateusz Loskot <mateusz@loskot.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#############################################################################

RT_CORE=../rt_core

all: check

core-check:
	$(MAKE) -C cunit check

check: core-check
	$(MAKE) -C regress check

clean:
	$(MAKE) -C cunit $@
	$(MAKE) -C regress $@

distclean: clean
	$(MAKE) -C cunit $@
	$(MAKE) -C regress $@
	rm -f Makefile
