#ifndef RIME_NO_LOGGING_H_
#define RIME_NO_LOGGING_H_

namespace rime {

class VoidLogger {
 public:
  VoidLogger() {}

  // hack: an unnamed VoidLogger() cannot be used as an l-value
  VoidLogger& stream() { return *this; }

  template <class T>
  VoidLogger& operator<< (const T& x) { return *this; }
};

// to avoid compiler warnings
class Voidify {
 public:
  Voidify() {}
  void operator& (VoidLogger&) {}
};

}  // namespace rime

#define RIME_NO_LOG true ? (void) 0 : rime::Voidify() & rime::VoidLogger().stream()

#define LOG(severity) RIME_NO_LOG
#define VLOG(verboselevel) RIME_NO_LOG
#define LOG_IF(severity, condition) RIME_NO_LOG
#define LOG_EVERY_N(severity, n) RIME_NO_LOG
#define LOG_IF_EVERY_N(severity, condition, n) RIME_NO_LOG
#define LOG_ASSERT(condition) RIME_NO_LOG

#define RIME_NO_CHECK (void) 0

#define CHECK(condition) RIME_NO_CHECK
#define CHECK_EQ(val1, val2) RIME_NO_CHECK
#define CHECK_NE(val1, val2) RIME_NO_CHECK
#define CHECK_LE(val1, val2) RIME_NO_CHECK
#define CHECK_LT(val1, val2) RIME_NO_CHECK
#define CHECK_GE(val1, val2) RIME_NO_CHECK
#define CHECK_GT(val1, val2) RIME_NO_CHECK
#define CHECK_NOTNULL(val) RIME_NO_CHECK
#define CHECK_STREQ(str1, str2) RIME_NO_CHECK
#define CHECK_STRCASEEQ(str1, str2) RIME_NO_CHECK
#define CHECK_STRNE(str1, str2) RIME_NO_CHECK
#define CHECK_STRCASENE(str1, str2) RIME_NO_CHECK

#define DLOG(severity) LOG(severity)
#define DVLOG(verboselevel) VLOG(verboselevel)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_EVERY_N(severity, n) LOG_EVERY_N(severity, n)
#define DLOG_IF_EVERY_N(severity, condition, n) \
  LOG_IF_EVERY_N(severity, condition, n)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)

#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val) CHECK_NOTNULL(val)
#define DCHECK_STREQ(str1, str2) CHECK_STREQ(str1, str2)
#define DCHECK_STRCASEEQ(str1, str2) CHECK_STRCASEEQ(str1, str2)
#define DCHECK_STRNE(str1, str2) CHECK_STRNE(str1, str2)
#define DCHECK_STRCASENE(str1, str2) CHECK_STRCASENE(str1, str2)

#endif  // RIME_NO_LOGGING_H_
