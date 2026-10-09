# WeaselTSF Compartment 外部控制修复分析

## 问题

通过 TSF compartment 程序化切换中英文模式（如 `ITfCompartment::SetValue(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)`）对 Weasel（小狼毫）无效。外部工具写入 compartment 后，Weasel 不切换模式，且 compartment 值被恢复原状。

此问题在 Windows 10 和 Windows 11 上表现不同：
- **Windows 10**：compartment 写入能触发 `OnChange`，但 CONVERSION handler 逻辑缺陷导致外部变更被撤销；gvim ESC 关闭键盘后无人重开，导致 RIME 被禁用
- **Windows 11**：compartment 写入根本不触发 `OnChange`，且 CONVERSION handler 存在级联反转问题

## 架构背景

WeaselTSF 是 TSF Text Service（实现 `ITfTextInputProcessorEx`），不接收 `WM_IME_CONTROL`。Windows TSF 框架负责在 IMM32 API 与 TSF compartments 之间桥接：

```
ImmSetConversionStatus(IME_CMODE_NATIVE)
    → TSF framework 翻译
        → ITfCompartment::SetValue(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)
            → _HandleCompartment()  ← 修复位置
```

两个 compartment 各司其职：
- **GUID_COMPARTMENT_KEYBOARD_OPENCLOSE**：管理 IME 启用/禁用（对应 Ctrl+Space、`-k open/close`）
- **GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION**：管理中/英文模式（对应 Shift 切换、`-c native/alphanumeric`）

### Weasel 配置 `ToggleImeOnOpenClose`

注册表 `HKCU\Software\Rime\weasel\ToggleImeOnOpenClose` 控制 OPENCLOSE handler 的行为分支，是 Win10/Win11 差异的根源：

| 配置值 | `_isToOpenClose` | OPENCLOSE handler 行为 | 典型场景 |
|--------|-------------------|------------------------|----------|
| `yes`  | `true`  | **if 分支**：OPENCLOSE=0 真正关闭键盘，OPENCLOSE=1 真正打开；不 toggle ascii_mode | Win10 |
| `no`   | `false` | **else 分支**：blind toggle ascii_mode；始终 `_SetKeyboardOpen(true)` 自动重开键盘 | Win11 |

## 根因

经 Windows 10 和 Windows 11 实测和日志分析，问题有**五个独立的层面**：

### 1. CONVERSION handler 撤销外部变更（Windows 10）

`_HandleCompartment` 中 `GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION` 的原始实现：

```cpp
// 原始代码 (commit 93eec2d)
} else if (IsEqualGUID(guidCompartment,
                       GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)) {
  BOOL isOpen = _IsKeyboardOpen();
  if (isOpen) {
    weasel::ResponseParser parser(NULL, NULL, &_status, NULL,
                                  &_cand->style());
    bool ok = m_client.GetResponseData(std::ref(parser));  // 查询 RIME 后端当前状态
    _UpdateLanguageBar(_status);                           // 写回 compartment
  }
}
```

**问题链条**：

1. 外部工具（如 im-control）写入 compartment，清除 `TF_CONVERSIONMODE_NATIVE` 位（请求切英文）
2. TSF 触发 `ITfCompartmentEventSink::OnChange` → `_HandleCompartment`
3. handler 调用 `m_client.GetResponseData(parser)` —— 从 RIME 后端获取**当前状态**（仍是中文，因为 RIME 还没收到切换指令）
4. handler 调用 `_UpdateLanguageBar(_status)` —— 把当前状态（中文）写回 compartment
5. compartment 被恢复为 `TF_CONVERSIONMODE_NATIVE` 置位 —— **外部变更被静默撤销**

原始代码从不读取 compartment 值来判断外部请求的目标模式，也不调用 `_HandleLangBarMenuSelect` 通知 RIME 引擎切换。它只是把 RIME 后端的当前状态同步回 compartment，方向与需求完全相反。

### 2. CoCreateInstance(CLSID_TF_ThreadMgr) 在 Win11 返回新实例

im-control 的 hook 用 `CoCreateInstance(CLSID_TF_ThreadMgr)` 获取 ThreadMgr。Windows 10 返回 per-thread 单例（与 WeaselTSF 注册 sink 的实例相同），Windows 11 返回**新实例**——compartment 写入到了不同实例，WeaselTSF 的 sink 收不到 OnChange。

