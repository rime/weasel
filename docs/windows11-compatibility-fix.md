# Windows 11 兼容性修复：CONVERSION handler 从 blind toggle 改为值驱动

## 问题

[blind toggle 方案](conversion-compartment-fix-analysis.md)（commit `f14f2a7`）在 Windows 10 下正常，但在 Windows 11 下失效：

- Vim 切换到 normal 模式时，RIME 总是变为中文（应为英文）
- AppIME 8 秒无键盘输入后，RIME 仍维持中文（应切回英文）

## 根因

blind toggle（`_status.ascii_mode = !_status.ascii_mode`）依赖 `_updatingLanguageBar` 布尔守卫拦截 `_UpdateLanguageBar` 引起的自触发 `OnChange`。此守卫仅在 `OnChange` **同步**回调时有效。

Windows 11 的 TSF 基础设施（TextInputHost.exe / InputApp.exe）与 Windows 10 行为不同：

1. **异步投递 `OnChange`**：`_UpdateLanguageBar` 设置 `_updatingLanguageBar=true` → 写 compartment → 守卫复位为 `false` → 异步 `OnChange` 到达 → 守卫已失效 → 额外 toggle
2. **系统注入额外 compartment 写入**：Windows 11 的文本输入框架可能在外部写入后自行同步 compartment，触发未被守卫覆盖的 `OnChange`

结果：toggle 次数从偶数（Windows 10）变为奇数（Windows 11），`ascii_mode` 翻转到错误状态。

### Windows 10 vs Windows 11 回调时序对比

```
Windows 10（同步 OnChange）：
  SetValue ──→ OnChange(同步) ──→ _UpdateLanguageBar ──→ SetValue ──→ OnChange(同步, 守卫=true, 跳过)
  toggle 次数: 1（_UpdateLanguageBar 的自触发被守卫拦截）
  ✗ 但 OPENCLOSE handler 也会 toggle，凑成偶数 → 结果正确

Windows 11（异步 OnChange）：
  SetValue ──→ OnChange(异步, 稍后) ──→ _UpdateLanguageBar ──→ SetValue ──→ OnChange(异步, 稍后)
  ......守卫已复位......
  OnChange #1 到达 → toggle #1
  OnChange #2 到达 → toggle #2（守卫已失效）
  OnChange #3 到达 → toggle #3（系统注入的额外写入）
  toggle 次数: 奇数 → 结果错误
```

## 修复方案

### 1. Weasel：CONVERSION handler 改为值驱动

**修改文件：** `WeaselTSF/Compartment.cpp`

将 blind toggle 替换为读取 compartment 实际值并与 `_status.ascii_mode` 比对：

```cpp
// 修复后
} else if (IsEqualGUID(guidCompartment,
                       GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)) {
  if (_updatingLanguageBar)
    return S_OK;
  DWORD convMode = 0;
  _GetCompartmentDWORD(convMode,
                       GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION);
  bool desiredAsciiMode = !(convMode & TF_CONVERSIONMODE_NATIVE);
  if (desiredAsciiMode != _status.ascii_mode) {
    _status.ascii_mode = desiredAsciiMode;
    _SetKeyboardOpen(true);
    if (_pLangBarButton && _pLangBarButton->IsLangBarDisabled())
      _EnableLanguageBar(true);
    _HandleLangBarMenuSelect(_status.ascii_mode
                                 ? ID_WEASELTRAY_ENABLE_ASCII
                                 : ID_WEASELTRAY_DISABLE_ASCII);
    if (_pEditSessionContext)
      m_client.ClearComposition();
    _UpdateLanguageBar(_status);
  }
}
```

**天然幂等性**：即使 `_updatingLanguageBar` 守卫失效（异步回调 / 系统注入），重新读取 compartment 值会得到与当前 `_status.ascii_mode` 一致的结果（因为 `_UpdateLanguageBar` 已将正确值写入），`desiredAsciiMode == _status.ascii_mode` → 跳过。不会产生额外翻转。

### 2. im-control：OPENCLOSE 跳过未变化的写入（防御性优化）

**修改文件：** `injector/hook.cpp`

im-control 原先对 `GUID_COMPARTMENT_KEYBOARD_OPENCLOSE` 无条件 `SetValue`，即使值未变。这会触发 Weasel 的 OPENCLOSE handler（`_isToOpenClose=false` 时仍为 blind toggle），产生不必要的 `ascii_mode` 翻转。

修复：写入前先 `GetValue` 比对，仅在值变化时 `SetValue`，与 CONVERSION 的已有逻辑一致。

> 此改动非 Windows 11 兼容性修复的必要条件——Weasel 的 CONVERSION 值驱动修复已能校正 OPENCLOSE handler 的错误 toggle。但消除不必要的 OPENCLOSE 写入可以避免 RIME 引擎收到一错一对的 TrayCommand，减少时序问题。

## 方案对比

| 项目 | blind toggle (`f14f2a7`) | 值驱动（本修复） |
|------|--------------------------|------------------|
| 回调次数依赖 | 严格依赖偶数次 `OnChange` | 幂等，不依赖回调次数 |
| 重入守卫 | 唯一防线，异步回调下失效 | 第一防线；值比对是第二防线 |
| Windows 10 | 正常 | 正常 |
| Windows 11 | 失效（奇数次 toggle） | 正常（幂等跳过） |
| stale value 风险 | 无 | 若 `GetValue` 返回旧值则静默跳过（安全失败） |

## 为什么不改 OPENCLOSE handler

OPENCLOSE handler 的 else 分支（`_isToOpenClose=false`）使用 blind toggle 是**为 Ctrl+Space 设计的**——每次 Ctrl+Space 切换 OPENCLOSE 值，Weasel 对应翻转 `ascii_mode`。改为值驱动会破坏此行为。

OPENCLOSE 值（open/close）与 `ascii_mode`（中/英）没有直接映射关系：OPENCLOSE=true 表示 IME 激活（可能是中文也可能是英文），OPENCLOSE=false 表示 IME 关闭（英文直通）。因此无法像 CONVERSION 那样从 compartment 值推导 `ascii_mode`。

通过 im-control 侧跳过未变化的 OPENCLOSE 写入，避免从外部触发此 handler，是最小侵入的解决方案。

## 部署

### 编译

在装有 Visual Studio 2022 + Boost 的机器上编译：

```
cd C:\Apps\git-kb\repos\VimWei\weasel
git pull
build.bat weasel release
```

产出 `output\weasel.dll`（Win32）和 `output\weaselx64.dll`（x64）。

### 部署 weasel.dll

管理员 PowerShell：

```powershell
Copy-Item output\weaselx64.dll C:\Windows\system32\weasel.dll -Force
Copy-Item output\weasel.dll C:\Windows\SysWOW64\weasel.dll -Force
```

> TSF 框架从系统路径加载 DLL，详见 [原始修复文档](conversion-compartment-fix-analysis.md#2-dll-部署位置)。

### 编译部署 im-control（可选）

```
cd C:\Apps\git-kb\repos\VimWei\im-control
git pull
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config RelWithDebInfo
cmake --install build --prefix bin --config RelWithDebInfo
```

```powershell
Copy-Item bin\* "C:\Apps\VimReader\lib\utils\im-control\" -Force
```

## 相关文档

- [原始修复：CONVERSION handler 响应外部变更](conversion-compartment-fix-analysis.md) — blind toggle 方案（Windows 10）
- [rime/weasel#1371](https://github.com/rime/weasel/issues/1371)
- [im-control](https://github.com/VimWei/im-control)
