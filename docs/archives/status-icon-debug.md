# WeaselTSF 状态图标调试记录

## 问题描述

使用 im-control 控制 RIME/weasel 输入法的中英文状态时，光标所在位置的图标(即语言栏按钮`_pLangBarButton`)显示不正确。

### 涉及场景

1. **AppIME**（VimReader/Windows Terminal）：自动切换中英文后，语言栏按钮显示"中"而非"A"（Win10/Win11 均出错）
2. **gvim**（Win11）：i/esc 切换模式后，OS 指示器时而正确时而错误
3. **gvim**（Win10）：始终正常

### 判断标准：哪个图标？

存在三个层级的状态图标，必须区分：

| 层级 | 位置 | 控制者 | 状态 |
|------|------|--------|------|
| **RIME 引擎** | 右下角通知区域托盘图标 | WeaselServer 读取 RIME 内部状态 | ✅ 始终正确 |
| **TSF Compartment** | 语言栏按钮 | `GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION` 的 `TF_CONVERSIONMODE_NATIVE` 位 | ❌ AppIME 场景错误 |
| **WeaselPanel** | 光标位置候选窗口上的状态图标 | WeaselTSF UI 组件 | 已被移除（不再自行弹图标） |

> 注意：WeaselServer 托盘图标显示的是 RIME 引擎状态，OS 指示器显示的是 TSF compartment 状态，两者可能不一致。

## 相关仓库

- `c:\Apps\git-kb\repos\VimWei\im-control\` - 输入法控制工具
- `c:\Apps\git-kb\repos\VimWei\weasel\` - 小狼毫输入法
- `c:\Apps\VimReader\lib\system\AppIME.ahk` - AppIME 自动切换
- `c:\Apps\vim-init\pack\mydev\opt\vim-im-select\` - vim 输入法切换插件

## 架构分析

### 手动 Shift 切换（正常工作的路径）

```
用户按 Shift
  → OnKeyDown → ProcessKeyEvent → 发送 WEASEL_IPC_PROCESS_KEY_EVENT 到服务器
  → RIME 切换 ascii_mode，发送响应
  → DoEditSession (ITfEditSession)
    → m_client.GetResponseData() 读取响应，更新 _status
    → _UpdateLanguageBar(_status)
      → _SetCompartmentDWORD(flags, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION) ✅ 成功
      → _pLangBarButton->UpdateWeaselStatus(stat)
  → OS 指示器更新 ✅
```

关键点：`_UpdateLanguageBar` 在 **ITfEditSession::DoEditSession** 中执行，不在 OnChange 回调内，所以 `SetValue` 成功。

### AppIME 切换（出错的路径）

```
AppIME 切换英文模式
  → 调用 WeaselServer.exe /ascii
    → 发送 TrayCommand(ENABLE_ASCII) 到服务器
    → RIME 切换 ascii_mode，发送响应给 AppIME（WeaselTSF 不收到）
  → 可能写入 TSF compartment（是否写入取决于 AppIME 的具体实现）
    → 如果写入：
      → OnChange 回调 (_HandleCompartment)
        → _SetCompartmentDWORD() ❌ E_UNEXPECTED（TSF 禁止在回调内写同一 compartment）
        → PostMessage 延迟写入（依赖 app 消息泵）
    → 如果不写入：
      → OnChange 不触发
      → WeaselTSF 完全不知情
      → OS 指示器保持旧值
```

## 关键发现

### 1. TSF OnChange 回调内不能写本 compartment

`ITfCompartment::SetValue()` 在 OnChange 回调（`ITfCompartmentEventSink::OnChange`）中调用时返回 `hr=0x8000FFFF (E_UNEXPECTED)`。这是 TSF 的固有限制——不允许在 change notification 处理过程中再次修改同一 compartment。

所有场景的失败根因都是 `_UpdateLanguageBar` 在 OnChange 内调用时，compartment 写入失败。

### 2. `_updatingLanguageBar` 防重入保护

```cpp
void WeaselTSF::_UpdateLanguageBar(weasel::Status stat) {
  // ... 计算 flags ...
  _updatingLanguageBar = true;
  _SetCompartmentDWORD(flags, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION);
  _updatingLanguageBar = false;
  // ...
}

