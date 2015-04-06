#ifndef WEASEL_LOGGGING_H_
#define WEASEL_LOGGGING_H_

#ifdef WEASEL_ENABLE_LOGGING
#define GLOG_NO_ABBREVIATED_SEVERITIES
#pragma warning(disable : 4244)
#include <glog/logging.h>
#pragma warning(default : 4244)
#else
#include "no_logging.h"
#endif  // WEASEL_ENABLE_LOGGING

#endif  // WEASEL_LOGGGING_H_
