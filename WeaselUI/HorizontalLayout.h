#pragma once

#include "StandardLayout.h"

namespace weasel
{
	class HorizontalLayout: public StandardLayout
	{
	public:
		HorizontalLayout(const UIStyle &style, const Context &context);

		virtual void DoLayout(CDCHandle dc);
	};
};