HRESULT WeaselTSF::_HandleCompartment(REFGUID guidCompartment) {
  if (IsEqualGUID(guidCompartment, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)) {
    if (_updatingLanguageBar) {
      return S_OK;  // 忽略自己写入触发的 OnChange
    }
    // ...
  }
}
```

此保护防止写入触发再次写入的无限循环。

### 3. `_pLangBarButton` 判空导致 Win11 gvim 问题

原 `_UpdateLanguageBar` 末尾：
```cpp
if (_pLangBarButton)
    _pLangBarButton->UpdateWeaselStatus(stat);
```

Windows 11 某些应用中 `_pLangBarButton` 为 NULL（TSF 未创建语言栏按钮），导致按钮更新被跳过。**移除该判空后，gvim 在 Win11 上恢复正确。**

但这本质上是绕过：在语言栏按钮不存在的系统中，OS 指示器可能依赖 compartment 值而非按钮图标。

### 4. `UI::Create` 非幂等导致候选窗口冻结

`UI::Create` 中直接 `new CWnd` 没有 `IsWindow()` 检查。当 `_HandleCompartment` 中调用 `_cand->UpdateUI()` 时可能重复创建窗口，导致 WndProc 覆盖和死锁。

修复：在 `Create()` 开头加 `if (IsWindow()) return true;`。

### 5. 尝试过的修复及其效果

| 修复 | 文件 | 效果 |
|------|------|------|
| `_HandleCompartment` 中调用 `_cand->UpdateUI()` | Compartment.cpp | 引入回归：候选窗口不关闭 |
| `UI::Create` 加 `IsWindow()` 检查 | WeaselUI.cpp | 修复候选窗口冻结 |
| `_HandleCompartment` 中移除 `_cand->UpdateUI()` + 改用 `_cand->RefreshStatus()` | Compartment.cpp | Win10 出现两个图标（我们的 Panel + OS 指示器）；Win11 只有 OS 指示器但仍错误 |
| `_UpdateLanguageBar` 中移除 `_pLangBarButton` 判空 | LanguageBar.cpp | 修复 gvim Win11（焦点变化路径）；AppIME 仍无效 |
| 从 CONVERSION handler 移除 `_UpdateLanguageBar`（因为 E_UNEXPECTED），保留 `UpdateWeaselStatus` | Compartment.cpp | 无变化（_UpdateLanguageBar 之前就失败了） |
| `_HandleCompartment` 内 `PostMessage` 延迟调用 `_UpdateLanguageBar` | Compartment.cpp, WeaselTSF.h/cpp | 理论上应该在回调外写入；实际因 app 消息泵不定时，结果不可靠 |
| `_HandleCompartment` 内移除 `RefreshStatus`（不再自行弹图标）| Compartment.cpp | 去掉冗余 panel 图标 |

### 6. 最终状态

```
手动 Shift 切换:  key → ProcessKeyEvent → DoEditSession  ✅ 始终正常
gvim Win10:       focus change → DoEditSession            ✅ 正常
gvim Win11:       focus change → DoEditSession            ⚠️ 时而正确（与 app 消息泵有关）
AppIME 切换:      仅 TrayCommand → 不触发 WeaselTSF       ❌ OS 指示器始终"中"
```

## gvim Win10/Win11 差异分析

### 路径 A：gvim 直接写 TSF compartment→OnChange

gvim 切换模式时（通过 vim-im-select 或 imm32 TSF API），可能直接写入 `GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION`：

```
gvim ESC/i
  → 写 TSF compartment (设置 ascii_mode)
  → OnChange 触发 _HandleCompartment(CONVERSION)
  → 更新 _status、_HandleLangBarMenuSelect、UpdateWeaselStatus
  → (当前：PostMessage 延迟 _UpdateLanguageBar)
