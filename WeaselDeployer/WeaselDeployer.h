#pragma once

#include "resource.h"
#include <string>
#include <atlstr.h>
#include <Windows.h>
#include "resource.h"

#define MSG_BY_IDS(idInfo, idCap, uType)           \
  {                                                \
    CString info, cap;                             \
    info.LoadStringW(idInfo);                      \
    cap.LoadStringW(idCap);                        \
    LANGID langID = GetThreadUILanguage();         \
    MessageBoxExW(NULL, info, cap, uType, langID); \
  }

#define MSG_ID_CAP(info, idCap, uType)             \
  {                                                \
    CString cap;                                   \
    cap.LoadStringW(idCap);                        \
    LANGID langID = GetThreadUILanguage();         \
    MessageBoxExW(NULL, info, cap, uType, langID); \
  }
