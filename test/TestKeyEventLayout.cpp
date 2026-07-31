#include <Windows.h>

#include <KeyEvent.h>

#include <cassert>

int main() {
  KeyInfo qKey(0);
  qKey.scanCode = 0x10;

  assert(FindKeyboardLayout(nullptr) == nullptr);
  assert(FindKeyboardLayout(L"") == nullptr);
  assert(FindKeyboardLayout(L"not-a-klid") == nullptr);
  assert(FindKeyboardLayout(L"ffffffff") == nullptr);

  HKL colemak = FindKeyboardLayout(L"00060409");
  assert(FindKeyboardLayout(L"colemak") == colemak);
  HKL activeLayout = GetKeyboardLayout(0);
  HKL loadedLayouts[64] = {};
  int loadedCount = GetKeyboardLayoutList(_countof(loadedLayouts),
                                          loadedLayouts);
  HKL dvorak = FindKeyboardLayout(L"United States-Dvorak");
  assert(dvorak);
  assert(GetKeyboardLayout(0) == activeLayout);
  bool dvorakWasLoaded = false;
  for (int i = 0; i < loadedCount; ++i)
    dvorakWasLoaded |= loadedLayouts[i] == dvorak;
  if (!dvorakWasLoaded)
    assert(UnloadKeyboardLayout(dvorak));
  if (colemak) {
    assert(VirtualKeyForLayout('Q', qKey, colemak) == 'Q');

    KeyInfo eKey(0);
    eKey.scanCode = 0x12;
    assert(VirtualKeyForLayout('E', eKey, colemak) == 'F');

    BYTE keyState[256] = {};
    weasel::KeyEvent event;
    assert(ConvertKeyEvent('E', eKey, keyState, colemak, event));
    assert(event.keycode == 'f');

    keyState[VK_SHIFT] = 0x80;
    assert(ConvertKeyEvent('E', eKey, keyState, colemak, event));
    assert(event.keycode == 'F');

    keyState[VK_SHIFT] = 0;
    eKey.isKeyUp = 1;
    assert(ConvertKeyEvent('E', eKey, keyState, colemak, event));
    assert(event.keycode == 'f');
    assert(event.mask & ibus::RELEASE_MASK);
    eKey.isKeyUp = 0;

    const struct {
      UINT scanCode;
      UINT keycode;
    } specialKeys[] = {
        {0x0e, ibus::BackSpace},
        {0x0f, ibus::Tab},
        {0x1c, ibus::Return},
        {0x01, ibus::Escape},
    };
    for (const auto& specialKey : specialKeys) {
      KeyInfo key(0);
      key.scanCode = specialKey.scanCode;
      assert(ConvertKeyEvent(VK_OEM_1, key, keyState, colemak, event));
      assert(event.keycode == specialKey.keycode);
    }
  }

  assert(VirtualKeyForLayout('Q', qKey, nullptr) == 'Q');
}
