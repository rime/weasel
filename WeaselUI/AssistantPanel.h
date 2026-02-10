#pragma once

#include <WeaselIPCData.h>
#include <WeaselUI.h>
#include <utility>

typedef CWinTraits<WS_POPUP | WS_CLIPSIBLINGS | WS_DISABLED,
                   WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE |
                       WS_EX_LAYERED>
    CAssistantPanelTraits;

class AssistantPanel
    : public CWindowImpl<AssistantPanel, CWindow, CAssistantPanelTraits>,
      CDoubleBufferImpl<AssistantPanel> {
 public:
  BEGIN_MSG_MAP(AssistantPanel)
  MESSAGE_HANDLER(WM_CREATE, OnCreate)
  MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  MESSAGE_HANDLER(WM_LBUTTONUP, OnLeftClickedUp)
  CHAIN_MSG_MAP(CDoubleBufferImpl<AssistantPanel>)
  END_MSG_MAP()

  AssistantPanel(weasel::UI& ui);
  ~AssistantPanel();

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnLeftClickedUp(UINT uMsg,
                          WPARAM wParam,
                          LPARAM lParam,
                          BOOL& bHandled);

  void MoveTo(RECT const& rc);
  void Refresh();
  void DoPaint(CDCHandle dc);
  void SetData(weasel::AiAnalyzeResponse const& response);
  void ClearData();
  bool GetIsReposition() { return false; }

 private:
  weasel::UI& ui_;
  std::vector<weasel::AiRisk> risks_;
  std::vector<weasel::AiSuggestion> suggestions_;
  std::wstring explanation_;
  std::vector<std::wstring> lines_;
  std::vector<RECT> suggestion_rects_;
  size_t selected_suggestion_index_ = 0;
  std::vector<RECT> action_rects_;
  std::vector<std::pair<weasel::AssistantAction, size_t>> action_meta_;
};
