#pragma once

#include "StandardLayout.h"

namespace weasel
{
	class HorizontalLayout: public StandardLayout
	{
	public:
		HorizontalLayout(const UIStyle &style, const Context &context, const Status &status, const int dpi = 96)
			: StandardLayout(style, context, status, dpi) {}
		virtual void DoLayout(CDCHandle dc, DirectWriteResources* pDWR = NULL);
	};
};
