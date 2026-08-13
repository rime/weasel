#pragma once
#include <WeaselIPCData.h>
#include <functional>
#include <memory>
#include <string>
#include <windows.h>
#include <wrl.h>

namespace weasel {
using namespace Microsoft::WRL;

template <typename T>
using an = std::shared_ptr<T>;
template <typename T>
using the = std::unique_ptr<T>;
typedef std::function<
    void(size_t* const, size_t* const, bool* const, bool* const)>
    UICallbackFunc;

enum ClientCapabilities {
  INLINE_PREEDIT_CAPABLE = 1,
};

class UIImpl;
class UI {
 public:
  UI();
  virtual ~UI();
  // 创建输入法界面
  bool Create(HWND parent, bool preview_mode = false);
  // 销毁界面
  void Destroy(bool full = false);
  // 界面显隐
  void Show();
  void Hide();
  void ShowWithTimeout(size_t millisec);
  BOOL IsCountingDown() const;
  BOOL IsShown() const;
  // 重绘界面
  void Refresh();
  // Reattach an undragged preview window to its parent dialog.
  void RepositionPreview();
  // 置输入焦点位置（光标跟随时移动候选窗）但不重绘
  void UpdateInputPosition(RECT const& rc);
  // 更新界面显示内容
  void Update(Context const& ctx, Status const& status);
  Context& ctx() { return ctx_; }
  Context& octx() { return octx_; }
  Status& status() { return status_; }
  UIStyle& style() { return style_; }
  UIStyle& ostyle() { return ostyle_; }
  bool GetIsReposition();
  UICallbackFunc& uiCallback() { return _uiCallback; }
  // compatibility shim for existing call sites
  void SetUICallBack(const UICallbackFunc& func) { _uiCallback = func; }
  void SetCallback(const UICallbackFunc& func) { _uiCallback = func; }
  bool& InServer() { return in_server_; }
  HWND hwnd();

 private:
  the<UIImpl> pimpl_;
  Context ctx_;
  Context octx_;
  Status status_;
  UIStyle style_;
  UIStyle ostyle_;
  bool in_server_;
  UICallbackFunc _uiCallback;
};
}  // namespace weasel
