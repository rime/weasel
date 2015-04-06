#ifndef WEASEL_ATL_H_
#define WEASEL_ATL_H_

#pragma warning(disable : 4996)

#include <atlbase.h>
#include <atlwin.h>

#pragma warning(default: 4996)

#if (_ATL_VER >= 0x0B00)

namespace ATL {
inline HRESULT AtlGetCommCtrlVersion(LPDWORD pdwMajor, LPDWORD pdwMinor) {
  ATLASSERT(pdwMajor != NULL && pdwMinor != NULL);
  if(pdwMajor == NULL || pdwMinor == NULL)
    return E_INVALIDARG;
  *pdwMajor = 6;
  *pdwMinor = 0;
  return S_OK;
}
}  // namespace ATL

#endif

#endif  // WEASEL_ATL_H_
