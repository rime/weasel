#pragma once

#include <WeaselCommon.h>

namespace weasel
{

	enum ClientCapabilities
	{
		INLINE_PREEDIT_CAPABLE = 1,
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
		bool IsShown() const;
		
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