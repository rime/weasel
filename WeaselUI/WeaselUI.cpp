#include "WeaselPanel.h"
#include <WeaselUI.h>

namespace weasel {
// ----------------------------------------------------------------------------
class UIImpl {
 public:
  WeaselPanel panel;
  UIImpl(UI& ui) : panel(ui) {}
  ~UIImpl() {}
  bool IsShown() const {
    return panel.IsWindow() && ::IsWindowVisible(panel.hwnd());
  }
  void Refresh() {
    if (!panel.IsWindow())
      return;
    panel.Refresh();
  }
  void Show() {
    if (!panel.IsWindow())
      return;
    panel.ShowWindow(SW_SHOWNA);
  }
  void Hide() {
    if (!panel.IsWindow())
      return;
    panel.ShowWindow(SW_HIDE);
  }
  void ShowWithTimeout(size_t millisec) { panel.ShowWithTimeout(millisec); }
  bool IsCountingDown() const { return panel.IsCountingDown(); }
};
// ----------------------------------------------------------------------------
BOOL UI::IsCountingDown() const {
  return pimpl_ && pimpl_->panel.IsCountingDown();
};

UI::UI() : pimpl_(nullptr), in_server_(false) {}

UI::~UI() {
  if (pimpl_)
    Destroy(true);
}
BOOL UI::IsShown() const {
  return pimpl_ && pimpl_->IsShown();
}
void UI::UpdateInputPosition(RECT const& rc) {
  if (pimpl_ && pimpl_->panel.IsWindow()) {
    pimpl_->panel.MoveTo(rc);
  }
}
void UI::Update(const Context& ctx, const Status& status) {
  if (ctx_ == ctx && status_ == status)
    return;
  ctx_ = ctx;
  status_ = status;
  if (style_.candidate_abbreviate_length > 0) {
    for (auto& c : ctx_.cinfo.candies) {
      if (c.str.length() > (size_t)style_.candidate_abbreviate_length) {
        c.str =
            c.str.substr(0, (size_t)style_.candidate_abbreviate_length - 1) +
            L"..." + c.str.substr(c.str.length() - 1);
      }
    }
  }
  Refresh();
}
void UI::Refresh() {
  if (pimpl_) {
    pimpl_->Refresh();
  }
}
void UI::ShowWithTimeout(size_t millisec) {
  if (pimpl_) {
    pimpl_->ShowWithTimeout(millisec);
  }
}
void UI::Show() {
  if (pimpl_) {
    pimpl_->Show();
  }
}
void UI::Hide() {
  if (pimpl_) {
    pimpl_->Hide();
  }
}
void UI::Destroy(bool full) {
  if (pimpl_) {
    if (pimpl_->panel.IsWindow())
      pimpl_->panel.DestroyWindow();
    if (full) {
      // ensure window resources and shared devices are released
      pimpl_->panel.ReleaseAllResources();
      pimpl_.reset();
    }
  }
}
bool UI::Create(HWND parent) {
  if (pimpl_) {
    pimpl_->panel.Create(parent);
    return true;
  }
  pimpl_ = std::make_unique<UIImpl>(*this);
  if (!pimpl_)
    return false;
  return pimpl_->panel.Create(parent);
}
bool UI::GetIsReposition() {
  return pimpl_ && pimpl_->panel.GetIsReposition();
}

HWND UI::hwnd() {
  if (pimpl_ && pimpl_->panel.IsWindow())
    return pimpl_->panel.hwnd();
  return nullptr;
}
}  // namespace weasel
