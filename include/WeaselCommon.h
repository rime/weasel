﻿#pragma once

//#include <string>
//#include <vector>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#define WEASEL_IME_NAME L"小狼毫"
#define WEASEL_REG_KEY L"Software\\Rime\\Weasel"
#define RIME_REG_KEY L"Software\\Rime"

#define USE_BLUR_UNDER_WINDOWS10
#define USE_HILITE_MARK
#define USE_CANDIDATE_BORDER
#define USE_MOUSE_EVENTS
//#define USE_THEME_DARK
//#define CLIP_SHADOW_UNDER_BACK_COLOR
//#define USE_MOUSE_HOVER
//#define USE_SHARP_COLOR_CODE
#define USE_PAGER_MARK

//#define _DEBUG_
namespace weasel
{

	enum TextAttributeType
	{
		NONE = 0,
		HIGHLIGHTED,
		LAST_TYPE
	};

	struct TextRange
	{
		TextRange() : start(0), end(0) {}
		TextRange(int _start, int _end) : start(_start), end(_end) {}
		bool operator==(const TextRange& tr)
		{
			return (start == tr.start && end == tr.end);
		}
		bool operator!=(const TextRange& tr)
		{
			return (start != tr.start || end != tr.end);
		}
		int start;
		int end;
	};

	struct TextAttribute
	{
		TextAttribute() : type(NONE) {}
		TextAttribute(int _start, int _end, TextAttributeType _type) : range(_start, _end), type(_type) {}
		bool operator==(const TextAttribute& ta)
		{
			return (range == ta.range && type == ta.type);
		}
		bool operator!=(const TextAttribute& ta)
		{
			return (range != ta.range || type != ta.type);
		}
		TextRange range;
		TextAttributeType type;
	};

	struct Text
	{
		Text() : str(L"") {}
		Text(std::wstring const& _str) : str(_str) {}
		void clear()
		{
			str.clear();
			attributes.clear();
		}
		bool empty() const
		{
			return str.empty();
		}
		bool operator==(const Text& txt)
		{
			if (str != txt.str || (attributes.size() != txt.attributes.size()))
				return false;
			for (size_t i = 0; i < attributes.size(); i++)
			{
				if ((attributes[i] != txt.attributes[i]))
					return false;
			}
			return true;
		}
		bool operator!=(const Text& txt)
		{
			if (str != txt.str || (attributes.size() != txt.attributes.size()))
				return true;
			for (size_t i = 0; i < attributes.size(); i++)
			{
				if ((attributes[i] != txt.attributes[i]))
					return true;
			}
			return false;
		}
		std::wstring str;
		std::vector<TextAttribute> attributes;
	};

	struct CandidateInfo
	{
		CandidateInfo()
		{
			currentPage = 0;
			totalPages = 0;
			highlighted = 0;
			is_last_page = false;
		}
		void clear()
		{
			currentPage = 0;
			totalPages = 0;
			highlighted = 0;
			is_last_page = false;
			candies.clear();
			labels.clear();
		}
		bool empty() const
		{
			return candies.empty();
		}
		bool operator==(const CandidateInfo& ci)
		{
			if (currentPage != ci.currentPage
				|| totalPages != ci.totalPages
				|| highlighted != ci.highlighted
				|| is_last_page != ci.is_last_page
				|| notequal(candies, ci.candies)
				|| notequal(comments, ci.comments)
				|| notequal(labels, ci.labels)
				)
				return false;
			return true;
		}
		bool operator!=(const CandidateInfo& ci)
		{
			if (currentPage != ci.currentPage
				|| totalPages != ci.totalPages
				|| highlighted != ci.highlighted
				|| is_last_page != ci.is_last_page
				|| notequal(candies, ci.candies)
				|| notequal(comments, ci.comments)
				|| notequal(labels, ci.labels)
				)
				return true;
			return false;
		}
		bool notequal(std::vector<Text> txtSrc, std::vector<Text> txtDst)
		{
			if (txtSrc.size() != txtDst.size()) return true;
			for (size_t i = 0; i < txtSrc.size(); i++)
			{
				if (txtSrc[i] != txtDst[i])
					return true;
			}
			return false;
		}
		int currentPage;
		bool is_last_page;
		int totalPages;
		int highlighted;
		std::vector<Text> candies;
		std::vector<Text> comments;
		std::vector<Text> labels;
	};

