#pragma once

#include "StandardLayout.h"

namespace weasel
{
	class VHorizontalLayout: public StandardLayout
	{
	public:
		VHorizontalLayout(const UIStyle &style, const Context &context, const Status &status, const int dpi = 96)
			: StandardLayout(style, context, status, dpi){}
		virtual void DoLayout(CDCHandle dc, DirectWriteResources* pDWR = NULL);
	private:
		void DoLayoutWithWrap(CDCHandle dc, DirectWriteResources* pDWR = NULL);
	};
};
