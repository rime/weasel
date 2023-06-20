#pragma once

#include "StandardLayout.h"

namespace weasel
{
	class HorizontalLayout: public StandardLayout
	{
	public:
		HorizontalLayout(const UIStyle &style, const Context &context, const Status &status)
			: StandardLayout(style, context, status) {}
		virtual void DoLayout(CDCHandle dc, PDWR pDWR = NULL);
	};
};
