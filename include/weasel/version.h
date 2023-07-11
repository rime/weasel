#pragma once

#define STR_(x) #x
#define STR(x) STR_(x)

#define VERSION_MAJOR 0
#define VERSION_MINOR 15
#define VERSION_PATCH 0
#define VERSION_BUILD 0

#define BUILD_COMMIT "??????"
#define BUILD_TAG "unknown"
#define GIT_STATE "dirty"

#define WEASEL_VERSION_STRING STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_PATCH) "." STR(VERSION_BUILD)
#define WEASEL_BUILD_STRING BUILD_COMMIT "(" BUILD_TAG ") " GIT_STATE
#define WEASEL_IME_NAME_WSTR L"小狼毫"
#define WEASEL_IME_NAME_U8   u8"小狼毫"
#define WEASEL_CODE_NAME     "Weasel"
