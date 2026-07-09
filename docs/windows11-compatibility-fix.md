# Windows 11 兼容性修复：CONVERSION handler 从 blind toggle 改为值驱动

## 问题

[blind toggle 方案](conversion-compartment-fix-analysis.md)（commit `f14f2a7`）在 Windows 10 下正常，但在 Windows 11 下失效：

- Vim 切换到 normal 模式时，RIME 总是变为中文（应为英文）
- AppIME 8 秒无键盘输入后，RIME 仍维持中文（应切回英文）

## 根因

经 Windows 11 实测和日志分析，问题有**三个独立的层面**：

### 层面 1：CoCreateInstance(CLSID_TF_ThreadMgr) 在 Win11 返回新实例

im-control 的 hook 用 `CoCreateInstance(CLSID_TF_ThreadMgr)` 获取 ThreadMgr。Windows 10 返回 per-thread 单例（与 WeaselTSF 注册 sink 的实例相同），Windows 11 返回**新实例**——compartment 写入到了不同实例，WeaselTSF 的 sink 收不到 OnChange。

**日志证据**：im-control `SetValue` 返回 `S_OK`，但 Weasel 日志中 `_HandleCompartment` 完全没有被调用。

### 层面 2：TF_CLIENTID_NULL 的 SetValue 在 Win11 不触发 OnChange

im-control 的 hook 用 `SetValue(0, ...)`（`TF_CLIENTID_NULL`）。WeaselTSF 自身用 `SetValue(_tfClientId, ...)`（有效非零 ID）。

**Windows 10**：不区分 `TfClientId`，所有 `SetValue` 都触发 `OnChange`。
**Windows 11**：仅为已激活客户端（非零 `TfClientId`）的写入触发 `OnChange`。

### 层面 3：CONVERSION handler 的 _SetKeyboardOpen(true) 引发级联反转

CONVERSION handler 调用 `_SetKeyboardOpen(true)` 写 OPENCLOSE=1，触发 OPENCLOSE handler 的 else 分支（`_isToOpenClose=false`，为 Shift 键设计的 blind toggle），将 CONVERSION 刚设置好的 `ascii_mode` 反转。

**日志证据**：
```
CONVERSION: processing -> switching mode (ascii 0 -> 1)     ← 正确
OPENCLOSE OnChange: isOpen=1 -> toggle ascii_mode 1 -> 0    ← 级联反转
CONVERSION done: ascii_mode=0                                ← 结果中文（错）
```

移除 `_SetKeyboardOpen(true)` 后：
```
CONVERSION: processing -> switching mode (ascii 0 -> 1)     ← 正确
CONVERSION done: ascii_mode=1                                ← 结果英文（对）
```

## 修复方案

### 1. im-control：使用 TF_GetThreadMgr 获取 per-thread 单例

**修改文件：** `injector/hook.cpp`

用 msctf.dll 导出的 `TF_GetThreadMgr` 替代 `CoCreateInstance(CLSID_TF_ThreadMgr)`，确保获取与 WeaselTSF 相同的 ThreadMgr 实例：

```cpp
typedef HRESULT(WINAPI* PFN_TF_GetThreadMgr)(ITfThreadMgr**);
static ITfThreadMgr* GetThreadMgrSingleton() {
    HMODULE hMsctf = GetModuleHandleW(L"msctf.dll");
    // ... GetProcAddress("TF_GetThreadMgr") ...
    ITfThreadMgr* pThreadMgr = nullptr;
    pfn(&pThreadMgr);
    return pThreadMgr;
}
```

### 2. im-control：使用有效 TfClientId 调用 SetValue

**修改文件：** `injector/hook.cpp`

调用 `ITfThreadMgr::Activate` 获取有效 `TfClientId`，用于所有 `SetValue` 调用，完成后 `Deactivate`：

```cpp
TfClientId clientId = TF_CLIENTID_NULL;
pThreadMgr->Activate(&clientId);
// ... SetValue(clientId, ...) 替代 SetValue(0, ...) ...
pThreadMgr->Deactivate();
```

### 3. Weasel：CONVERSION handler 改为值驱动

**修改文件：** `WeaselTSF/Compartment.cpp`

将 blind toggle 替换为读取 compartment 实际值并与 `_status.ascii_mode` 比对：

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
    // 注意：不调用 _SetKeyboardOpen(true)，避免触发 OPENCLOSE 级联
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

**天然幂等性**：即使 `_updatingLanguageBar` 守卫失效（异步回调 / 系统注入），重新读取 compartment 值会得到与当前 `_status.ascii_mode` 一致的结果，`desiredAsciiMode == _status.ascii_mode` → 跳过。

### 4. Weasel：从 CONVERSION handler 移除 _SetKeyboardOpen(true)

**修改文件：** `WeaselTSF/Compartment.cpp`

CONVERSION handler 不再调用 `_SetKeyboardOpen(true)`。两个 compartment 完全解耦：
- **OPENCLOSE**：管理 IME 启用/禁用（由 OPENCLOSE handler 处理，为 Ctrl+Space 和 Shift 设计）
- **CONVERSION**：管理中/英文模式（由 CONVERSION handler 处理，为外部工具和系统切换设计）

### 5. im-control：OPENCLOSE 跳过未变化的写入

**修改文件：** `injector/hook.cpp`

写入前先 `GetValue` 比对，仅在值变化时 `SetValue`，避免不必要地触发 OPENCLOSE handler。

### 6. vim 插件：去掉 -k open，只写 CONVERSION

**修改文件：** `autoload/im_select.vim`

`im_control_set_mode()` 从 `[im-control, '-k', 'open', '-c', 'native']` 改为 `[im-control, '-c', 'native']`，两个 compartment 完全解耦。

## 设计原则

遵循 Weasel 开发者的指导：

> GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION 和 GUID_COMPARTMENT_KEYBOARD_OPENCLOSE 是不一样的消息，作用要区分。输入功能是主功能，外部消息控制是辅助，不能因为要引入辅助功能导致主功能失效是基本要求。

我们的修改完全符合此原则：
- CONVERSION handler 不再触碰 OPENCLOSE compartment
- OPENCLOSE handler 未改动（Shift/Ctrl+Space 主功能不受影响）
- 外部工具只写 CONVERSION 切换中英文，不写 OPENCLOSE

## 方案对比

| 项目 | blind toggle (`f14f2a7`) | 值驱动（本修复） |
|------|--------------------------|------------------|
| 回调次数依赖 | 严格依赖偶数次 `OnChange` | 幂等，不依赖回调次数 |
| 重入守卫 | 唯一防线，异步回调下失效 | 第一防线；值比对是第二防线 |
| compartment 耦合 | CONVERSION 写 OPENCLOSE（级联反转） | 完全解耦 |
| Windows 10 | 正常 | 正常 |
| Windows 11 | 失效 | 正常 |

## 部署

### 重要：部署后需重启所有使用 RIME 的应用

`weasel.dll` 由 TSF 框架在进程启动时加载。部署新 DLL 后，已运行的进程仍使用内存中的旧 DLL。必须重启 Windows Terminal、Total Commander、gvim 等应用（或重启 Windows）才能加载新版本。

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

### 编译部署 im-control

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

## 相关文档

- [原始修复：CONVERSION handler 响应外部变更](conversion-compartment-fix-analysis.md) — blind toggle 方案（Windows 10）
- [rime/weasel#1371](https://github.com/rime/weasel/issues/1371)
- [im-control](https://github.com/VimWei/im-control)
