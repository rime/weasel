#pragma once

#include "StandardLayout.h"

namespace weasel
{
	class VerticalLayout: public StandardLayout
	{
	public:
		VerticalLayout(const UIStyle &style, const Context &context, const Status &status);

		virtual void DoLayout(CDCHandle dc, GDIFonts* pFonts = 0);
		virtual void DoLayout(CDCHandle dc, DirectWriteResources* pDWR);
	};
};
