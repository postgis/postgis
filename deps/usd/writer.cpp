#include "writer.h"

using namespace USD;

void
Writer::WritePoint(const LWPOINT *p)
{}

void
Writer::WriteCollection(const LWCOLLECTION *coll)
{}

Writer::Writer()
{
	m_stage = pxr::UsdStage::CreateInMemory();
}

Writer::~Writer() {}

void
Writer::Write(LWGEOM *geom)
{
	try
	{
		int type = geom->type;
		switch (type)
		{
		case POINTTYPE:
			WritePoint((LWPOINT *)geom);
			break;
		case COLLECTIONTYPE:
			WriteCollection((LWCOLLECTION *)geom);
			break;
		default:
			lwerror("usd: geometry type '%s' not supported", lwtype_name(type));
			break;
		}
	}
	catch (const std::exception &e)
	{
		lwerror("usd: caught standard c++ exception '%s'", e.what());
	}
	catch (...)
	{
		lwerror("usd: caught unknown c++ exception");
	}
}
