# 光标语言栏按钮不刷新修复

## 问题

使用 AppIME / im-control / vim-im-select 外部工具切换 RIME/weasel 输入法中英文状态时，光标位置的语言栏按钮（`_pLangBarButton`）图标不更新——右下角托盘图标和任务栏指示器始终正确，唯独光标附近的语言栏按钮停留在旧状态。

## 三个层级

| 层级 | 位置 | 更新方式 | 修复前状态 |
|------|------|---------|-----------|
| RIME 引擎 | 右下角通知区域托盘图标 | WeaselServer IPC | 始终正确 |
| 任务栏"中"/"A"指示器 | 任务栏系统托盘区 | TSF compartment 渲染 | 始终正确 |
| **光标语言栏按钮** | 光标附近的系统语言栏按钮 | `_pLangBarButton->UpdateWeaselStatus()` | **不更新** |

## 根因

### 1. OPENCLOSE handler else 分支盲 toggle + `_UpdateLanguageBar` 写回 compartment（race）

Win11（`_isToOpenClose=false`）时 OPENCLOSE else 分支盲目翻转 `_status.ascii_mode`，然后调 `_UpdateLanguageBar` 写 CONVERSION compartment。当外部工具（im-control）同时写 compartment 时，两条 OnChange 的到达顺序不确定——若 OPENCLOSE 后到，会覆盖外部写入，产生死锁。

### 2. CONVERSION handler 在 OnChange 回调中写 compartment 失败

`_HandleCompartment(CONVERSION)` 在 `OnChange` 回调内调 `_UpdateLanguageBar` 写 compartment 会触发 TSF `E_UNEXPECTED`，导致 LBB 不刷新。

### 3. `_status` 陈旧导致方向性 force 缺失

用户 gvim insert 中文模式停顿 2s+ 后 Esc，RIME server 端 `ascii_composer` 可能已自动切回 ascii=true，但客户端 `_status` 未通过 `DoEditSession` 更新仍为 false（中文）。此时 im-control 读 compartment 发现值未变跳过 SetValue，无 OnChange 触发，LBB 停留在"中"。

## 修复

### WeaselTSF 端（本 PR）

**`WeaselTSF/Compartment.cpp`**

- **OPENCLOSE if 分支**（Win10 `_isToOpenClose=true`）：调 `_UpdateLanguageBar` 前先从 CONVERSION compartment 同步 `_status`，避免陈旧状态覆盖 compartment
- **OPENCLOSE else 分支**（Win11 `_isToOpenClose=false`）：
  - 删除盲 toggle `_status.ascii_mode = !_status.ascii_mode`
  - 删除 `_HandleLangBarMenuSelect` + `_UpdateLanguageBar`（避免覆盖外部写入）
  - 改为值驱动：读 CONVERSION compartment 同步 `_status`，仅在 mismatch 时刷 LBB
  - **方向性 force**：当 OPENCLOSE 刚被关闭（`keyboardJustClosed`）且 `_status.ascii_mode` 仍是中文时，主动切英文（通知 RIME + 写 compartment + 刷 LBB）。覆盖 gvim Esc 场景
- **CONVERSION handler**：用 `_pLangBarButton->UpdateWeaselStatus(_status)` 替代 `_UpdateLanguageBar`（避免在 OnChange 回调中写 compartment），并调度 `CUpdateLangBarEditSession` 异步刷新

**`WeaselTSF/LanguageBar.cpp`**

- 新增 `_ReconcileCompartment()`：纯本地比对 `_status` 与 compartment，mismatch 时同步并刷 LBB。零 IPC、零广播
- `_UpdateLanguageBar` 加 `_pLangBarButton` 空指针保护

**`WeaselTSF/WeaselTSF.cpp`**

- 新增 `_InitDeferredWindow` / `_UninitDeferredWindow` / `_DeferredWndProc`：message-only 隐藏窗口 + 2s `SetTimer` 周期调 `_ReconcileCompartment`，作为 OnChange 漏检的安全网
- `OnSetThreadFocus`：先 `_ReconcileCompartment` 再从 server 拉权威态 + `_UpdateLanguageBar`

