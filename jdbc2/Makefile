JAVAC = javac
JAVA = java
JAR = jar
MKDIR = mkdir -p
DELETE = rm -rvf

#
# Make sure you CLASSPATH environmental variable
# includes your postgresql.jar 
# 

#CP=src/:../../../src/interfaces/jdbc/jars/postgresql.jar
#CP=src/:lib/postgresql-8.0.309.jdbc3.jar
CP=$(CLASSPATH):src/:/usr/share/java/postgresql.jar

SRCDIR=src
EXAMPLES=examples
BUILD=bin/make
PGHOST?=localhost
PGPORT?=5432
PGDATABASE?=jdbc_test
PGUSER?=psql
PGPASS?=guess

SRC= $(SRCDIR)/examples/Test.java \
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


all:	jar \
	offlinetests

jar:	compile
	$(JAR) -cf postgis.jar -C $(BUILD) . -C $(SRCDIR) org/postgresql/postgresql.properties README	
	$(JAR) -cf postgis_debug.jar -C $(BUILD) . README
	$(JAR) -uf postgis_debug.jar -C $(SRCDIR) .

$(BUILD):
	$(MKDIR) $(BUILD)


compile: $(BUILD) $(SRC)
	$(JAVAC) -classpath "$(CP)" -d $(BUILD) $(SRC) 
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

offlinetests: boxtestoffline ptestoffline test

onlinetests: boxtest ptest jtest

# boxtest and ptest include boxtestoffline and ptestoffline, so we only need
# to run test in addition to the onlinetests
alltests: onlinetests test

clean:  
	$(DELETE) $(BUILD) bin postgis.jar postgis_debug.jar compile
     
