JAVAC = javac
JAVA = java
JAR = jar
CLASSPATH = ../../../src/interfaces/jdbc/jars/postgresql.jar:.

all: 	ogis \
	pgobjs \
	test

jar:
	$(JAR) -cf postgis.jar org/postgis/*.java org/postgis/*.class README	

ogis: 
	$(JAVAC) -classpath $(CLASSPATH) \
		org/postgis/Geometry.java \
		org/postgis/Point.java \
		org/postgis/MultiPoint.java \
		org/postgis/LineString.java \
		org/postgis/MultiLineString.java \
		org/postgis/LinearRing.java \
		org/postgis/Polygon.java \
		org/postgis/MultiPolygon.java


pgobjs:
	$(JAVAC) -classpath $(CLASSPATH) \
		org/postgis/PGgeometry.java \
		org/postgis/PGbox3d.java

test:
	$(JAVAC) -classpath $(CLASSPATH) examples/Test.java
	$(JAVA) -classpath $(CLASSPATH) examples/Test
	

jtest:
	$(JAVAC) -classpath $(CLASSPATH) examples/TestServer.java
	$(JAVA) -classpath $(CLASSPATH) examples/TestServer

