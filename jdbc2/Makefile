# PostGIS Makefile

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

JAVAC = javac
JAVA = java
JAR = jar
MKDIR = mkdir -p
DELETE = rm -rvf

#
# Make sure you CLASSPATH environmental variable
# includes your postgresql.jar 
# 

CLASSPATH?=/usr/share/java/postgresql.jar
CP=$(CLASSPATH):src/

SRCDIR=src
EXAMPLES=examples
BUILD=bin
PGHOST?=localhost
PGPORT?=5432
PGDATABASE?=jdbc_test
PGUSER?=psql
PGPASS?=guess

SRC= $(SRCDIR)/examples/Test.java \
		$(SRCDIR)/examples/TestAutoregister.java \
		$(SRCDIR)/examples/TestBoxes.java \
		$(SRCDIR)/examples/TestParser.java \
		$(SRCDIR)/examples/TestServer.java \
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
		$(SRCDIR)/org/postgis/Polygon.java 

STUBDIR=stubs
STUBBUILD=stubbin/
STUBSRC= $(STUBDIR)/org/postgresql/Connection.java \
	$(STUBDIR)/org/postgresql/PGConnection.java


all:	jar \
	offlinetests

jar:	compile
	$(JAR) -cf postgis.jar -C $(BUILD) . \
	       -C $(SRCDIR) org/postgresql/driverconfig.properties \
	       README COPYING_LGPL -C .. COPYING
	$(JAR) -cf postgis_debug.jar -C $(BUILD) . README COPYING_LGPL -C .. COPYING
	$(JAR) -uf postgis_debug.jar -C $(SRCDIR) .

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

# boxtest and ptest include boxtestoffline and ptestoffline, so we only need
# to run test in addition to the onlinetests
alltests: onlinetests test

clean:  
	$(DELETE) $(BUILD) bin stubbin postgis.jar postgis_debug.jar compile stubcompile

# pgjdbc 7.2 special treating (needed e. G. for woody backport)
# Calling this rules multiple times does not really hurt, but litter
# your src directory with some .rej files.
woodypatch:
	patch -p0 -N <woody.patch

woodyunpatch:
	patch -p0 -R -N <woody.patch

