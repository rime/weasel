#pragma once

#include <WeaselCommon.h>
#include <vector>
#include <regex>
#include <iterator>
#include <d2d1.h>
#include <dwrite_2.h>
#include <memory>
#include <wrl/client.h>
#include <functional>
using namespace Microsoft::WRL;
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
			{ 
				pDWR.reset();
			}
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
		bool GetIsReposition(); 

		std::function<void(size_t* const, size_t* const, bool* const, bool* const)>& uiCallback() { return _UICallback; }
		void SetUICallBack(std::function<void(size_t* const, size_t* const, bool* const, bool* const)> const & func) { _UICallback = func; }

	private:
		UIImpl* pimpl_;
		PDWR pDWR;

		Context ctx_;
		Context octx_;
		Status status_;
		UIStyle style_;
		UIStyle ostyle_;
		std::function<void(size_t* const, size_t* const, bool* const, bool* const)> _UICallback;
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

		HRESULT CreateTextLayout(std::wstring text, int nCount, IDWriteTextFormat1* txtFormat, float width, float height) {
			return pDWFactory->CreateTextLayout(text.c_str(), nCount, txtFormat, width, height, reinterpret_cast<IDWriteTextLayout**>(pTextLayout.GetAddressOf()));
		}
		void DrawRect(D2D1_RECT_F* rect,float strokeWidth=1.0f, ID2D1StrokeStyle* sstyle=(ID2D1StrokeStyle*)0) {
			pRenderTarget->DrawRectangle(rect, pBrush.Get(), strokeWidth, sstyle);
		}
		HRESULT GetLayoutOverhangMetrics(DWRITE_OVERHANG_METRICS* overhangMetrics) {
			return pTextLayout->GetOverhangMetrics(overhangMetrics);
		}
		HRESULT GetLayoutMetrics(DWRITE_TEXT_METRICS* metrics) {
			return pTextLayout->GetMetrics(metrics);
		}
		HRESULT SetLayoutReadingDirection(DWRITE_READING_DIRECTION direct) {
			return pTextLayout->SetReadingDirection(direct);
		}
		HRESULT SetLayoutFlowDirection(DWRITE_FLOW_DIRECTION direct) {
			return pTextLayout->SetFlowDirection(direct);
		}
		void DrawTextLayoutAt(D2D1_POINT_2F point) {
			pRenderTarget->DrawTextLayout(point, pTextLayout.Get(), pBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
		}
		HRESULT CreateBrush(D2D1_COLOR_F color) {
			return pRenderTarget->CreateSolidColorBrush(color, pBrush.GetAddressOf());
		}
		void ResetLayout() { pTextLayout.Reset(); }
		void SetBrushColor(D2D1_COLOR_F color) {
			pBrush->SetColor(color);
		}
		void SetDpi(UINT dpi);

		float dpiScaleX_, dpiScaleY_;
		ComPtr<ID2D1Factory> pD2d1Factory;
		ComPtr<IDWriteFactory2> pDWFactory;
		ComPtr<ID2D1DCRenderTarget> pRenderTarget;
		ComPtr<IDWriteTextFormat1> pPreeditTextFormat;
		ComPtr<IDWriteTextFormat1> pTextFormat;
		ComPtr<IDWriteTextFormat1> pLabelTextFormat;
		ComPtr<IDWriteTextFormat1> pCommentTextFormat;
		ComPtr<IDWriteTextLayout2> pTextLayout;
		ComPtr<ID2D1SolidColorBrush> pBrush;
	private:
		UIStyle& _style;
		void _ParseFontFace(const std::wstring fontFaceStr, DWRITE_FONT_WEIGHT& fontWeight, DWRITE_FONT_STYLE& fontStyle);
		void _SetFontFallback(ComPtr<IDWriteTextFormat1> pTextFormat, std::vector<std::wstring> fontVector);
	};
}
