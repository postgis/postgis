#ifndef USD_WRITER_H
#define USD_WRITER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwgeom_log.h"
#include "liblwgeom.h"

#ifdef __cplusplus
}
#endif

#include <pxr/usd/usd/stage.h>

namespace USD {

class Writer {
	private:
		void WritePoint(const LWPOINT *p);
		void WriteMultiPoint(const LWMPOINT *mp);
		void WriteLineString(const LWLINE* l);
		void WriteMultiLineString(const LWMLINE* ml);
		void WritePolygon(const LWPOLY *p);
		void WriteMultiPolygon(const LWMPOLY *mp);
		void WriteTriangle(const LWTRIANGLE *t);
		void WriteCollection(const LWCOLLECTION *coll);

	public:
		Writer();
		~Writer();

		void Write(LWGEOM *geom);

		pxr::UsdStageRefPtr m_stage;
};

}; // namespace USD

#endif
