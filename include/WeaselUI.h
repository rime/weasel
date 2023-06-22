#pragma once

#include <WeaselCommon.h>
#include <vector>
#include <regex>
#include <iterator>
#include <d2d1.h>
#include <dwrite_2.h>
#include <memory>

namespace weasel
{

	template <class T> void SafeRelease(T** ppT)
	{
		if (*ppT)
		{
			(*ppT)->Release();
			*ppT = NULL;
		}
	}

	enum ClientCapabilities
	{
		INLINE_PREEDIT_CAPABLE = 1,
	};

	class UIImpl;
	class DirectWriteResources;
	template <class T>
	using an = std::shared_ptr<T>;

	using PDWR = an<DirectWriteResources>;
	//
	// 输入法界面接口类
	//
	class UI
	{
	public:
		UI() : pimpl_(0)
		{ }

		virtual ~UI()
		{
			if (pimpl_)
				Destroy(true);
			if (pDWR)
				pDWR.reset();
		}

		// 创建输入法界面
		bool Create(HWND parent);

		// 销毁界面
		void Destroy(bool full = false);
		
		// 界面显隐
		void Show();
		void Hide();
		void ShowWithTimeout(DWORD millisec);
		bool IsCountingDown() const;
		bool IsShown() const;
		
		// 重绘界面
		void Refresh();

		// 置输入焦点位置（光标跟随时移动候选窗）但不重绘
		void UpdateInputPosition(RECT const& rc);

		// 更新界面显示内容
		void Update(Context const& ctx, Status const& status);

		Context& ctx() { return ctx_; } 
		Context& octx() { return octx_; } 
		Status& status() { return status_; } 
		UIStyle& style() { return style_; }
		UIStyle& ostyle() { return ostyle_; }
		PDWR pdwr() { return pDWR; }

	private:
		UIImpl* pimpl_;
		PDWR pDWR;

		Context ctx_;
		Context octx_;
		Status status_;
		UIStyle style_;
		UIStyle ostyle_;
	};

	class DirectWriteResources
	{
	public:
		DirectWriteResources(weasel::UIStyle& style, UINT dpi);
		~DirectWriteResources();

		HRESULT InitResources(std::wstring label_font_face, int label_font_point,
			std::wstring font_face, int font_point,
			std::wstring comment_font_face, int comment_font_point, bool vertical_text = false);
		HRESULT InitResources(UIStyle& style, UINT dpi);
		void SetDpi(UINT dpi);
		float dpiScaleX_, dpiScaleY_;
		ID2D1Factory* pD2d1Factory;
		IDWriteFactory2* pDWFactory;
		ID2D1DCRenderTarget* pRenderTarget;
		IDWriteTextFormat1* pPreeditTextFormat;
		IDWriteTextFormat1* pTextFormat;
		IDWriteTextFormat1* pLabelTextFormat;
		IDWriteTextFormat1* pCommentTextFormat;
		IDWriteTextLayout2* pTextLayout;
		ID2D1SolidColorBrush* pBrush;
	private:
		UIStyle& _style;
		void _ParseFontFace(const std::wstring fontFaceStr, DWRITE_FONT_WEIGHT& fontWeight, DWRITE_FONT_STYLE& fontStyle);
		void _SetFontFallback(IDWriteTextFormat1* pTextFormat, std::vector<std::wstring> fontVector);
	};
}
