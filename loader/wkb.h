/*
 * OGC WellKnownBinaryGeometry
 */

/*
 * 3d geometries are encode as in OGR by adding WKB3DOFFSET to the type.  
 */
#define WKB3DOFFSET 0x80000000

typedef char byte;
typedef unsigned int uint32;

typedef struct Point_t {
	double x;
	double y;
	double z;
} Point;

typedef struct LinearRing_t {
	uint32    numPoints;
	Point    points[1]; // [numPoints]
} LinearRing; 

enum wkbGeometryType {
	wkbPoint = 1,
	wkbLineString = 2,
	wkbPolygon = 3,
	wkbMultiPoint = 4,
	wkbMultiLineString = 5,
	wkbMultiPolygon = 6,
	wkbGeometryCollection = 7
};

enum wkbByteOrder {
	wkbXDR = 0,    // Big Endian
	wkbNDR = 1    // Little Endian
};

typedef struct WKBPoint_t {
	byte    byteOrder;
	uint32    wkbType;     // 1
	Point    point;
} WKBPoint;

typedef struct WKBLineString_t {
	byte    byteOrder;
	uint32    wkbType;    // 2
	uint32    numPoints;
	Point    points[1]; // [numPoints]
} WKBLineString;

typedef struct WKBPolygon_t {
	byte    byteOrder;
	uint32    wkbType;    // 3
	uint32    numRings;
	LinearRing    rings[1]; // [numRings]
} WKBPolygon;

typedef struct WKBMultiPoint_t {
	byte    byteOrder;
	uint32    wkbType;       // 4
	uint32    num_wkbPoints;
	WKBPoint    WKBPoints[1]; // [num_wkbPoints];
} WKBMultiPoint;

typedef struct WKBMultiLineString_t {
	byte    byteOrder;
	uint32    wkbType;    // 5
	uint32    num_wkbLineStrings;
	WKBLineString    WKBLineStrings[1]; // [num_wkbLineStrings];
} WKBMultiLineString;

typedef struct WKBMultiPolygon_t {
	byte    byteOrder;
	uint32    wkbType;    // 6
	uint32    num_wkbPolygons;
	WKBPolygon    wkbPolygons[1]; // [num_wkbPolygons];
} WKBMultiPolygon;

typedef struct WKBGeometryCollection_t {
	byte    byte_order;
	uint32    wkbType;    // 7
	uint32    num_wkbGeometries;
	char    wkbGeometries[1]; // WKBGeometry [num_wkbGeometries];
} WKBGeometryCollection;

typedef struct WKBGeometry_t {
	union {
		WKBPoint    point;
		WKBLineString    linestring;
		WKBPolygon    polygon;
		WKBGeometryCollection    collection;
		WKBMultiPoint    mpoint;
		WKBMultiLineString    mlinestring;
		WKBMultiPolygon    mpolygon;
	};
} WKBGeometry;

