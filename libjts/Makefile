JTSJAR?=jts-1.5.jar

CXXFLAGS = -g -O2 -Wall 
GCJFLAGS = -g -O2 -Wall 

JTSCLASSES = \
		com.vividsolutions.jts.geom.Geometry \
		com.vividsolutions.jts.geom.Point \
		com.vividsolutions.jts.geom.MultiPoint \
		com.vividsolutions.jts.geom.LineString \
		com.vividsolutions.jts.geom.LinearRing \
		com.vividsolutions.jts.geom.MultiLineString \
		com.vividsolutions.jts.geom.Polygon \
		com.vividsolutions.jts.geom.MultiPolygon \
		com.vividsolutions.jts.geom.GeometryCollection \
		com.vividsolutions.jts.geom.GeometryFactory \
		com.vividsolutions.jts.geom.PrecisionModel \
		com.vividsolutions.jts.geom.Coordinate \
		com.vividsolutions.jts.geom.CoordinateSequenceFactory \
		com.vividsolutions.jts.geom.CoordinateSequence \
		com.vividsolutions.jts.geom.IntersectionMatrix \
		com.vividsolutions.jts.io.WKTReader \
		com.vividsolutions.jts.io.WKTWriter  

all: libjts.so jts.h 

libjts.so: $(JTSJAR)
	gcj $(GCJFLAGS) -shared -o libjts.so $(JTSJAR)

jtsincludes: $(JTSJAR) 
	mkdir -p com/vividsolutions/jts/geom com/vividsolutions/jts/io
	gcjh -I $(JTSJAR) $(JTSCLASSES)

jts.h: $(JTSJAR) jtsincludes
	gcjh -I $(JTSJAR) \
		-stub \
		-o jts.h $(JTSCLASSES)

clean:
	rm -Rf com libjts.so jts.h
