#ifndef USD_WRITER_H
#define USD_WRITER_H

/* do not forget that lwgeom is C library */
extern "C" {
#include "lwgeom_log.h"
#include "liblwgeom.h"
}

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
