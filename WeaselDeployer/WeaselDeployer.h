#pragma once

#include "resource.h"
#include <string>

const std::string WeaselDeployerLogFilePath();

#define EZLOGGER_OUTPUT_FILENAME WeaselDeployerLogFilePath()
#define EZLOGGER_REPLACE_EXISTING_LOGFILE_

#pragma warning(disable: 4995)
#pragma warning(disable: 4996)
#include <ezlogger/ezlogger_headers.hpp>
#pragma warning(default: 4996)
#pragma warning(default: 4995)