### 3. TF_CLIENTID_NULL 的 SetValue 在 Win11 不触发 OnChange

im-control 的 hook 用 `SetValue(0, ...)`（`TF_CLIENTID_NULL`）。WeaselTSF 自身用 `SetValue(_tfClientId, ...)`（有效非零 ID）。

**Windows 10**：不区分 `TfClientId`，所有 `SetValue` 都触发 `OnChange`。
**Windows 11**：仅为已激活客户端（非零 `TfClientId`）的写入触发 `OnChange`。

### 4. CONVERSION handler 的 _SetKeyboardOpen(true) 引发级联反转（Windows 11）

CONVERSION handler 调用 `_SetKeyboardOpen(true)` 写 OPENCLOSE=1，触发 OPENCLOSE handler 的 else 分支（`_isToOpenClose=false`，为 Shift 键设计的 blind toggle），将 CONVERSION 刚设置好的 `ascii_mode` 反转。

### 5. gvim ESC 关闭键盘后无人重开（Windows 10）

gvim 在退出 insert mode 时会写 OPENCLOSE=0 关闭键盘。在 `_isToOpenClose=true`（Win10）时，OPENCLOSE handler 的 if 分支只做 `_EnableLanguageBar(isOpen)` 和 `_UpdateLanguageBar`，**不重开键盘**。如果此时 im-control 的 CONVERSION 写入因值未变而跳过 SetValue（`newMode == oldMode`），则 CONVERSION OnChange 不触发，键盘一直关闭，RIME 被禁用。

**Windows 11** 不受此问题影响：`_isToOpenClose=false` 的 else 分支始终调用 `_SetKeyboardOpen(true)` 自动重开。

## 修复方案

### 1. Weasel：CONVERSION handler 改为值驱动

**修改文件：** `WeaselTSF/Compartment.cpp`

将原始的"查询后端 → 写回 compartment"改为"读取 compartment 值 → 比对状态 → 通知 RIME → 同步 UI"：

```cpp
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
    if (_isToOpenClose && !_IsKeyboardOpen()) {
      _SetKeyboardOpen(true);
    }
    // ... 通知 RIME、同步 UI ...
    _UpdateLanguageBar(_status);
  } else {
    if (_isToOpenClose && !_IsKeyboardOpen()) {
      _SetKeyboardOpen(true);
      // ... 启用 LanguageBar ...
    }
  }
}
```

**天然幂等性**：即使 `_updatingLanguageBar` 守卫失效（异步回调 / 系统注入），重新读取 compartment 值会得到与当前 `_status.ascii_mode` 一致的结果，`desiredAsciiMode == _status.ascii_mode` → 跳过。不会产生额外翻转。

### 2. Weasel：CONVERSION handler 按 `_isToOpenClose` 条件调用 `_SetKeyboardOpen(true)`

**修改文件：** `WeaselTSF/Compartment.cpp`

`_SetKeyboardOpen(true)` 的调用由 `_isToOpenClose` 控制，避免在错误场景下触发级联反转：

| `_isToOpenClose` | CONVERSION handler 调用 `_SetKeyboardOpen(true)`？ | 原因 |
|-------------------|---------------------------------------------------|------|
| `true` (Win10)    | **是** — 键盘关闭时重开                              | OPENCLOSE if 分支不自动重开，需要 CONVERSION handler 补救 |
| `false` (Win11)   | **否** — 不触碰 OPENCLOSE                           | OPENCLOSE else 分支自己 `_SetKeyboardOpen(true)` + blind toggle，CONVERSION 写 OPENCLOSE 会触发级联反转 |

### 3. Weasel：重入守卫

**修改文件：** `WeaselTSF/LanguageBar.cpp`、`WeaselTSF/WeaselTSF.h`

`_UpdateLanguageBar` 内部调用 `_SetCompartmentDWORD` 写入 CONVERSION compartment，这会再次触发 `OnChange` → `_HandleCompartment`。如果不加守卫，handler 会再次处理，导致双重翻转或无限递归。

```cpp
// LanguageBar.cpp
void WeaselTSF::_UpdateLanguageBar(weasel::Status stat) {
  // ...
  _updatingLanguageBar = true;
  _SetCompartmentDWORD(flags, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION);
  _updatingLanguageBar = false;
  // ...
}
```

