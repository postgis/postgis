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
CP = $(CLASSPATH):src/:/usr/share/java/postgresql.jar

SRC=src
EXAMPLES=examples
BUILD=bin/make

all:	jar \
	test

jar:	compile
	$(JAR) -cf postgis.jar -C $(BUILD) . -C $(SRC)/org/postgresql/ postgresql.properties README	
	$(JAR) -cf postgis_debug.jar -C $(BUILD) . -C $(SRC) . README	

prepare:
	$(MKDIR) $(BUILD)


compile:	prepare
	$(JAVAC) -classpath $(CP) -d $(BUILD)\
		$(SRC)/examples/Test.java \
		$(SRC)/examples/TestParser.java \
		$(SRC)/examples/TestServer.java \
		$(SRC)/org/postgis/ComposedGeom.java \
		$(SRC)/org/postgis/DriverWrapper.java \
		$(SRC)/org/postgis/GeometryCollection.java \
		$(SRC)/org/postgis/Geometry.java \
		$(SRC)/org/postgis/LinearRing.java \
		$(SRC)/org/postgis/LineString.java \
		$(SRC)/org/postgis/MultiLineString.java \
		$(SRC)/org/postgis/MultiPoint.java \
		$(SRC)/org/postgis/MultiPolygon.java \
		$(SRC)/org/postgis/PGbox3d.java \
		$(SRC)/org/postgis/PGbox.java \
		$(SRC)/org/postgis/PGgeometry.java \
		$(SRC)/org/postgis/PointComposedGeom.java \
		$(SRC)/org/postgis/Point.java \
		$(SRC)/org/postgis/Polygon.java 

test:	compile
	$(JAVA) -classpath $(BUILD):$(CP) $(EXAMPLES)/Test
        
jtest:	compile
	$(JAVA) -classpath $(BUILD):$(CP) $(EXAMPLES)/TestServer

ptestoffline: compile	
	$(JAVA) -classpath $(BUILD):$(CP) $(EXAMPLES)/TestParser offline

ptest:  compile
	$(JAVA) -classpath $(BUILD):$(CP) $(EXAMPLES)/TestParser

clean:  
	$(DELETE) $(BUILD) postgis.jar postgis_debug.jar
     
