#include "stdafx.h"

#include "AssistantPanel.h"

#include <algorithm>

AssistantPanel::AssistantPanel(weasel::UI& ui) : ui_(ui) {}

AssistantPanel::~AssistantPanel() {}

LRESULT AssistantPanel::OnCreate(UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam,
                                 BOOL& bHandled) {
  return TRUE;
}

LRESULT AssistantPanel::OnDestroy(UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  BOOL& bHandled) {
  return 0;
}

LRESULT AssistantPanel::OnLeftClickedUp(UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam,
                                        BOOL& bHandled) {
  POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

  for (size_t i = 0; i < suggestion_rects_.size(); ++i) {
    if (PtInRect(&suggestion_rects_[i], pt)) {
      selected_suggestion_index_ = i;
      Refresh();
      return 0;
    }
  }

  for (size_t i = 0; i < action_rects_.size() && i < action_meta_.size(); ++i) {
    if (PtInRect(&action_rects_[i], pt)) {
      ui_.HandleAssistantAction(action_meta_[i].first, action_meta_[i].second);
      break;
    }
  }
  return 0;
}

void AssistantPanel::MoveTo(RECT const& rc) {
  SetWindowPos(NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
               SWP_NOACTIVATE | SWP_NOZORDER);
}

void AssistantPanel::Refresh() {
  lines_.clear();
  suggestion_rects_.clear();
  action_rects_.clear();
  action_meta_.clear();
  if (!explanation_.empty()) {
    lines_.push_back(std::wstring(L"AI: ") + explanation_);
  }
  for (size_t i = 0; i < risks_.size(); ++i) {
    std::wstring line = L"Risk: " + risks_[i].text;
    if (!risks_[i].reason.empty()) {
      line.append(L" (" + risks_[i].reason + L")");
    }
    lines_.push_back(line);
    if (i >= 2) {
      break;
    }
  }
  for (size_t i = 0; i < suggestions_.size(); ++i) {
    lines_.push_back(std::wstring(L"Suggest: ") + suggestions_[i].text);
    if (i >= 2) {
      break;
    }
  }
  Invalidate();
}

void AssistantPanel::DoPaint(CDCHandle dc) {
  RECT rc = {0};
  GetClientRect(&rc);
  HBRUSH back = CreateSolidBrush(RGB(245, 250, 255));
  dc.FillRect(&rc, back);
  DeleteObject(back);

  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, RGB(33, 37, 41));

  int y = 8;
  const int line_height = 20;

  if (!explanation_.empty()) {
    RECT text_rc = {8, y, rc.right - 8, y + line_height};
    std::wstring line = std::wstring(L"AI: ") + explanation_;
    DrawTextW(dc, line.c_str(), -1, &text_rc,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    y += line_height;
  }

  for (size_t i = 0; i < risks_.size() && i < 3; ++i) {
    RECT text_rc = {8, y, rc.right - 8, y + line_height};
    std::wstring line = std::wstring(L"Risk: ") + risks_[i].text;
    if (!risks_[i].reason.empty()) {
      line.append(L" (").append(risks_[i].reason).append(L")");
    }
    DrawTextW(dc, line.c_str(), -1, &text_rc,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    y += line_height;
  }

  suggestion_rects_.clear();
  for (size_t i = 0; i < suggestions_.size() && i < 3; ++i) {
    RECT text_rc = {8, y, rc.right - 8, y + line_height};
    if (i == selected_suggestion_index_) {
      HBRUSH sel = CreateSolidBrush(RGB(227, 242, 253));
      dc.FillRect(&text_rc, sel);
      DeleteObject(sel);
    }
    std::wstring line = std::wstring(L"Suggest ") + std::to_wstring(i + 1) +
                        L": " + suggestions_[i].text;
    DrawTextW(dc, line.c_str(), -1, &text_rc,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    suggestion_rects_.push_back(text_rc);
    y += line_height;
  }

  const int button_h = 24;
  const int button_w = 94;
  const int gap = 8;
  int by = max(8, rc.bottom - button_h - 8);
  int bx = 8;

  const wchar_t* labels[] = {L"Apply", L"Replace All", L"Ignore"};
  weasel::AssistantAction actions[] = {
      weasel::AssistantAction::ApplyItem,
      weasel::AssistantAction::ReplaceAll,
      weasel::AssistantAction::IgnoreAndSend,
  };

  for (int i = 0; i < 3; ++i) {
    bool enabled = (i == 2) || !suggestions_.empty();
    RECT br = {bx, by, bx + button_w, by + button_h};
    HBRUSH btn = CreateSolidBrush(enabled ? RGB(223, 234, 246) : RGB(238, 238, 238));
    dc.FillRect(&br, btn);
    DeleteObject(btn);
    FrameRect(dc, &br, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
    DrawTextW(dc, labels[i], -1, &br,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (enabled) {
      action_rects_.push_back(br);
      size_t action_index = (i == 0 && !suggestions_.empty())
                                ? (std::min)(selected_suggestion_index_, suggestions_.size() - 1)
                                : 0;
      action_meta_.push_back(std::make_pair(actions[i], action_index));
    }
    bx += button_w + gap;
  }
}

void AssistantPanel::SetData(weasel::AiAnalyzeResponse const& response) {
  explanation_ = response.explanation;
  risks_ = response.risks;
  suggestions_ = response.suggestions;
  if (selected_suggestion_index_ >= suggestions_.size()) {
    selected_suggestion_index_ = 0;
  }
}

void AssistantPanel::ClearData() {
  explanation_.clear();
  risks_.clear();
  suggestions_.clear();
  lines_.clear();
  suggestion_rects_.clear();
  selected_suggestion_index_ = 0;
}
