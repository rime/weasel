#pragma once

//#include <string>
//#include <vector>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#define WEASEL_IME_NAME L"小狼毫"
#define WEASEL_REG_KEY L"Software\\Rime\\Weasel"
#define RIME_REG_KEY L"Software\\Rime"

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
		int start;
		int end;
	};

	struct TextAttribute
	{
		TextAttribute() : type(NONE) {}
		TextAttribute(int _start, int _end, TextAttributeType _type) : range(_start, _end), type(_type) {}
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
		}
		void clear()
		{
			currentPage = 0;
			totalPages = 0;
			highlighted = 0;
			candies.clear();
			labels.clear();
		}
		bool empty() const
		{
			return candies.empty();
		}
		int currentPage;
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
		Text preedit;
		Text aux;
		CandidateInfo cinfo;
	};

	// 由ime管理
	struct Status
	{
		Status() : ascii_mode(false), composing(false), disabled(false) {}
		void reset()
		{
			schema_name.clear();
			ascii_mode = false;
			composing = false;
			disabled = false;
		}
		// 輸入方案
		std::wstring schema_name;
		// 轉換開關
		bool ascii_mode;
		// 寫作狀態
		bool composing;
		// 維護模式（暫停輸入功能）
		bool disabled;
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
			PREVIEW
		};
		enum LayoutType
		{
			LAYOUT_VERTICAL = 0,
			LAYOUT_HORIZONTAL,
			LAYOUT_VERTICAL_FULLSCREEN,
			LAYOUT_HORIZONTAL_FULLSCREEN,
			LAYOUT_TYPE_LAST
		};

		PreeditType preedit_type;
		LayoutType layout_type;
		std::wstring font_face;
		int font_point;
		bool inline_preedit;
		bool display_tray_icon;
		std::wstring label_text_format;
		// layout
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
			label_text_format(L"%s."),
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
}
namespace boost {
	namespace serialization {
		template <typename Archive>
		void serialize(Archive &ar, weasel::UIStyle &s, const unsigned int version)
		{
			ar & s.font_face;
			ar & s.font_point;
			ar & s.inline_preedit;
			ar & s.preedit_type;
			ar & s.display_tray_icon;
			ar & s.label_text_format;
			// layout
			ar & s.layout_type;
			ar & s.min_width;
			ar & s.min_height;
			ar & s.border;
			ar & s.margin_x;
			ar & s.margin_y;
			ar & s.spacing;
			ar & s.candidate_spacing;
			ar & s.hilite_spacing;
			ar & s.hilite_padding;
			ar & s.round_corner;
			// color scheme
			ar & s.text_color;
			ar & s.candidate_text_color;
			ar & s.label_text_color;
			ar & s.comment_text_color;
			ar & s.back_color;
			ar & s.border_color;
			ar & s.hilited_text_color;
			ar & s.hilited_back_color;
			ar & s.hilited_candidate_text_color;
			ar & s.hilited_candidate_back_color;
			ar & s.hilited_label_text_color;
			ar & s.hilited_comment_text_color;
			// per client
			ar & s.client_caps;
		}

		template <typename Archive>
		void serialize(Archive &ar, weasel::CandidateInfo &s, const unsigned int version)
		{
			ar & s.currentPage;
			ar & s.totalPages;
			ar & s.highlighted;
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