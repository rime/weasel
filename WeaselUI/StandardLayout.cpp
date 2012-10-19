#include "stdafx.h"
#include "StandardLayout.h"

using namespace weasel;

static WCHAR LABEL_PATTERN[] = L"%1%.";

StandardLayout::StandardLayout(const UIStyle &style, const Context &context)
	: Layout(style, context)
{
}

std::wstring StandardLayout::GetLabelText(const std::string &label, int id) const
{
	if (id < label.length())
		return (boost::wformat(LABEL_PATTERN) % label.at(id)).str();
	else
		return (boost::wformat(LABEL_PATTERN) % ((id + 1) % 10)).str();
}

CSize StandardLayout::GetPreeditSize(CDCHandle dc) const
{
	const wstring &preedit = _context.preedit.str;
	const vector<weasel::TextAttribute> &attrs = _context.preedit.attributes;
	CSize size(0, 0);
	if (!preedit.empty())
	{
		dc.GetTextExtent(preedit.c_str(), preedit.length(), &size);
		for (int i = 0; i < attrs.size(); i++)
		{
			if (attrs[i].type == weasel::HIGHLIGHTED)
			{
				const weasel::TextRange &range = attrs[i].range;
				if (range.start < range.end)
				{
					if (range.start > 0)
						size.cx += _style.hilite_spacing;
					else
						size.cx += _style.hilite_padding;
					if (range.end < static_cast<int>(preedit.length()))
						size.cx += _style.hilite_spacing;
					else
						size.cx += _style.hilite_padding;
				}
			}
		}
	}
	return size;
}
