﻿#pragma once

#include <WeaselCommon.h>

namespace weasel
{
	enum LayoutType
	{
		LAYOUT_VERTICAL = 0,
		LAYOUT_HORIZONTAL,
		LAYOUT_VERTICAL_FULLSCREEN,
		LAYOUT_HORIZONTAL_FULLSCREEN,
		LAYOUT_TYPE_LAST
	};

	enum ClientCapabilities
	{
		INLINE_PREEDIT_CAPABLE = 1,
	};

	enum PreeditType
	{
		COMPOSITION,
		PREVIEW
	};

	struct UIStyle
	{
		std::wstring font_face;
		int font_point;
		bool inline_preedit;
		PreeditType preedit_type;
		bool display_tray_icon;
		// layout
		LayoutType layout_type;
		int min_width;
		int min_height;
		int border;
		int margin_x;
		int margin_y;
		int spacing;
		int candidate_spacing;
		int hilite_spacing;
		int hilite_padding;
		int round_corner;
		// color scheme
		int text_color;
		int candidate_text_color;
		int label_text_color;
		int comment_text_color;
		int back_color;
		int border_color;
		int hilited_text_color;
		int hilited_back_color;
		int hilited_candidate_text_color;
		int hilited_candidate_back_color;
		int hilited_label_text_color;
		int hilited_comment_text_color;
		// per client
		int client_caps;

		UIStyle() : font_face(), 
			        font_point(0),
					inline_preedit(false),
					preedit_type(COMPOSITION),
					display_tray_icon(false),
					layout_type(LAYOUT_VERTICAL),
					min_width(0),
					min_height(0),
					border(0),
					margin_x(0),
					margin_y(0),
					spacing(0),
					candidate_spacing(0),
					hilite_spacing(0),
					hilite_padding(0),
					round_corner(0),
					text_color(0),
					candidate_text_color(0),
					label_text_color(0),
					comment_text_color(0),
					back_color(0),
					border_color(0),
					hilited_text_color(0),
					hilited_back_color(0),
					hilited_candidate_text_color(0),
					hilited_candidate_back_color(0),
					hilited_label_text_color(0),
					hilited_comment_text_color(0),
					client_caps(0) {}
	};

	class UIImpl;

	//
	// 输入法界面接口类
	//
	class UI
	{
	public:
		UI() : pimpl_(0)
		{
		}

		virtual ~UI()
		{
			if (pimpl_)
				Destroy();
		}

		// 创建输入法界面
		bool Create(HWND parent);

		// 销毁界面
		void Destroy();
		
		// 界面显隐
		void Show();
		void Hide();
		void ShowWithTimeout(DWORD millisec);
		bool IsCountingDown() const;
		
		// 重绘界面
		void Refresh();

		// 置输入焦点位置（光标跟随时移动候选窗）但不重绘
		void UpdateInputPosition(RECT const& rc);

		// 更新界面显示内容
		void Update(Context const& ctx, Status const& status);

		Context& ctx() { return ctx_; } 
		Status& status() { return status_; } 
		UIStyle& style() { return style_; }

	private:
		UIImpl* pimpl_;

		Context ctx_;
		Status status_;
		UIStyle style_;
	};

}