	struct Context
	{
		Context() {}
		void clear()
		{
			preedit.clear();
			aux.clear();
			cinfo.clear();
		}
		bool empty() const
		{
			return preedit.empty() && aux.empty() && cinfo.empty();
		}
		bool operator==(const Context& ctx)
		{
			if (preedit == ctx.preedit && aux == ctx.aux || cinfo == ctx.cinfo)
				return true;
			return false;
		}
		bool operator!=(const Context& ctx)
		{
			return !(operator==(ctx));
		}

		bool operator!()
		{
			if (preedit.str.empty()
				&& aux.str.empty()
				&& cinfo.candies.empty()
				&& cinfo.labels.empty()
				&& cinfo.comments.empty())
				return true;
			else
				return false;
		}
		Text preedit;
		Text aux;
		CandidateInfo cinfo;
	};

	// 由ime管理
	struct Status
	{
		Status() : ascii_mode(false), composing(false), disabled(false), full_shape(false) {}
		void reset()
		{
			schema_name.clear();
			ascii_mode = false;
			composing = false;
			disabled = false;
			full_shape = false;
		}
		// 輸入方案
		std::wstring schema_name;
		// 轉換開關
		bool ascii_mode;
		// 寫作狀態
		bool composing;
		// 維護模式（暫停輸入功能）
		bool disabled;
		// 全角状态
		bool full_shape;
	};

	// 用於向前端告知設置信息
	struct Config
	{
		Config() : inline_preedit(false) {}
		void reset()
		{
			inline_preedit = false;
		}
		bool inline_preedit;
	};

	struct UIStyle
	{
		enum PreeditType
		{
			COMPOSITION,
			PREVIEW,
			PREVIEW_ALL
		};

		enum LayoutType
		{
			LAYOUT_VERTICAL = 0,
			LAYOUT_HORIZONTAL,
			LAYOUT_VERTICAL_TEXT,
			LAYOUT_VERTICAL_FULLSCREEN,
			LAYOUT_HORIZONTAL_FULLSCREEN,
			LAYOUT_TYPE_LAST
		};

		enum LayoutAlignType
		{
			ALIGN_BOTTOM = 0,
			ALIGN_CENTER,
			ALIGN_TOP
		};

		LayoutAlignType align_type;
		PreeditType preedit_type;
		LayoutType layout_type;
		bool vertical_text_left_to_right;
		bool vertical_text_with_wrap;
		std::wstring font_face;
		std::wstring label_font_face;
		std::wstring comment_font_face;
		int font_point;
		int label_font_point;
		int comment_font_point;
		bool inline_preedit;
#ifdef USE_BLUR_UNDER_WINDOWS10
		bool blur_window;
#endif
		bool display_tray_icon;
		std::wstring current_zhung_icon;

		std::wstring label_text_format;
#ifdef USE_HILITE_MARK
		std::wstring mark_text;
#endif
		// layout
		int min_width;
		int max_width;
		int min_height;
		int max_height;
		int border;
		int margin_x;
		int margin_y;
		int spacing;
		int candidate_spacing;
		int hilite_spacing;
		int hilite_padding;
		int round_corner;
		int round_corner_ex;
		int shadow_radius;
		int shadow_offset_x;
		int shadow_offset_y;
		bool vertical_auto_reverse;
		// color scheme
		int text_color;
		int candidate_text_color;
		int candidate_back_color;
		int candidate_shadow_color;
#ifdef USE_CANDIDATE_BORDER
		int candidate_border_color;
#endif
		int label_text_color;
		int comment_text_color;
		int back_color;
		int shadow_color;
		int border_color;
		int hilited_text_color;
		int hilited_back_color;
		int hilited_shadow_color;
		int hilited_candidate_text_color;
		int hilited_candidate_back_color;
		int hilited_candidate_shadow_color;
#ifdef USE_CANDIDATE_BORDER
		int hilited_candidate_border_color;
#endif
		int hilited_label_text_color;
		int hilited_comment_text_color;
#ifdef USE_HILITE_MARK
		int hilited_mark_color;
#endif
#ifdef USE_PAGER_MARK
		int prevpage_color;
		int nextpage_color;
#endif /*  USE_PAGER_MARK */
		// per client
		int client_caps;

