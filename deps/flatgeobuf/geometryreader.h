#ifndef FLATGEOBUF_GEOMETRYREADER_H
#define FLATGEOBUF_GEOMETRYREADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwgeom_log.h"
#include "liblwgeom.h"

#ifdef __cplusplus
}
#endif

#include "feature_generated.h"

namespace FlatGeobuf {

class GeometryReader {
    private:
        const FlatGeobuf::Geometry *m_geometry;
        FlatGeobuf::GeometryType m_geometry_type;
        bool m_has_z;
        bool m_has_m;

        uint32_t m_length = 0;
        uint32_t m_offset = 0;

        LWPOINT *readPoint();
        LWMPOINT *readMultiPoint();
        LWLINE *readLineString();
        LWMLINE *readMultiLineString();
        LWPOLY *readPolygon();
        LWMPOLY *readMultiPolygon();
        LWCOLLECTION *readGeometryCollection();

        POINTARRAY *readPA();

    public:
        GeometryReader(
            const FlatGeobuf::Geometry *geometry,
            FlatGeobuf::GeometryType geometry_type,
            bool has_z,
            bool has_m) :
            m_geometry (geometry),
            m_geometry_type (geometry_type),
            m_has_z (has_z),
            m_has_m (has_m)
            { }
        LWGEOM *read();
};

}


#endif /* FLATGEOBUF_GEOMETRYREADER_H */