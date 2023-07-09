#include "pch.h"
#include <weasel/log.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace weasel::log
{

namespace
{

void init_shared()
{
  spdlog::set_pattern("%C-%m-%d %T %!(%s:%#): %v");
  spdlog::flush_on(spdlog::level::warn);
#if defined(DEBUG) || defined(_DEBUG)
  spdlog::set_level(spdlog::level::debug);
#endif
}

}

void init_console(const bool to_stderr) {
  std::shared_ptr<spdlog::logger> l;
  if (to_stderr) {
    l = spdlog::stdout_color_mt("console");
  } else {
    l = spdlog::stderr_color_mt("console");
  }

  spdlog::set_default_logger(l);
  init_shared();
}

} // namespace weasel::common::log