		UIStyle() : font_face(),
			label_font_face(),
			comment_font_face(),
			font_point(0),
			label_font_point(0),
			comment_font_point(0),
			inline_preedit(false),
			align_type(ALIGN_BOTTOM),
			preedit_type(COMPOSITION),
#ifdef USE_BLUR_UNDER_WINDOWS10
			blur_window(false),
#endif
			display_tray_icon(false),
			current_zhung_icon(),

			label_text_format(L"%s."),
#ifdef USE_HILITE_MARK
			mark_text(),
#endif
			layout_type(LAYOUT_VERTICAL),
			vertical_text_left_to_right(false),
			vertical_text_with_wrap(false),
			min_width(0),
			max_width(0),
			min_height(0),
			max_height(0),
			border(0),
			margin_x(0),
			margin_y(0),
			spacing(0),
			candidate_spacing(0),
			hilite_spacing(0),
			hilite_padding(0),
			round_corner(0),
			round_corner_ex(0),
			shadow_radius(0),
			shadow_offset_x(0),
			shadow_offset_y(0),
			vertical_auto_reverse(false),
			text_color(0),
			candidate_text_color(0),
			candidate_back_color(0),
			candidate_shadow_color(0),
#ifdef USE_CANDIDATE_BORDER
			candidate_border_color(0),
#endif
			label_text_color(0),
			comment_text_color(0),
			back_color(0),
			shadow_color(0),
			border_color(0),
			hilited_text_color(0),
			hilited_back_color(0),
			hilited_shadow_color(0),
			hilited_candidate_text_color(0),
			hilited_candidate_back_color(0),
			hilited_candidate_shadow_color(0),
#ifdef USE_CANDIDATE_BORDER
			hilited_candidate_border_color(0),
#endif
			hilited_label_text_color(0),
			hilited_comment_text_color(0),
#ifdef USE_HILITE_MARK
			hilited_mark_color(0),
#endif
#ifdef USE_PAGER_MARK
			prevpage_color(0),
			nextpage_color(0),
#endif
			client_caps(0) {}
		bool operator!=(const UIStyle& st)
		{
			return
				(
					align_type != st.align_type
					|| preedit_type != st.preedit_type
					|| layout_type != st.layout_type
					|| vertical_text_left_to_right != st.vertical_text_left_to_right
					|| vertical_text_with_wrap != st.vertical_text_with_wrap
					|| font_face != st.font_face
					|| label_font_face != st.label_font_face
					|| comment_font_face != st.comment_font_face
					|| font_point != st.font_point
					|| label_font_point != st.label_font_point
					|| comment_font_point != st.comment_font_point
					|| inline_preedit != st.inline_preedit
#ifdef USE_BLUR_UNDER_WINDOWS10
					|| blur_window != st.blur_window
#endif
#ifdef USE_HILITE_MARK
					|| mark_text != st.mark_text
#endif
					|| display_tray_icon != st.display_tray_icon
					|| current_zhung_icon != st.current_zhung_icon
					|| label_text_format != st.label_text_format
					|| min_width != st.min_width
					|| max_width != st.max_width
					|| min_height != st.min_height
					|| max_height != st.max_height
					|| border != st.border
					|| margin_x != st.margin_x
					|| margin_y != st.margin_y
					|| spacing != st.spacing
					|| candidate_spacing != st.candidate_spacing
					|| hilite_spacing != st.hilite_spacing
					|| hilite_padding != st.hilite_padding
					|| round_corner != st.round_corner
					|| round_corner_ex != st.round_corner_ex
					|| shadow_radius != st.shadow_radius
					|| shadow_offset_x != st.shadow_offset_x
					|| shadow_offset_y != st.shadow_offset_y
					|| vertical_auto_reverse != st.vertical_auto_reverse
					|| text_color != st.text_color
					|| candidate_text_color != st.candidate_text_color
					|| candidate_back_color != st.candidate_back_color
					|| candidate_shadow_color != st.candidate_shadow_color
#ifdef USE_CANDIDATE_BORDER
					|| candidate_border_color != st.candidate_border_color
					|| hilited_candidate_border_color != st.hilited_candidate_border_color
#endif
					|| label_text_color != st.label_text_color
					|| comment_text_color != st.comment_text_color
					|| back_color != st.back_color
					|| shadow_color != st.shadow_color
					|| border_color != st.border_color
					|| hilited_text_color != st.hilited_text_color
					|| hilited_back_color != st.hilited_back_color
					|| hilited_shadow_color != st.hilited_shadow_color
					|| hilited_candidate_text_color != st.hilited_candidate_text_color
					|| hilited_candidate_back_color != st.hilited_candidate_back_color
					|| hilited_candidate_shadow_color != st.hilited_candidate_shadow_color
					|| hilited_label_text_color != st.hilited_label_text_color
					|| hilited_comment_text_color != st.hilited_comment_text_color
#ifdef USE_HILITE_MARK
					|| hilited_mark_color != st.hilited_mark_color
#endif
#ifdef USE_PAGER_MARK
					|| prevpage_color != st.prevpage_color
					|| nextpage_color != st.nextpage_color
#endif /* USE_PAGER_MARK */
					);
		}
	};
}
namespace boost {
	namespace serialization {
		template <typename Archive>
		void serialize(Archive &ar, weasel::UIStyle &s, const unsigned int version)
		{
			ar & s.font_face;
			ar & s.label_font_face;
			ar & s.comment_font_face;
			ar & s.font_point;
			ar & s.label_font_point;
			ar & s.comment_font_point;
			ar & s.inline_preedit;
			ar & s.align_type;
#ifdef USE_BLUR_UNDER_WINDOWS10
			ar & s.blur_window;
#endif
#ifdef USE_HILITE_MARK
			ar & s.mark_text;
#endif
			ar & s.preedit_type;
			ar & s.display_tray_icon;
			ar & s.current_zhung_icon;
			ar & s.label_text_format;
			// layout
			ar & s.layout_type;
			ar & s.vertical_text_left_to_right;
			ar & s.vertical_text_with_wrap;
			ar & s.min_width;
			ar & s.max_width;
			ar & s.min_height;
			ar & s.max_height;
			ar & s.border;
			ar & s.margin_x;
			ar & s.margin_y;
			ar & s.spacing;
			ar & s.candidate_spacing;
			ar & s.hilite_spacing;
			ar & s.hilite_padding;
			ar & s.round_corner;
			ar & s.round_corner_ex;
			ar & s.shadow_radius;
			ar & s.shadow_offset_x;
			ar & s.shadow_offset_y;
			ar & s.vertical_auto_reverse;
			// color scheme
			ar & s.text_color;
			ar & s.candidate_text_color;
			ar & s.candidate_back_color;
			ar & s.candidate_shadow_color;
#ifdef USE_CANDIDATE_BORDER
			ar & s.candidate_border_color;
#endif
			ar & s.label_text_color;
			ar & s.comment_text_color;
			ar & s.back_color;
			ar & s.shadow_color;
			ar & s.border_color;
			ar & s.hilited_text_color;
			ar & s.hilited_back_color;
			ar & s.hilited_shadow_color;
			ar & s.hilited_candidate_text_color;
			ar & s.hilited_candidate_back_color;
			ar & s.hilited_candidate_shadow_color;
#ifdef USE_CANDIDATE_BORDER
			ar & s.hilited_candidate_border_color;
#endif
			ar & s.hilited_label_text_color;
			ar & s.hilited_comment_text_color;
#ifdef USE_HILITE_MARK
			ar & s.hilited_mark_color;
#endif
#ifdef USE_PAGER_MARK
			ar & s.prevpage_color;
			ar & s.nextpage_color;
#endif
			// per client
			ar & s.client_caps;
		}

		template <typename Archive>
		void serialize(Archive &ar, weasel::CandidateInfo &s, const unsigned int version)
		{
			ar & s.currentPage;
			ar & s.totalPages;
			ar & s.highlighted;
			ar & s.is_last_page;
			ar & s.candies;
			ar & s.comments;
			ar & s.labels;
		}
		template <typename Archive>
		void serialize(Archive &ar, weasel::Text &s, const unsigned int version)
		{
			ar & s.str;
			ar & s.attributes;
		}
		template <typename Archive>
		void serialize(Archive &ar, weasel::TextAttribute &s, const unsigned int version)
		{
			ar & s.range;
			ar & s.type;
		}
		template <typename Archive>
		void serialize(Archive &ar, weasel::TextRange &s, const unsigned int version)
		{
			ar & s.start;
			ar & s.end;
		}
	} // namespace serialization
} // namespace boost