```cpp
// WeaselTSF.h
BOOL _updatingLanguageBar = false;
```

### 4. im-control：使用 TF_GetThreadMgr 获取 per-thread 单例

**修改文件：** `injector/hook.cpp`

用 msctf.dll 导出的 `TF_GetThreadMgr` 替代 `CoCreateInstance(CLSID_TF_ThreadMgr)`，确保获取与 WeaselTSF 相同的 ThreadMgr 实例。

### 5. im-control：使用有效 TfClientId 调用 SetValue

**修改文件：** `injector/hook.cpp`

调用 `ITfThreadMgr::Activate` 获取有效 `TfClientId`，用于所有 `SetValue` 调用，完成后 `Deactivate`。

### 6. im-control：OPENCLOSE 跳过未变化的写入

**修改文件：** `injector/hook.cpp`

写入前先 `GetValue` 比对，仅在值变化时 `SetValue`，避免不必要地触发 OPENCLOSE handler。

### 7. im-control：读注册表条件重开 OPENCLOSE

**修改文件：** `injector/hook.cpp`

hook DLL 在 `DllMain` 时读取注册表 `HKCU\Software\Rime\weasel\ToggleImeOnOpenClose`，缓存到全局变量 `g_isToOpenClose`。CONVERSION 写入后，仅在 `g_isToOpenClose=true` 且 `keyboardOpenClose` 未设置且 OPENCLOSE 当前为 0 时，重开键盘：

```cpp
// DllMain DLL_PROCESS_ATTACH:
g_isToOpenClose = ReadToggleImeOnOpenClose();

// hook 触发，CONVERSION 写入后:
if (g_isToOpenClose && g_pSharedData->conversionModeNative && !g_pSharedData->keyboardOpenClose) {
    // 读 OPENCLOSE 当前值，仅当 == 0 时 SetValue(1) 重开
}
```

| `ToggleImeOnOpenClose` | hook 行为 | 原因 |
|------------------------|-----------|------|
| `yes` (Win10)          | 检查并重开 OPENCLOSE | OPENCLOSE if 分支不自动重开，gvim ESC 关键盘后需要 hook 补救 |
| `no` (Win11)           | **不干预** OPENCLOSE | OPENCLOSE else 分支自动重开，hook 干预会导致级联反转 |

**与 Weasel 侧修复的协同**：Weasel CONVERSION handler 也在 `_isToOpenClose=true` 时重开键盘。两者互为保底——任一方先触发即可重开，另一方检测到已打开则跳过（幂等）。

### 8. vim 插件：去掉 -k open，只写 CONVERSION

**修改文件：** `autoload/im_select.vim`（vim-im-select）

`im_control_set_mode()` 从 `[im-control, '-k', 'open', '-c', 'native']` 改为 `[im-control, '-c', 'native']`，两个 compartment 完全解耦。

## 设计原则

遵循 fxliang 的指导：

> GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION 和 GUID_COMPARTMENT_KEYBOARD_OPENCLOSE 是不一样的消息，作用要区分。在 GUID_COMPARTMENT_KEYBOARD_OPENCLOSE 的情况下 GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION 的消息不应该再作任何响应，否则就是对旧功能的 break。需要明确的是，输入功能是主功能，外部消息控制是辅助，不能因为要引入辅助功能导致主功能失效是基本要求。

修复完全符合此原则：
- OPENCLOSE handler 未改动（Shift/Ctrl+Space 主功能不受影响）
- CONVERSION handler 对 OPENCLOSE 的干预由 `_isToOpenClose` 严格控制：仅在 OPENCLOSE handler 自身不重开键盘的配置下才介入
- 外部工具只写 CONVERSION 切换中英文，不写 OPENCLOSE（除 `g_isToOpenClose=true` 时的保底重开）

## Win10/Win11 差异总结

| 维度 | Windows 10 (`ToggleImeOnOpenClose=yes`) | Windows 11 (`ToggleImeOnOpenClose=no`) |
|------|------------------------------------------|------------------------------------------|
| `_isToOpenClose` | `true` | `false` |
| OPENCLOSE handler | if 分支：真正开/关键盘，不 toggle ascii | else 分支：blind toggle ascii + 自动重开键盘 |
| gvim ESC 关键盘 | 键盘保持关闭，需要外部重开 | else 分支自动 `_SetKeyboardOpen(true)` 重开 |
| CONVERSION handler `_SetKeyboardOpen` | **调用**（重开键盘） | **不调用**（避免级联反转） |
| im-control hook OPENCLOSE 重开 | **启用**（读注册表 `yes`） | **禁用**（读注册表 `no`） |
| 级联反转风险 | 无（if 分支不 toggle） | 有（else 分支 blind toggle） |

