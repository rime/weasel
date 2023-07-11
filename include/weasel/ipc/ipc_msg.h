#pragma once
#include <cstdint>

namespace weasel::ipc
{

#pragma pack(push, 1)
struct key_event
{
  uint16_t key_code;
  uint16_t mask;

  key_event() : key_code(0), mask(0) {}
  key_event(const uint32_t _key_code, const uint32_t _mask) : key_code(_key_code), mask(_mask) {}
  explicit key_event(const uint32_t x)
  {
    *reinterpret_cast<uint32_t*>(this) = x;
  }
  explicit operator uint32_t const() const
  {
    return *reinterpret_cast<uint32_t const*>(this);
  }
};
#pragma pack(pop)

namespace msg
{

}

}
