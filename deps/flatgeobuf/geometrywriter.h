#ifndef FLATGEOBUF_GEOMETRYWRITER_H
#define FLATGEOBUF_GEOMETRYWRITER_H

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

class GeometryWriter {
    private:
        flatbuffers::FlatBufferBuilder &m_fbb;
        const LWGEOM *m_lwgeom;
        FlatGeobuf::GeometryType m_geometry_type;
        const bool m_has_z;
        const bool m_has_m;
        std::vector<double> m_xy;
        std::vector<double> m_z;
        std::vector<double> m_m;

        void writePoint(const LWPOINT *p);
        void writeMultiPoint(const LWMPOINT *mp);
        void writeLineString(const LWLINE *l);
        void writeMultiLineString(const LWMLINE *ml);
        void writePolygon(const LWPOLY *p);
        const flatbuffers::Offset<FlatGeobuf::Geometry> writeMultiPolygon(const LWMPOLY *mp, int depth);
        const flatbuffers::Offset<FlatGeobuf::Geometry> writeGeometryCollection(const LWCOLLECTION *gc, int depth);

        void writePA(POINTARRAY *pa);
        void writePPA(POINTARRAY **ppa, uint32_t len);
        /*
        const flatbuffers::Offset<FlatGeobuf::Geometry> writeCompoundCurve(const OGRCompoundCurve *cc, int depth);
        const flatbuffers::Offset<FlatGeobuf::Geometry> writeCurvePolygon(const OGRCurvePolygon *cp, int depth);
        const flatbuffers::Offset<FlatGeobuf::Geometry> writePolyhedralSurface(const OGRPolyhedralSurface *p, int depth);
        void writeTIN(const OGRTriangulatedSurface *p);
        */

    public:
        GeometryWriter(
            flatbuffers::FlatBufferBuilder &fbb,
            const LWGEOM *lwgeom,
            const FlatGeobuf::GeometryType geometry_type,
            const bool has_z,
            const bool has_m) :
            m_fbb (fbb),
            m_lwgeom (lwgeom),
            m_geometry_type (geometry_type),
            m_has_z (has_z),
            m_has_m (has_m)
            { }
        std::vector<uint32_t> m_ends;
        const flatbuffers::Offset<FlatGeobuf::Geometry> write(int depth);
        static const FlatGeobuf::GeometryType get_geometrytype(const LWGEOM *lwgeom);
};

}


#endif /* FLATGEOBUF_GEOMETRYWRITER_H */