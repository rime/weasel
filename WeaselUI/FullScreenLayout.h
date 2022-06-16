#pragma once

#include "StandardLayout.h"

namespace weasel
{
	class FullScreenLayout: public StandardLayout
	{
	public:
		FullScreenLayout(const UIStyle &style, const Context &context, const Status &status, const CRect& inputPos, Layout* layout);
		virtual ~FullScreenLayout();

		virtual void DoLayout(CDCHandle dc);
		virtual void DoLayout(CDCHandle dc, IDWriteTextFormat* pTextFormat, IDWriteFactory* pDWFactory);

	private:
		bool AdjustFontPoint(CDCHandle dc, const CRect& workArea, int& fontPoint, int& step);

		const CRect& mr_inputPos;
		Layout* m_layout;
	};
};
