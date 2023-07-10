#pragma once
#include <cstdint>

namespace weasel::ipc
{

#pragma pack(push, 1)
struct key_event
{
  uint16_t key_code;
  uint16_t mask;

  explicit operator uint32_t const() const
  {
    return *reinterpret_cast<uint32_t const*>(this);
  }
};
#pragma pack(pop)

}
