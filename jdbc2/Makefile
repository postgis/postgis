# PostGIS Makefile

# (C) 2004 Paul Ramsey, pramsey@refractions.net
# (C) 2005 Markus Schaber, schabios@logi-track.com
# (C) 2005 Sandro Santilli, strk@refractions.net
# (C) 2005 Alex Bodnaru, alexbodn@012.net.il

#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# Following are some variable definitions that may have to be changed
# according to your build environment. This can be done either by editing
# this Makefile, or setting them via shell environment prior to calling make.

### Config variable section starts ###

# Configure the helper executables used during build.

JAVAC?=javac
JAVA?=java
JAR?=jar
MKDIR?=mkdir -p
DELETE?=rm -rvf
CP?=cp

# If you happen to be stuck with ancient jar implementations that do not
# know about -u, but happen to have one that does not complain about
# duplicate directorys (e. G. debian woody fastjar), set the DEBUGJAR
# variable to postgis_debug.fastjar and JAR (above) to fastjar.
DEBUGJAR?=postgis_debug.jar

# Find our postgresql build directory - needed for "make installdirs" and 
# "make install" that are used by the debian packaging process. As everything
# else works without Makefile.global, we don't abort on inclusion failure
# and use late checking iside those rules instead.
ifeq (${PGSQL_SRC},)
	PGSQL_SRC=../../..
endif

top_builddir:=$(PGSQL_SRC)
top_makefile:=$(PGSQL_SRC)/src/Makefile.global
-include $(top_makefile)

# Make sure our classpath includes your postgresql.jar. This is somehow
# difficult because some JREs (e. G. those on debian woody or sablevm) seem
# to forget their own system classes (aka bootclasspath) as soon as _any_
# classpath is set, either via environment or via command line. They do not
# implement the concept of distinct classpath and bootclasspath settings,
# as most current java implementations do. This makes it necessary to either
# explicitly include the system classes in the set classpath, or use
# non-standard options line --addclasspath to add both the postgresql.jar
# and our own classes. We also cannot set an unset CLASSPATH variable here
# for this reason. Current (Jan 2005) implementations from SUN, IBM, 
# Blackdown, gij and kaffe are known to work fine.

# If you set POSTGIS_BUILD_CLASSPATH via Environment (even to an empty string), we
# simply use it and do not try to guess anything. If POSTGIS_CLASSPATH is
# not set, we use the contents of CLASSPATH. If CLASSPATH is empty or not
# set, we try to guess a default location that works on most unixoid systems.

ifeq (${CLASSPATH},)
	POSTGIS_BUILD_CLASSPATH?=/usr/share/java/postgresql.jar
else
	POSTGIS_BUILD_CLASSPATH?=${CLASSPATH}
endif

# Set those to --addclasspath for sablevm, to avoid overriding the
# BOOTCLASSPATH, the first one is for the compiler, the second one for JRE.
COMP_ADDCP?=-classpath
EXEC_ADDCP?=-classpath

# Class path entry separator, this is : on Linux/Unix and ; on Windows
CPSEP?=:

# This are used for the online regression tests, override via environment
# as applicable. The jtest needs table creation privilege in current_schema(),
# the others only call self contained SELECTs that do not access any tables.
PGHOST?=localhost
PGPORT?=5432
PGDATABASE?=jdbc_test
PGUSER?=psql
PGPASS?=guess

### Config variable section ends ###

# build internal constants follow:
SRCDIR=src
EXAMPLES=examples
BUILD=bin
RUNCP=$(POSTGIS_BUILD_CLASSPATH)$(CPSEP)postgis.jar
BUILDCP=$(STUBBUILD)$(CPSEP)$(POSTGIS_BUILD_CLASSPATH)$(CPSEP)$(SRCDIR)

# All the java files to compile
SRC= $(SRCDIR)/examples/Test.java \
		$(SRCDIR)/examples/TestAutoregister.java \
		$(SRCDIR)/examples/TestBoxes.java \
		$(SRCDIR)/examples/TestParser.java \
		$(SRCDIR)/examples/TestServer.java \
		$(SRCDIR)/examples/VersionPrinter.java \
		$(SRCDIR)/org/postgis/ComposedGeom.java \
		$(SRCDIR)/org/postgis/DriverWrapper.java \
		$(SRCDIR)/org/postgis/GeometryCollection.java \
		$(SRCDIR)/org/postgis/Geometry.java \
		$(SRCDIR)/org/postgis/LinearRing.java \
		$(SRCDIR)/org/postgis/LineString.java \
		$(SRCDIR)/org/postgis/MultiLineString.java \
		$(SRCDIR)/org/postgis/MultiPoint.java \
		$(SRCDIR)/org/postgis/MultiPolygon.java \
		$(SRCDIR)/org/postgis/PGbox2d.java \
		$(SRCDIR)/org/postgis/PGbox3d.java \
		$(SRCDIR)/org/postgis/PGboxbase.java \
		$(SRCDIR)/org/postgis/PGgeometry.java \
		$(SRCDIR)/org/postgis/PointComposedGeom.java \
		$(SRCDIR)/org/postgis/Point.java \
		$(SRCDIR)/org/postgis/Polygon.java \
		$(SRCDIR)/org/postgis/binary/BinaryParser.java \
		$(SRCDIR)/org/postgis/binary/ByteGetter.java \
		$(SRCDIR)/org/postgis/binary/ValueGetter.java \
		$(SRCDIR)/org/postgis/Version.java 

