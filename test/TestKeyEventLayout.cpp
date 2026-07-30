#include <Windows.h>

#include <KeyEvent.h>

#include <cassert>

int main() {
  struct Mapping {
    UINT scan_code;
    UINT expected;
  };
  const Mapping colemak[] = {
      {0x12, 'F'}, {0x13, 'P'}, {0x14, 'G'}, {0x15, 'J'},
      {0x16, 'L'}, {0x17, 'U'}, {0x18, 'Y'}, {0x19, VK_OEM_1},
      {0x1f, 'R'}, {0x20, 'S'}, {0x21, 'T'}, {0x22, 'D'},
      {0x24, 'N'}, {0x25, 'E'}, {0x26, 'I'}, {0x27, 'O'},
      {0x31, 'K'},
  };

  for (const auto& mapping : colemak) {
    assert(RemapKeyByLayout(0, mapping.scan_code, L"colemak") ==
           mapping.expected);
    assert(RemapKeyByLayout(0, mapping.scan_code, L"COLEMAK") ==
           mapping.expected);
  }

  assert(RemapKeyByLayout('Q', 0x10, L"colemak") == 'Q');
  assert(RemapKeyByLayout('E', 0x12, L"") == 'E');
  assert(RemapKeyByLayout('E', 0x12, nullptr) == 'E');
  assert(RemapKeyByLayout(VK_LEFT, 0x4b, L"colemak") == VK_LEFT);

  const Mapping dvorak[] = {
      {0x10, VK_OEM_7},     {0x11, VK_OEM_COMMA}, {0x12, VK_OEM_PERIOD},
      {0x13, 'P'},          {0x14, 'Y'},          {0x15, 'F'},
      {0x16, 'G'},          {0x17, 'C'},          {0x18, 'R'},
      {0x19, 'L'},          {0x1a, VK_OEM_2},     {0x1b, VK_OEM_PLUS},
      {0x1e, 'A'},          {0x1f, 'O'},          {0x20, 'E'},
      {0x21, 'U'},          {0x22, 'I'},          {0x23, 'D'},
      {0x24, 'H'},          {0x25, 'T'},          {0x26, 'N'},
      {0x27, 'S'},          {0x28, VK_OEM_MINUS}, {0x2c, VK_OEM_1},
      {0x2d, 'Q'},          {0x2e, 'J'},          {0x2f, 'K'},
      {0x30, 'X'},          {0x31, 'B'},          {0x32, 'M'},
      {0x33, 'W'},          {0x34, 'V'},          {0x35, 'Z'},
  };
  for (const auto& mapping : dvorak)
    assert(RemapKeyByLayout(0, mapping.scan_code, L"dvorak") ==
           mapping.expected);

  const Mapping workman[] = {
      {0x11, 'D'}, {0x12, 'R'}, {0x13, 'W'}, {0x14, 'B'},
      {0x15, 'J'}, {0x16, 'F'}, {0x17, 'U'}, {0x18, 'P'},
      {0x19, VK_OEM_1},         {0x20, 'H'}, {0x21, 'T'},
      {0x23, 'Y'}, {0x24, 'N'}, {0x25, 'E'}, {0x26, 'O'},
      {0x27, 'I'}, {0x2e, 'M'}, {0x2f, 'C'}, {0x30, 'V'},
      {0x31, 'K'}, {0x32, 'L'},
  };
  for (const auto& mapping : workman)
    assert(RemapKeyByLayout(0, mapping.scan_code, L"workman") ==
           mapping.expected);

  assert(RemapKeyByLayout('E', 0x12, L"qwerty") == 'E');
  assert(RemapKeyByLayout('E', 0x12, L"unknown") == 'E');
}
