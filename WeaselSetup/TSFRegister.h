#pragma once

#include "stdafx.h"

BOOL RegisterProfiles(std::wstring filename, HKL hkl);
void UnregisterProfiles();
BOOL RegisterCategories();
void UnregisterCategories();
BOOL RegisterServer(std::wstring filename);
void UnregisterServer();
