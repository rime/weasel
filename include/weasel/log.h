#pragma once
#define NOMINMAX
#include "spdlog/spdlog.h"

namespace weasel
{
// compile funcs to decrease build time
inline void init_console(bool to_stderr = false);

#ifdef WEASEL_NO_LOGGING

#define LOG(severity, ...) (void)0
#define CHECK(cond, ...) (void)0

#else // WEASEL_NO_LOGGING

#define LOG(severity, ...) SPDLOG_##severity(__VA_ARGS__)
#define CHECK(cond, ...)       \
  (cond) ?                     \
  (void) 0 :                   \
  LOG(CRITICAL, "CHECK FAILED")

#endif // WEASEL_NO_LOGGING

#if defined(DEBUG) || defined(_DEBUG)

#define DLOG(severity, ...) LOG(severity, __VA_ARGS__)
#define DCHECK(severity, ...) CHECK(severity, __VA_ARGS__)

#else // defined(DEBUG) || defined(_DEBUG)

#define DLOG(severity, ...) (void)0
#define DCHECK(severity, ...) (void)0

#endif // defined(DEBUG) || defined(_DEBUG)


#define LOG_LASTERROR(str) LOG(ERROR, "{0} ({1:x})", str, GetLastError())
#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_LT(a, b) CHECK((a) < (b))

#define DCHECK_EQ(a, b) CHECK_EQ(a, b)
#define DCHECK_NE(a, b) CHECK_NE(a, b)
#define DCHECK_GE(a, b) CHECK_GE(a, b)
#define DCHECK_LE(a, b) CHECK_LE(a, b)
#define DCHECK_GT(a, b) CHECK_GT(a, b)
#define DCHECK_LT(a, b) CHECK_LT(a, b)

} // namespace weasel::common::log