## 方案对比

| 项目 | 原始代码 (93eec2d) | blind toggle (f14f2a7) | 值驱动 + `_isToOpenClose` 分支（最终） |
|------|---------------------|------------------------|----------------------------------------|
| 响应方式 | 查询后端→写回（撤销外部变更） | blind toggle | 读取值→比对→切换 |
| 回调次数依赖 | N/A | 严格依赖偶数次 OnChange | 幂等，不依赖回调次数 |
| `_SetKeyboardOpen(true)` | 无 | 无条件调用（Win11 级联反转） | 按 `_isToOpenClose` 条件调用 |
| OPENCLOSE 重开 | 无 | 无 | Weasel + im-control 双重保底 |
| Windows 10 | 失效 | 正常 | 正常 |
| Windows 11 | 失效 | 失效 | 正常 |

## DLL 部署位置

WeaselTSF 的 CLSID `{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}` 注册路径为：

| 架构 | 注册路径 |
|------|---------|
| 64-bit | `C:\Windows\system32\weasel.dll` |
| 32-bit | `C:\Windows\SysWOW64\weasel.dll` |

TSF 框架从上述系统路径加载 DLL。如果将修复后的 DLL 部署到其他路径（如 `C:\Program Files\Rime\weasel-0.17.4\`），系统不会加载它，进程内仍是旧 DLL。

### 更新方法

管理员身份打开 PowerShell，执行：

```powershell
Rename-Item "C:\Windows\system32\weasel.dll" "weasel.dll.bak" -Force
Copy-Item "C:\path\to\weaselx64.dll" "C:\Windows\system32\weasel.dll" -Force
Rename-Item "C:\Windows\SysWOW64\weasel.dll" "weasel.dll.bak" -Force
Copy-Item "C:\path\to\weasel.dll" "C:\Windows\SysWOW64\weasel.dll" -Force
```

### 重要：部署后需重启所有使用 RIME 的应用

`weasel.dll` 由 TSF 框架在进程启动时加载。部署新 DLL 后，已运行的进程仍使用内存中的旧 DLL。必须重启 Windows Terminal、Total Commander、gvim 等应用（或重启 Windows）才能加载新版本。

## 编译

### Weasel

在装有 Visual Studio 2022 + Boost 的机器上编译：

```
cd C:\Apps\git-kb\repos\VimWei\weasel
git pull
build.bat weasel release
```

产出 `output\weasel.dll`（Win32）和 `output\weaselx64.dll`（x64）。

### im-control

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

### vim 插件

vim-im-select 插件为纯脚本，无需编译，pull 后 reload vim 配置即可。

## 验证

### 测试环境

- 操作系统：Windows 10 pro 19045.7417 和 Windows 11 home 26200.8655
- 前台进程：gvim 9.2.0735、Windows Terminal、Total Commander
- 外部工具：im-control —— 通过 `SetWindowsHookEx` 注入 hook DLL 到前台进程，在目标线程内调用 `ITfCompartment::SetValue`

### 测试结果

Windows 10 和 Windows 11 下均通过：
- gvim 或 terminal vim: normal 英文 → i → edit, RIME 保持英文
- gvim 或 terminal vim: edit 英文 → ESC → normal, RIME 保持英文，不被禁用
- gvim 或 terminal vim: edit 中文 → ESC → normal, RIME 自动切回英文
- gvim 或 terminal vim: 进入/离开 command mode 与 insert mode 行为一致
- Shift 切换中英文、Ctrl+Space 启停 IME 均正常
- Windows Terminal / Total Commander：AppIME 进入窗口自动切英文，8 秒空闲后自动切回英文

## 相关链接

- [rime/weasel#1371](https://github.com/rime/weasel/issues/1371)
- [weasel fork](https://github.com/VimWei/weasel/tree/im-control)
- [im-control fork](https://github.com/VimWei/im-control/tree/dev)
- [vim-im-select fork](https://github.com/VimWei/vim-im-select/tree/im-control)