**`WeaselTSF/WeaselTSF.h`** — 新方法/成员声明
**`WeaselTSF/stdafx.h`** — 加 `<strsafe.h>`（`StringCchPrintfW`）

### im-control 端（配套 PR）

`injector/hook.cpp`：OPENCLOSE 重开条件 `g_isToOpenClose && conversionModeNative.has_value() && !keyboardOpenClose`——不再解引用 `*conversionModeNative`（只在 `-c native` 时重开会漏掉 `-c alphanumeric` 场景导致 Win10 gvim RIME 被禁用）。

### VimReader 端（配套修改）

`IME.ahk` 新增 `IME_SetEnglishViaF13()`：`IME_SetAlphanumeric()`（im-control 值驱动切状态）+ `Send {LControl}`（触发前台 WeaselTSF `OnKeyDown` → `DoEditSession` 拉 server 权威态刷 LBB）。LCtrl 不生成 WM_CHAR（不在任何 app 中插入可见字符），不被 RIME `ascii_composer` 处理（无 blind toggle 副作用）。

`AppIME.ahk`：两处触发点改用 `IME_SetEnglishViaF13()`。

## 场景验证

| # | 场景 | 修复点 | 结果 |
|---|------|--------|------|
| 1 | Win10 AppME 闲置 8s 切英文 | VimReader LCtrl keystroke path | ✅ |
| 2 | Win10 gvim insert 英文 → Esc | im-control OPENCLOSE 重开 | ✅ |
| 2b | Win10 gvim insert 中文 → Esc | OPENCLOSE if 分支值驱动 | ✅ |
| 3 | Win11 AppME 闲置 8s 切英文 | VimReader LCtrl keystroke path | ✅ |
| 4 | Win11 gvim Shift 切中 → 快速 Esc | RIME server vim_mode 或方向性 force | ✅ |
| 4b | Win11 gvim Shift 切中 → 2s+ 停顿 → Esc | **OPENCLOSE else 方向性 force（`!_status.ascii_mode`）** | ✅ |
| 5 | Win10/Win11 手动 Shift、Ctrl+Space | 主功能未改动 | ✅ |

## 设计原则

1. **值驱动 > 盲 toggle**：读 compartment / `_status` 决定方向，不盲目翻转
2. **不覆盖外部写入**：OPENCLOSE else 分支不调 `_UpdateLanguageBar` 写 compartment
3. **`_isToOpenClose` 分支隔离**：Win10（if）和 Win11（else）行为独立，互不干扰
4. **方向性 force 仅在 keyboardJustClosed + Chinese**：不破坏 English→Esc、Ctrl+Space 等场景

## 维护踩坑警示

| 陷阱 | 正确做法 |
|------|---------|
| OPENCLOSE else 分支盲 toggle `_status` | 读 compartment 同步 `_status`；仅 `keyboardJustClosed && !_status.ascii_mode` 时方向性 force |
| 在 OnChange 回调中 `_UpdateLanguageBar` 写 compartment | 用 `UpdateWeaselStatus` 替代；或 `PostMessage` 延迟到消息泵 |
| im-control hook `*conversionModeNative` 解引用 | 只检查 `has_value()`，alphanumeric 也需要重开 OPENCLOSE |
| AppME 用 Send Shift 切英文 | Shift 是 RIME blind toggle，改用 im-control + LCtrl keystroke |
| AppME 用 Send F13 切英文 | F13 在 gvim insert mode 显示 `<F13>` 字面文本，改用 LCtrl |
| 删 2s `_ReconcileCompartment` 定时器 | 该定时器是 Win11 gvim OnChange 漏检的安全网，不可删 |
| 进程内实例广播解决跨进程 | 跨进程需走 WeaselServer IPC 或 keystroke path，PostMessage 只在同进程有效 |

## 相关文档

- `docs/compartment-external-control-fix.md` — 历史 compartment 外部控制修复
- `docs/archives/cursor-indicator-debug.md` — 完整调试过程记录（v1-v10 演进）
- `docs/archives/status-icon-debug.md` — 早期状态图标探索（术语混淆，保留原始状态）
