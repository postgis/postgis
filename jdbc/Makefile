JAVAC = javac
JAVA = java
JAR = jar

PGSQL_SRC ?= ${PWD}/../../..

CLASSPATH ?= $(PGSQL_SRC)/src/interfaces/jdbc/jars/postgresql.jar:.

top_builddir:=$(PGSQL_SRC)
include $(PGSQL_SRC)/src/Makefile.global

all: 	ogis \
	pgobjs \
	test

jar: ogis pgobjs
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

install: jar installdirs
	$(INSTALL_DATA) postgis.jar $(DESTDIR)

installdirs:
	$(mkinstalldirs) $(DESTDIR)

uninstall:
	rm -f $(DESTDIR)/postgis.jar

test:
	$(JAVAC) -classpath $(CLASSPATH) examples/Test.java
	$(JAVA) -classpath $(CLASSPATH) examples/Test
	

jtest:
	$(JAVAC) -classpath $(CLASSPATH) examples/TestServer.java
	$(JAVA) -classpath $(CLASSPATH) examples/TestServer

clean:
	rm -f postgis.jar
	rm -f org/postgis/*.class
	rm -f examples/*.class
