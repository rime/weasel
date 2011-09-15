#pragma once

#include <WeaselCommon.h>

namespace weasel
{

	struct UIStyle
	{
		std::wstring fontFace;
		int fontPoint;
		UIStyle() : fontFace(), fontPoint(0) {}
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
		
		// 设置界面样式
		void SetStyle(UIStyle const& style);

		// 置输入焦点位置（光标跟随时移动候选窗）但不重绘
		void UpdateInputPosition(RECT const& rc);

		// 重绘
		void Refresh();

		// 更新界面显示内容
		void UpdateContext(weasel::Context const& ctx);

		// 更新输入法状态
		void UpdateStatus(weasel::Status const& status);

	private:
		static UIStyle const GetUIStyleSettings();

		UIImpl* pimpl_;
	};

}