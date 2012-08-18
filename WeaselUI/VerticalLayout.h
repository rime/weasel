#pragma once

#include "StandardLayout.h"

namespace weasel
{
	class VerticalLayout: public StandardLayout
	{
	public:
		VerticalLayout(const UIStyle &style, const Context &context);

		virtual void DoLayout(CDCHandle dc);
	};
};