#pragma once
#include <msctf.h>
#include <WeaselUI.h>

class CLangBarItemButton : public ITfLangBarItemButton, public ITfSource {
 public:
  CLangBarItemButton(com_ptr<WeaselTSF> pTextService,
                     REFGUID guid,
                     weasel::UIStyle& style);
  ~CLangBarItemButton();

  /* IUnknown */
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  /* ITfLangBarItem */
  STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* pInfo);
  STDMETHODIMP GetStatus(DWORD* pdwStatus);
  STDMETHODIMP Show(BOOL fShow);
  STDMETHODIMP GetTooltipString(BSTR* pbstrToolTip);

  /* ITfLangBarItemButton */
  STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT* prcArea);
  STDMETHODIMP InitMenu(ITfMenu* pMenu);
  STDMETHODIMP OnMenuSelect(UINT wID);
  STDMETHODIMP GetIcon(HICON* phIcon);
  STDMETHODIMP GetText(BSTR* pbstrText);

  /* ITfSource */
  STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD* pdwCookie);
  STDMETHODIMP UnadviseSink(DWORD dwCookie);
  BOOL IsLangBarDisabled() { return (_status & TF_LBI_STATUS_DISABLED); }

  void UpdateWeaselStatus(weasel::Status stat);
  void SetLangbarStatus(DWORD dwStatus, BOOL fSet);

 private:
  GUID _guid;
  com_ptr<WeaselTSF> _pTextService;
  com_ptr<ITfLangBarItemSink> _pLangBarItemSink;
  LONG _cRef; /* COM Reference count */
  DWORD _status;
  bool ascii_mode;
  weasel::UIStyle& _style;
  std::wstring _current_schema_zhung_icon;
  std::wstring _current_schema_ascii_icon;
};
