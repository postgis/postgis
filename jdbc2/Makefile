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


# Configure the helper executables used during build - change this according to
# your environment if necessary.

JAVAC?=javac
JAVA?=java
JAR?=jar
MKDIR?=mkdir -p
DELETE?=rm -rvf

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

# Make sure you CLASSPATH environmental variable includes your postgresql.jar
# We set a reasonable default for un*xoid systems here. Note that some JREs
# (e. G. sablevm on debian woody) seem to need their own system classes
# to be included in the classpath, because they do not adhere to the standard
# concept of -classpath and -bootclasspath. SUN jdk, IBM jdk, Blackdown JDK,
# gij and kaffe are known to work fine.

CLASSPATH?=/usr/share/java/postgresql.jar
CP=$(CLASSPATH):src/

# This are used for the online regression tests, override via environment
# as applicable.
PGHOST?=localhost
PGPORT?=5432
PGDATABASE?=jdbc_test
PGUSER?=psql
PGPASS?=guess

# build internal constants follow:
SRCDIR=src
EXAMPLES=examples
BUILD=bin

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
		$(SRCDIR)/org/postgis/PGgeometry.java \
		$(SRCDIR)/org/postgis/PointComposedGeom.java \
		$(SRCDIR)/org/postgis/Point.java \
		$(SRCDIR)/org/postgis/Polygon.java \
		$(SRCDIR)/org/postgis/Version.java 

STUBDIR=stubs
STUBBUILD=stubbin/
STUBSRC= $(STUBDIR)/org/postgresql/Connection.java \
	 $(STUBDIR)/org/postgresql/PGConnection.java

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

jar:	compile postgis.jar postgis_debug.jar

postgis.jar: compile $(SRCDIR)/org/postgresql/driverconfig.properties
	$(JAR) -cf postgis.jar -C $(BUILD) . \
	       -C $(SRCDIR) org/postgresql/driverconfig.properties \
	       -C . README COPYING_LGPL \
	       -C .. COPYING
	       
postgis_debug.jar: compile $(SRC) $(SRCDIR)/org/postgresql/driverconfig.properties
	$(JAR) -cf postgis_debug.jar -C $(BUILD) . \
		-C . README COPYING_LGPL \
		-C .. COPYING
	$(JAR) -uf postgis_debug.jar -C $(SRCDIR) .
# We cannot add $(SRCDIR) with -cf because some jar implementations
# abort with file name conflicts because of $(BUILD)/org and $(SRCDIR)/org.
# Thus we have to use -uf above.

$(BUILD):
	$(MKDIR) $(BUILD)

$(STUBBUILD):
	$(MKDIR) $(STUBBUILD)

stubcompile: $(STUBBUILD) 
	$(JAVAC) -d $(STUBBUILD) $(STUBSRC)
	touch stubcompile

compile: stubcompile $(BUILD) $(SRC)
	$(JAVAC) -classpath "$(STUBBUILD):$(CP)" -d $(BUILD) $(SRC) 
	touch compile

test:	compile
	$(JAVA) -classpath "$(BUILD):$(CP)" $(EXAMPLES)/Test
        
jtest:	compile
	$(JAVA) -classpath "$(BUILD):$(CP)" $(EXAMPLES)/TestServer jdbc:postgresql://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

ptestoffline: compile	
	$(JAVA) -classpath "$(BUILD):$(CP)" $(EXAMPLES)/TestParser offline

ptest:  compile
	$(JAVA) -classpath "$(BUILD):$(CP)" $(EXAMPLES)/TestParser jdbc:postgresql_postGIS://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

boxtestoffline: compile	
	$(JAVA) -classpath "$(BUILD):$(CP)" $(EXAMPLES)/TestBoxes offline

boxtest:  compile
	$(JAVA) -classpath "$(BUILD):$(CP)" $(EXAMPLES)/TestBoxes jdbc:postgresql_postGIS://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

autoregistertest:  compile
	$(JAVA) -classpath "$(BUILD):$(CP)" $(EXAMPLES)/TestAutoregister jdbc:postgresql://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

offlinetests: boxtestoffline ptestoffline test

onlinetests: boxtest ptest jtest autoregistertest

# Print all available versions
versions:  compile
	$(JAVA) -classpath "$(BUILD):$(CP)" $(EXAMPLES)/VersionPrinter jdbc:postgresql://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

# boxtest and ptest include boxtestoffline and ptestoffline, so we only need
# to run test in addition to the onlinetests
alltests: onlinetests test

clean:  
	$(DELETE) $(BUILD) bin stubbin postgis.jar postgis_debug.jar \
		compile stubcompile jtscompile $(JTSBUILD) postgis_jts.jar

# The following two targets are needed for debian build, but may be hepful
# for someone else, too.
install: jar installdirs
	@echo Testing for successful inclusion of $(top_makefile)
	test $(DESTDIR)
	test $(INSTALL_DATA)
	$(INSTALL_DATA) postgis.jar $(DESTDIR)

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
	$(JAVAC) -classpath "$(BUILD):$(CP)" -d $(JTSBUILD) $(JTSSRC) 
	touch jtscompile
	
jtstestoffline: jtscompile	
	$(JAVA) -classpath "$(JTSBUILD):$(BUILD):$(CP)" $(EXAMPLES)/JtsTestParser offline

jtstest:  jtscompile
	$(JAVA) -classpath "$(JTSBUILD):$(BUILD):$(CP)" $(EXAMPLES)/JtsTestParser jdbc:postgres_jts://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

postgis_jts.jar: jtscompile $(SRCDIR)/org/postgresql/driverconfig.properties
	$(JAR) -cf postgis_jts.jar -C $(BUILD) . \
	       -C $(SRCDIR) org/postgresql/driverconfig.properties \
	       README COPYING_LGPL -C .. COPYING
	$(JAR) -uf postgis_jts.jar -C $(JTSBUILD) .


	