# The pgjdbc 7.2/7.4/8.0 compatibility stub stuff
STUBDIR=stubs
STUBBUILD=stubbin/
STUBSRC= $(STUBDIR)/org/postgresql/Connection.java \
	 $(STUBDIR)/org/postgresql/PGConnection.java

# What to put into the JAR files:
JARCONTENTS=-C $(BUILD) org \
		-C $(BUILD) examples \
		-C $(SRCDIR) org/postgresql/driverconfig.properties \
		-C . README COPYING_LGPL \
		-C .. COPYING

DEBUGJARCONTENTS=-C $(SRCDIR) org \
		-C $(SRCDIR) examples

# Preliminary jts support - not stable yet!
JTSDIR=jtssrc
JTSBUILD=jtsbin/
JTSSRC= $(JTSDIR)/examples/JtsTestParser.java \
	$(JTSDIR)/org/postgis/jts/JtsBinaryParser.java \
	$(JTSDIR)/org/postgis/jts/JtsGeometry.java \
	$(JTSDIR)/org/postgis/jts/JtsGisWrapper.java \
	$(JTSDIR)/org/postgis/jts/JtsWrapper.java \

# Now the makefile targets that do the work:

all:	jar \
	offlinetests

jar:	compile postgis.jar $(DEBUGJAR)

postgis.jar: compile $(SRCDIR)/org/postgresql/driverconfig.properties
	$(JAR) -cf postgis.jar $(JARCONTENTS)
	       
postgis_debug.jar: postgis.jar $(SRC)
	$(CP) postgis.jar postgis_debug.jar
	$(JAR) -uf postgis_debug.jar $(DEBUGJARCONTENTS)

# woody seems to have only broken jar implementations
postgis_debug.fastjar: compile $(SRC) $(SRCDIR)/org/postgresql/driverconfig.properties
	$(JAR) -cf postgis_debug.jar $(JARCONTENTS) $(DEBUGJARCONTENTS)

$(BUILD):
	$(MKDIR) $(BUILD)

$(STUBBUILD):
	$(MKDIR) $(STUBBUILD)

stubcompile: $(STUBBUILD) 
	$(JAVAC) -d $(STUBBUILD) $(STUBSRC)
	touch stubcompile

compile: stubcompile $(BUILD) $(SRC)
	$(JAVAC) $(COMP_ADDCP) "$(BUILDCP)" -d $(BUILD) $(SRC) 
	touch compile

test:	postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/Test
        
jtest:	postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestServer jdbc:postgresql://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

ptestoffline: postgis.jar	
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestParser offline

ptest:  postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestParser jdbc:postgresql_postGIS://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

boxtestoffline: postgis.jar	
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestBoxes offline

boxtest:  postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestBoxes jdbc:postgresql_postGIS://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

autoregistertest:  postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestAutoregister jdbc:postgresql://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

offlinetests: boxtestoffline ptestoffline test

onlinetests: boxtest ptest jtest autoregistertest

# Print all available versions
versions:  postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/VersionPrinter jdbc:postgresql://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

# boxtest and ptest include boxtestoffline and ptestoffline, so we only need
# to run test in addition to the onlinetests
alltests: onlinetests test

clean:  
	$(DELETE) $(BUILD) bin stubbin postgis.jar postgis_debug.jar \
		compile stubcompile jtscompile $(JTSBUILD) postgis_jts.jar

# Install - this needs the successfull inclusion of PostgreSQL top
# level Makefile (which is mandatory for the C part of PostGIS as well).
install: jar installdirs
	@echo Testing for successful inclusion of $(top_makefile)
	test $(top_makefile)
	$(INSTALL_DATA) postgis.jar $(DESTDIR)
	$(INSTALL_DATA) postgis_debug.jar $(DESTDIR)

installdirs:
	@echo Testing for successful inclusion of $(top_makefile)
	test $(DESTDIR)
	$(mkinstalldirs) $(DESTDIR)

# Preliminary JTS support

postgis_jts: postgis_jts.jar jtstestoffline
	@echo "Warning! This is beta code. Use at your own risk."

$(JTSBUILD):
	$(MKDIR) $(JTSBUILD)


jtscompile: compile stubcompile $(JTSBUILD) $(JTSSRC)
	$(JAVAC) $(COMP_ADDCP) "$(BUILD)$(CPSEP)$(CP)" -d $(JTSBUILD) $(JTSSRC) 
	touch jtscompile
	
jtstestoffline: jtscompile	
	$(JAVA) $(EXEC_ADDCP) "$(JTSBUILD)$(CPSEP)$(BUILD)$(CPSEP)$(CP)" $(EXAMPLES)/JtsTestParser offline

jtstest:  jtscompile
	$(JAVA) -$(EXEC_ADDCP) "$(JTSBUILD)$(CPSEP)$(BUILD)$(CPSEP)$(CP)" $(EXAMPLES)/JtsTestParser jdbc:postgres_jts://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

postgis_jts.jar: jtscompile $(SRCDIR)/org/postgresql/driverconfig.properties
	$(JAR) -cf postgis_jts.jar -C $(BUILD) . \
	       -C $(SRCDIR) org/postgresql/driverconfig.properties \
	       README COPYING_LGPL -C .. COPYING
	$(JAR) -uf postgis_jts.jar -C $(JTSBUILD) .


	