```

| 平台 | 行为 |
|------|------|
| Win10 | OS 指示器直接从 compartment 读取 → gvim 写入后立即正确 ✅ |
| Win11 | OS 指示器可能依赖按钮 OnUpdate 而非 compartment → 延迟更新不及时则错误 ❌ |

### 路径 B：gvim 触发焦点变化→OnSetThreadFocus

gvim 失焦/聚焦时，TSF 调用 `OnSetThreadFocus`。当前代码：

```cpp
STDMETHODIMP WeaselTSF::OnSetThreadFocus() {
  // ...
  if (m_client.Echo()) {                           // ← 条件 1
    m_client.ProcessKeyEvent(0);
    weasel::ResponseParser parser(..., &_status, ...);
    bool ok = m_client.GetResponseData(parser);    // ← 条件 2
    if (ok)
      _UpdateLanguageBar(_status);
  }
  // 如果以上任一条件失败 → _UpdateLanguageBar 被跳过
  return S_OK;
}
```

| 平台 | Echo 稳定性 | GetResponseData 稳定性 | 结果 |
|------|-------------|----------------------|------|
| Win10 | ✅ 始终成功 | ✅ 始终成功 | `_UpdateLanguageBar` 始终执行 → OS 正确 |
| Win11 | ⚠️ 偶发失败 | ⚠️ 偶发失败 | `_UpdateLanguageBar` 被跳过 → "时而正确" |

**Win11 "时而正确"的根因**：`m_client.Echo()` 或 `m_client.GetResponseData()` 偶发失败 → `_UpdateLanguageBar` 被跳过 → compartment 未写入 → OS 指示器不更新。

### 差异总结

| 场景 | Win10 | Win11 |
|------|-------|-------|
| gvim 直接写 compartment（路径 A） | ✅ OS 读 compartment | ❌ 依赖按钮更新（OnChange 内可能被忽略） |
| gvim 焦点变化（路径 B） | ✅ Echo 稳定 | ⚠️ Echo/GetResponseData 偶发失败 |
| 手动 Shift 切换 | ✅ DoEditSession | ✅ DoEditSession |
| AppIME 切换 | ❌ 不写 compartment，OnChange 不触发 | ❌ 同上 |

## 解决方向（更新版）

### 方向 A：im-control 写 TSF compartment（关键是一致性）

调研发现 im-control 已经通过 DLL 注入 + `SetWindowsHookEx` 在目标进程内直接调用 `ITfCompartment::SetValue(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION, ...)`，**不调用 `WeaselServer.exe /ascii`**。

所以 OnChange 应该已经触发了。如果 Steps 1+2 后 OS 指示器仍不正确，问题可能是：
1. **im-control 写入的 compartment 值与 RIME 引擎状态不一致**（写入 compartment 但没通知 RIME）
2. **TSF 的 `SetValue` 在 hook 上下文中触发 OnChange 不可靠**
3. **`TfClientId` 不正确导致 compartment 写入不生效**

**可能的修复路径**：
- 在 im-control 写完 compartment 后额外调用 `WeaselServer.exe /ascii` 同步 RIME（补偿写入路径）
- 或者反过来：保留 `WeaselServer.exe /ascii` 调用，去掉 im-control 的 compartment 写入，让 WeaselTSF 的 Steps 1+2 来写 compartment

### 方向 B：轮询 —— 已放弃

切换后轮询到新状态时，OS 指示器已经显示错误图标了，没有实际意义。

### 方向 C：RequestEditSession 替代 PostMessage（有效）

在 `_HandleCompartment` 中如果当前有焦点上下文，使用 `ITfContext::RequestEditSession` 异步执行 `_UpdateLanguageBar`。TSF 保证 EditSession 最终执行，不依赖 app 消息泵。如果拿不到上下文（如控制台应用），再 fallback 到 PostMessage。

### 方向 D（新）：无条件 OnSetThreadFocus

修复 `OnSetThreadFocus` 中的条件跳过问题——无论 Echo/GetResponseData 是否成功，都调用 `_UpdateLanguageBar`：

```cpp
STDMETHODIMP WeaselTSF::OnSetThreadFocus() {
  // ...
  if (m_client.Echo()) {
    m_client.ProcessKeyEvent(0);
    weasel::ResponseParser parser(..., &_status, ...);
    m_client.GetResponseData(parser);         // 不检查返回值
  }
  _UpdateLanguageBar(_status);                // 始终执行
  return S_OK;
}
```

## 下一步计划

### Step 1：无条件 OnSetThreadFocus（方向 D）

修复 `WeaselTSF.cpp` 中 `OnSetThreadFocus`，无条件调用 `_UpdateLanguageBar`。

**目标**：修复 gvim Win11 "时而正确"问题。

### Step 2：RequestEditSession 替代 PostMessage（方向 C）

在 `_HandleCompartment` 中：
- 优先 `_pThreadMgr->GetFocus` → 拿 ITfContext → `RequestEditSession` → `DoEditSession` → `_UpdateLanguageBar`
- 拿不到上下文时 fallback 到 `PostMessage`

**目标**：确保 AppIME 写 compartment 后（方向 A 实现时），OnChange 内触发的 compartment 写入可靠执行。

### Step 3：im-control 同步调优（方向 A）

先测试 Steps 1+2 的效果：如果 im-control 的 compartment 写入已经能触发正确的 OS 指示器更新，则 Step 3 只需确保 RIME 引擎与 compartment 一致。

如果 OS 指示器仍然错误，两种修法：
- **修 im-control**：写入 compartment 后额外调用 `WeaselServer.exe /ascii` 同步 RIME
- **修 WeaselTSF**：在 `_HandleCompartment` 中依赖 RIME 响应来确认写入（Steps 1+2 已实现）

在 `C:\Apps\git-kb\repos\VimWei\im-control\injector\hook.cpp` 中确认 compartment 写入逻辑的 `TfClientId` 和 `TF_GetThreadMgr` 用法是否正确。

### 预期效果

| 场景 | Step 1+2 后（当前） | Step 3 后 |
|------|-------------------|-----------|
| gvim Win10 | ✅ | ✅ |
| gvim Win11 | ✅ 不再"时而正确" | ✅ |
| AppIME Win10 | ❓ 待测试（im-control 已写 compartment，但同步可能有问题） | ✅ |
| AppIME Win11 | ❓ 待测试（同上） | ✅ |

## 测试记录

### 2025-07-12 版本

OS 指示器（任务栏"中"/"A"）的判定逻辑：
- Win10 任务栏输入法指示器：可能读取 TSF compartment 值
- Win11 任务栏输入法指示器：可能读取 IME 语言栏按钮图标 + compartment

| 场景 | Win10 | Win11 |
|------|-------|-------|
| gvim 模式切换 | ✅ | ⚠️ 有时正确 |
| AppIME 模式切换 | ❌ "中" | ❌ "中" |
| 手动 Shift 切换 | ✅ | ✅ |

## 编译部署

```powershell
cd C:\Apps\git-kb\repos\VimWei\weasel
build.bat weasel release

# 部署（管理员权限）
Rename-Item C:\Windows\system32\weasel.dll weasel.dll.bak -Force
Copy-Item output\weaselx64.dll C:\Windows\system32\weasel.dll -Force
Rename-Item C:\Windows\SysWOW64\weasel.dll weasel.dll.bak -Force
Copy-Item output\weasel.dll C:\Windows\SysWOW64\weasel.dll -Force

# 重启使用 RIME 的应用
```
