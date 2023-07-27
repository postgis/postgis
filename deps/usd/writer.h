#pragma once

#include <pxr/usd/usd/stage.h>

namespace USD {

class Writer {
	private:
		pxr::UsdStageRefPtr m_stage;

	public:
		Writer();
		~Writer();
};

}; // namespace USD
