#pragma once

#include <WeaselCommon.h>
#include <WeaselUI.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")
#define IS_FULLSCREENLAYOUT(style)	(style.layout_type == UIStyle::LAYOUT_VERTICAL_FULLSCREEN || style.layout_type == UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)
#define NOT_FULLSCREENLAYOUT(style)	(style.layout_type != UIStyle::LAYOUT_VERTICAL_FULLSCREEN && style.layout_type != UIStyle::LAYOUT_HORIZONTAL_FULLSCREEN)

namespace weasel
{
	class GraphicsRoundRectPath : public Gdiplus::GraphicsPath
	{
	public:
		GraphicsRoundRectPath() {};
		GraphicsRoundRectPath(int left, int top, int width, int height, int cornerx, int cornery) : Gdiplus::GraphicsPath()
		{
			AddRoundRect(left, top, width, height, cornerx, cornery);
		}
		GraphicsRoundRectPath(const CRect rc, int corner)
		{
			if (corner > 0) AddRoundRect(rc.left, rc.top, rc.Width(), rc.Height(), corner, corner);
			else AddRectangle(Gdiplus::Rect(rc.left, rc.top, rc.Width(), rc.Height()));
		}

		GraphicsRoundRectPath(const CRect rc, int corner, bool roundTopLeft, bool roundTopRight, bool roundBottomRight, bool roundBottomLeft);

	public:
		void AddRoundRect(int left, int top, int width, int height, int cornerx, int cornery);
	};

	struct IsToRoundStruct
	{
		bool IsTopLeftNeedToRound;
		bool IsBottomLeftNeedToRound;
		bool IsTopRightNeedToRound;
		bool IsBottomRightNeedToRound;
		bool Hemispherical;
		IsToRoundStruct():
			IsTopLeftNeedToRound(true), 
			IsTopRightNeedToRound(true), 
			IsBottomLeftNeedToRound(true), 
			IsBottomRightNeedToRound(true),
			Hemispherical(false)
		{}
	};

	class Layout
	{
	public:
		Layout(const UIStyle &style, const Context &context, const Status &status);

		virtual void DoLayout(CDCHandle dc, PDWR pDWR = NULL) = 0;
		/* All points in this class is based on the content area */
		/* The top-left corner of the content area is always (0, 0) */
		virtual CSize GetContentSize() const = 0;
		virtual CRect GetPreeditRect() const = 0;
		virtual CRect GetAuxiliaryRect() const = 0;
		virtual CRect GetHighlightRect() const = 0;
		virtual CRect GetCandidateLabelRect(int id) const = 0;
		virtual CRect GetCandidateTextRect(int id) const = 0;
		virtual CRect GetCandidateRect(int id) const = 0;
		virtual CRect GetCandidateCommentRect(int id) const = 0;
		virtual CRect GetStatusIconRect() const = 0;
		virtual IsToRoundStruct GetRoundInfo(int id) = 0;
		virtual IsToRoundStruct GetTextRoundInfo() = 0;
		virtual CRect GetContentRect() = 0;
		virtual CRect GetPrepageRect() = 0;
		virtual CRect GetNextpageRect() = 0;

		virtual std::wstring GetLabelText(const std::vector<Text> &labels, int id, const wchar_t *format) const = 0;
		virtual bool IsInlinePreedit() const = 0;
		virtual bool ShouldDisplayStatusIcon() const = 0;
		virtual void GetTextSizeDW(const std::wstring text, size_t nCount, IDWriteTextFormat1* pTextFormat, PDWR pDWR,  LPSIZE lpSize) const = 0;
		
		int offsetX = 0;
		int offsetY = 0;
		int MARK_WIDTH = 4;
		int MARK_GAP = 8;
		int MARK_HEIGHT = 0;

	protected:
		const UIStyle &_style;
		const Context &_context;
		const Status &_status;
		const std::vector<Text> &candidates;
		const std::vector<Text> &comments;
		const std::vector<Text> &labels;
		const int &id;
		const int candidates_count;
		const int real_margin_x;
		const int real_margin_y;
		const int labelFontValid;
		const int textFontValid;
		const int cmtFontValid;
	};
};
