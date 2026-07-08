# 修复分析：WeaselTSF 响应 TSF 转换模式 Compartment 外部变更

## 问题

通过 TSF compartment 程序化切换中英文模式（如 `ITfCompartment::SetValue(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)`）对小狼毫无效。外部工具写入 compartment 后，小狼毫不切换模式，且 compartment 值被恢复原状。

## 行为变化

| 操作 | 修复前 | 修复后 |
|------|--------|--------|
| 通过 TSF compartment 设置转换模式 | 无效果（外部变更被撤销） | 正常切换中英文 |
| 键盘关闭时设置转换模式 | 不处理 | 自动打开键盘并切换 |
| Shift 按键切换 | 正常 | 正常（不变） |
| 托盘菜单切换 | 正常 | 正常（不变） |

## 架构背景

WeaselTSF 是 TSF Text Service（实现 `ITfTextInputProcessorEx`），不接收 `WM_IME_CONTROL`。Windows TSF 框架负责在 IMM32 API 与 TSF compartments 之间桥接：

```
ImmSetConversionStatus(IME_CMODE_NATIVE)
    → TSF framework 翻译
        → ITfCompartment::SetValue(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)
            → _HandleCompartment()  ← 修复位置
```

## 根因

根因有两个层面：**源代码逻辑缺陷** 和 **DLL 部署位置**。两者必须同时解决，缺一不可。

### 1. 源代码：CONVERSION handler 撤销外部变更

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

### 2. DLL 部署位置

WeaselTSF 的 CLSID `{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}` 注册路径为：

| 架构 | 注册路径 |
|------|---------|
| 64-bit | `C:\Windows\system32\weasel.dll` |
| 32-bit | `C:\Windows\SysWOW64\weasel.dll` |

TSF 框架从上述系统路径加载 DLL。如果将修复后的 DLL 部署到其他路径（如 `C:\Program Files\Rime\weasel-0.17.4\`），系统不会加载它，进程内仍是旧 DLL。

更新方法：管理员身份打开 PowerShell，执行

Rename-Item "C:\Windows\system32\weasel.dll" "weasel.dll.bak3" -Force
Copy-Item "C:\Apps\git-kb\repos\VimWei\weasel\output\weaselx64.dll" "C:\Windows\system32\weasel.dll" -Force
Rename-Item "C:\Windows\SysWOW64\weasel.dll" "weasel.dll.bak3" -Force
Copy-Item "C:\Apps\git-kb\repos\VimWei\weasel\output\weasel.dll" "C:\Windows\SysWOW64\weasel.dll" -Force

## 修复方案

### 源代码修改

**修改文件：** `WeaselTSF/Compartment.cpp`、`WeaselTSF/LanguageBar.cpp`、`WeaselTSF/WeaselTSF.h`

将 CONVERSION handler 从"查询后端 → 写回 compartment"改为"toggle → 通知 RIME → 同步 UI"，与 `GUID_COMPARTMENT_KEYBOARD_OPENCLOSE` 的 else 分支（Shift 按键切换路径）保持一致：

```cpp
// 修复后
} else if (IsEqualGUID(guidCompartment,
                       GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)) {
  if (_updatingLanguageBar)
    return S_OK;
  _status.ascii_mode = !_status.ascii_mode;
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
```

与原始代码的关键差异：

| 项目 | 原始代码 | 修复后 |
|------|---------|--------|
| **响应方式** | `m_client.GetResponseData()` 查询 RIME 后端旧状态，`_UpdateLanguageBar()` 写回 compartment（撤销外部变更） | `_status.ascii_mode = !_status.ascii_mode` 直接翻转，`_HandleLangBarMenuSelect()` 通知 RIME 引擎切换 |
| **键盘状态守卫** | `_IsKeyboardOpen()` —— 键盘关闭时完全不处理 | 移除守卫，改为 `_SetKeyboardOpen(true)` 主动打开键盘 |
| **重入保护** | 无 —— `_UpdateLanguageBar` 写 compartment 再次触发 handler | `_updatingLanguageBar` 守卫，阻断 `_UpdateLanguageBar` 引起的自触发 |

`_HandleLangBarMenuSelect` → `TrayCommand` IPC → RIME 后端的 `SetOption("ascii_mode")`，与 Shift 按键切换走同一路径，确保行为和配置（如 `global_ascii_mode`）一致。

### 重入守卫

`_UpdateLanguageBar` 内部调用 `_SetCompartmentDWORD` 写入 CONVERSION compartment，这会再次触发 `OnChange` → `_HandleCompartment`。如果不加守卫，handler 会再次 toggle，导致双重翻转或无限递归。

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

### 为什么用 blind toggle 而非读取 compartment 值

曾尝试读取 compartment 值来推导目标模式（`_GetCompartmentDWORD` → 检查 `TF_CONVERSIONMODE_NATIVE` 位 → 与 `_status.ascii_mode` 比对）。但 TSF 的 `OnChange` 通知时机可能导致 `GetValue` 返回旧值。blind toggle 避免了这一时序问题，且与 `OPENCLOSE` handler 的 else 分支（Shift 按键路径）完全一致。

外部工具（如 im-control）只在 `newMode != oldMode` 时才调用 `SetValue`，所以每次 `OnChange` 通知都代表一次真实的模式切换请求，blind toggle 语义正确。

## 验证

### 测试环境

- 前台进程：Windows Terminal (x64)
- 外部工具：[im-control](https://github.com/VimWei/im-control) —— 通过 `SetWindowsHookEx` 注入 hook DLL 到前台进程，在目标线程内调用 `ITfCompartment::SetValue`

### 测试结果

```
# 中文状态下切英文
im-control -g              → open native
im-control -c alphanumeric → (成功)
im-control -g              → open alphanumeric

# 英文状态下切中文
im-control -g              → open alphanumeric
im-control -c native       → (成功)
im-control -g              → open native
```

### 调试日志验证

在 `_HandleCompartment` 和 `_UpdateLanguageBar` 中加入 `OutputDebugStringW` 日志，确认完整链路：

```
[05:38:09.417] CONVERSION fired, compartment=0xF49BE320 (NATIVE=0), ascii_mode(before)=0, _updatingLanguageBar=0
[05:38:09.417] CONVERSION toggled ascii_mode: 0 -> 1
[05:38:09.417] CONVERSION called HandleLangBarMenuSelect(40013)    # ID_WEASELTRAY_ENABLE_ASCII
[05:38:09.417] CONVERSION calling _UpdateLanguageBar
[05:38:09.417] CONVERSION done, ascii_mode(after)=1
[05:38:09.433] CONVERSION fired, compartment=0xF49BE320 (NATIVE=0), ascii_mode(before)=1, _updatingLanguageBar=1
[05:38:09.433] CONVERSION skipped (re-entrant from _UpdateLanguageBar)   # 重入守卫生效
```

链路完整：hook 写入 compartment → sink 触发 → toggle ascii_mode → 通知 RIME 引擎 → `_UpdateLanguageBar` 写回 compartment → 重入守卫跳过自触发。

## 相关链接

- [rime/weasel#1371](https://github.com/rime/weasel/issues/1371)
- [im-control](https://github.com/VimWei/im-control) —— 用于测试的 TSF compartment 控制工具
