<a name="0.16.1"></a>
## [0.16.1](https://github.com/rime/weasel/compare/0.16.0...0.16.1) (2024-06-06)


#### 安裝須知

**⚠️如您由0.16.0之前的版本升級，由於參數變化，安裝小狼毫前請保存好文件資料，於安裝後重啓或註銷 Windows，否則正在使用小狼毫的應用可能會崩潰。**

**⚠如您由0.16.0之前的版本升級，請確認您的 `installation.yaml` 文件編碼爲 `UTF-8`, 否則如您在其中修改了非 ASCII 字符內容的路徑時，有可能會引起未明錯誤。**

#### 主要更新
* 爲`WeaselServer.exe`使能Windows Error Reporting, 提供對應的`WeaselServer.pdb`文件, 在`WeaselServer.exe`崩潰時可以生成dmp報告文件在日誌文件夾中
* 提供`WeaselServer.exe`守護，在服務崩潰後6個按鍵事件（三次擊鍵Down&Up)後拉起服務
* 新增英文界面語言
* 更新7z和curl到最新版本，修復一些因爲7z的bug引起的問題
* 優化預覽圖PNG文件大小
* 新增語言欄菜單，打開日誌文件夾，調整日誌文件夾路徑爲`%TEMP%\rime.weasel`,方便查閱管理
* 異步處理消息，避免服務崩潰時長時間未響應引起客戶端程序崩潰
* 不在服務中部署方案，避免在守護拉起服務進入長耗時部署引起的僵死問題

#### Bug 修復

* 修復自動折行未正確處理標點符號（標點在折行後最前）的問題
* 修復`vim-mode`下的typo引起的`<C-C>`無法生效問題
* 修復部署消息未更新問題
* 修復卸載小狼毫時意外安裝語言包問題
* 修復`semi_hilite`下的UI未正確響應問題, `semi_hilite`顏色調整爲高亮色的半透明度狀態，改善體驗
* 減少不必要的服務端UI更新，提高性能減少服務崩潰機率
* 修復在非`DPI=96`的副屏上響應慢的問題
* 修復在高分屏上layout參數未dpi aware問題
* 修復Windows 11下Chrome等瀏覽器中非激活光標狀態下的按鍵響應異常問題
* 修復64位系統下默認安裝路徑不準確的問題



<a name="0.16.0"></a>
## [0.16.0](https://github.com/rime/weasel/compare/0.15.0...0.16.0) (2024-05-14)


#### 安裝須知

**⚠️由於參數變化，安裝小狼毫前請保存好文件資料，於安裝後重啓或註銷 Windows，否則正在使用小狼毫的應用可能會崩潰。**

**⚠請確認您的 `installation.yaml` 文件編碼爲 `UTF-8`, 否則如您在其中修改了非 ASCII 字符內容的路徑時，有可能會引起未明錯誤。**

#### 主要更新

* 升級核心算法庫至 [librime 1.11.2](https://github.com/rime/librime/releases/tag/1.11.2)
* 改善輸入法病毒誤報問題
* 新增 64 位算法服務程序，支持 64 位 librime，支持大內存（可部署大規模詞庫方案）
* 支持 arm/arm64 架構
* 單安裝包支持 win32/x64/arm/arm64 架構系統的自動釋放文件
* 32 位算法服務增加 LARGE ADDRESS AWARE 支持
* 升級 boost 算法庫至 1.84.0
* IME改爲可選項，默認不安裝
* 棄用 `weaselt*.dll`，增加註冊香港、澳門、新加坡區域語言配置（默認未啓用，需在控制面板/設置中手工添加）；支持簡繁體小狼毫同時使能
* 棄用 `weaselt*.ime`
* 移除 `pyweasel`
* 候選窗口 UI 內存優化
* 改善候選窗口 UI 繪製性能
* 升級 WTL 庫至 10.0，gdi+ 至 1.1
* 每顯示器 dpi aware，自適應不同顯示器不同 dpi 設定變化
* 更新高清圖標
* 增大 IPC 數據長度限制至 64k，支持長候選
* 升級 plum
* 應用界面及菜單簡繁體自動適應
* `app_options` 中應用名大小寫不敏感
* 字體抗鋸齒設定參數 `style/antialias_mode: {force_dword|cleartype|grayscale|aliased|default}`
* ASCII狀態提示跟隨鼠標光標設定 `style/ascii_tip_follow_cursor: bool`
* 新增參數 `style/layout/hilite_padding_x: int`、`style/layout/hilite_padding_y: int`，支持分別設置xy向的 padding
* 新增參數 `schema/full_icon: string`, `schema/half_icon: string`，支持在方案中設定全半角圖標
* 新增參數 `style/text_orientation: "horizontal" | "vertical"`, 與 `style/vertical_text: bool` 冗餘，設定文字繪製方向，兼容 squirrel 參數
* 新增參數 `style/paging_on_scroll: bool`，可設定滾輪相應類型（翻頁或切換前後候選）
* 新增參數，Windows10 1809後版本的Windows，支持 `style/color_scheme_dark: string` 設定暗色模式配色
* 新增參數 `style/candidate_abbreviate_length: int`，支持候選字數超限時縮略顯示
* 新增參數 `style/click_to_capture: bool` 設定鼠標點擊是否截圖
* 新增參數 `show_notifications_time: int` 可設定提示顯示時間，單位 ms；設置 0 時不顯示提示
* 新增參數 `show_notifications: bool` 或 `show_notifications: 開關列表 | "schema"`，可定製是否顯示切換提示、顯示那些切換提示
* 新增參數 `style/layout/baseline: int` 和 `style/layout/linespacing: int`，可自行調整參數修復候選窗高度跳躍閃爍問題
* 棄用 `style/mouse_hover_ms`；新增 `style/hover_type: "none"|"semi_hilite"|"hilite"`，改善鼠標懸停相應體驗
* 新增參數 `global_ascii: bool`, 支持全局 ascii 模式
* 新增 `app_options`，支持應用專用 `vim_mode: bool`，支持常見 vim 切換 normal 模式按鍵時，切換到 `ascii_mode`
* 新增 `app_options`，支持應用專用 `inline_preedit: bool` 設定，優先級高於方案內設定，高於 `weasel.yaml` 中的設定
* 支持命令行設置小狼毫 `ascii_mode` 狀態，`WeaselServer.exe /ascii`，`WeaselServer.exe /nascii`
* 支持設置 `comment_text_color`、`hilited_comment_text_color` 透明來隱藏對應文字顯示
* `hilited_mark_color` 非透明，`mark_text` 爲空字符串時，類 windowns 11 的高亮標識
* 切換方案後，提示方案圖標和方案名字
* 支持全部 switch 提示使用方案內設定的 label
* WeaselSetup通過打開目錄窗口設置用戶目錄路徑
* 新增支持方案內定義方案專用配色
* 支持 imtip
* 增加類微軟拼音的高亮標識在鼠標點擊時的動態
* 支持在字體設定任一分組中設置字體整體的字重或字形
* 優化點擊選字邏輯
* 豎直佈局反轉時，互換上下方向鍵
* 候選窗超出下方邊界時，在當前合成結束前保持在輸入位置上方，減少候選窗口高度變小時潛在的窗口上下跳動
* 調整 TSF 光標位置（`inline_preedit: false` 時），減少光標閃爍
* WeaselSetup 修改用戶目錄路徑（已安裝時）
* 語言欄新增菜單，重啓服務
* IPC 報文轉義 `\n`、`\t`，不再因 `\n` 引發應用崩潰
* 使用 clang-format 格式化代碼，統一代碼風格
* 自動文件版本信息
* 測試項目 test 只在 debug 配置狀態下編譯構建

#### Bug 修復

* 修復 word 365 中候選窗閃爍無法正常顯示的問題
* 修復 word 行尾輸入時候選窗反覆跳動問題
* 修復 word 中無法點擊選詞問題
* 修復 excel 等應用中，第一鍵 keydown 時未及時彈出候選窗問題
* 修復導出詞典數據後引起的多個 explorer 進程的問題，優化對應對話框界面顯示
* 修復打開用戶目錄，程序目錄引起的多個 explorer.exe 進程問題，支持服務未啓動時打開這些目錄
* 修復系統托盤重啓後未及時顯示的問題
* 修復 `style/layout/min_width` 在部分佈局下未生效問題
* 修復 preedit 寬高計算錯誤問題
* 修復翻頁按鈕在豎直佈局反轉時位置錯誤
* 修復豎直佈局帶非空 mark_text 時的計算錯誤
* 修復 composing 中候選窗隨文字移動問題
* 修復 wezterm gpu 模式下無法使用問題
* 修復 `style/inline_preedit: true` 時第一鍵輸入時候選窗位置錯誤
* 修復算法服務單例運行
* 修復調用 WeaselServer.exe 未正常重啓服務問題
* 修復偶發的顯卡關聯文字空白問題
* 修復部署過程中如按鍵輸入引發的重複發出 tip 提示窗問題
* 修復部分方案中的圖標顯示（`english.schema.yaml`）
* 修復 `preedit_type: preview` 時的光標錯誤問題
* 修復 `shadow_color` 透明時截圖尺寸過大問題，減小截圖尺寸
* 修復天園地方時，高亮候選圓角半徑不正確問題
* 修復某些狀態下天園地方的 preedit 背景色圓角異常問題
* 修復候選尾部空白字符引起的佈局計算錯誤問題
* 修復 mark_text 繪製鋸齒問題
* 修復靜默安裝彈窗問題
* 修復 librime-preedit 引起的應用崩潰問題
* 修復 plum 用戶目錄識別錯誤問題
* 修復安裝後未在控制面板中添加輸入法、卸載後未刪除控制面板中的輸入法清單問題
* 修復一些其他已知的 bug

#### 已知問題

* 部分應用仍存在輸入法無法輸入文字或響應異常的問題
* WeaselServer 仍可能發生崩潰
* 仍有極少部分防病毒軟件可能誤報病毒



<a name="0.15.0"></a>
## [0.15.0](https://github.com/rime/weasel/compare/0.14.3...0.15.0) (2023-06-06)


#### 安裝須知

**⚠️安裝小狼毫前請保存好文件資料，於安裝後重啓 Windows ，否則正在使用小狼毫的應用將會崩潰。**
**⚠️此版本的小狼毫需要使用 Windows 8.1 或更高版本的操作系統。**

#### 主要更新

* 升級核心算法庫至 [librime 1.8.5](https://github.com/rime/librime/blob/master/CHANGELOG.md#185-2023-02-05)
* DPI 根據顯示器自動調整
* 支持候選窗口等圓角顯示
  * `style/layout/corner_radius: int`
* 兼容鼠鬚管中高亮圓角參數`style/layout/hilited_corner_radius: int`
* 支持主題顏色中含有透明通道代碼, 支持格式 0xaabbggrr，0xbbggrr, 0xabgr, 0xbgr
* 配色主題支持默認ABGR順序，或ARGB、RGBA順序
  * `preset_color_schemes/color_scheme/color_format: "argb" | "rgba" | ""`
* 支持編碼/高亮候選/普通候選/輸入窗口/候選邊框的陰影顏色繪製
  * `style/layout/shadow_radius: int`
  * `style/layout/shadow_offset_x: int`
  * `style/layout/shadow_offset_y: int`
  * `preset_color_schemes/color_scheme/shadow_color: color`
  * `preset_color_schemes/color_scheme/nextpage_color: color`
  * `preset_color_schemes/color_scheme/prevpage_color: color`
  * `preset_color_schemes/color_scheme/candidate_back_color: color`
  * `preset_color_schemes/color_scheme/candidate_shadow_color: color`
  * `preset_color_schemes/color_scheme/candidate_border_color: color`
  * `preset_color_schemes/color_scheme/hilited_shadow_color: color`
  * `preset_color_schemes/color_scheme/hilited_candidate_shadow_color: color`
  * `preset_color_schemes/color_scheme/hilited_candidate_border_color: color`
  * `preset_color_schemes/color_scheme/hilited_mark_color: color`
* 支持自定義標籤、註解字體及字號
  * `style/label_font_face: string`
  * `style/comment_font_face: string`
  * `style/label_font_point: int`
  * `style/comment_font_point: int`
  * `style/layout/align_type: "top" | "center" | "bottom"`
* 支持指定字符 Unicode 區間字體設定
* 支持字重，字形風格設定
  * `style/font_face: font_name[:start_code_point:end_code_point][:weight_set][:style_set][,font2...]`
    * example: `"Segoe UI Emoji:20:39:bold:italic, Segoe UI Emoji:1f51f:1f51f, Noto Color Emoji SVG:80, Arial:600:6ff, Segoe UI Emoji:80, LXGW Wenkai Narrow"`
* 支持自定义字体回退範圍、順序定义
* 彩色字體支持
  * Windows 10 周年版前：需要使用 COLR 格式彩色字體
  * Windows 11 ：可以使用 SVG 字體
* 新增豎直文字佈局
  * `style/vertical_text: bool`
  * `style/vertical_text_left_to_right: bool`
  * `style/vertical_text_with_wrap: bool`
* 新增豎直佈局vertical窗口上移時自動倒序排列
  * `style/vertical_auto_reverse: bool`
* 新增「天圓地方」佈局：由 margin 與 hilite_padding 確定, 當margin <= hilite_padding時生效
* margin_x 或 margin_y 設置爲負值時，隱藏輸入窗口，不影響方案選單顯示
* 新增 preedit_type: preview_all ，在輸入時將候選項顯示於 composition 中
  * `style/preedit_type: "composition" | "preview" | "preview_all"`
* 新增輸入法高亮提示標記
  * `style/mark_text: string`
* 新增輸入方案圖標顯示，可在語言欄中顯示，文件格式爲ico
  * `schema/icon: string`
  * `schema/ascii_icon: string`
* 新增選項，允許在光標位置獲取失敗時於窗口左上角繪製候選框（而不是桌面左上角）
  * `style/layout/enhanced_position: bool`
* 新增鼠標點擊截圖到剪貼板功能
* 新增選項，支持越長自動折行/換列顯示
  * `style/layout/max_width: int`
  * `style/layout/max_height: int`
* 支持方案內設定配色
  * `style/color_scheme: string`
* 支持多行内容顯示，\r, \n, \r\n均支持
* 支持方案內設定配色
* 繪製性能提升
* composition 模式下新增下劃線顯示
* 隨二進制文件提供調試符號

#### Bug 修復

* 轉義日文鍵盤中特殊按鍵
* 候選文字過長時崩潰
* 修復用戶目錄下無 `default.custom.yaml` 或 `weasel.custom.yaml` 時，設定窗口無法彈出的問題
* 方案中設定inline_preedit爲true時，部署後編碼末端出現異常符號
* 部分應用無法輸入文字的問題
* 修復部署時無顯示提示的問題
* 修復中文路徑相關問題
* 修復右鍵菜單打開程序目錄/用戶目錄時，資源管理器無響應的問題
* 修復部分內存訪問問題
* 修復操作系統 / WinGet 無法識別小狼毫版本號的問題
* 修復 composition 模式下光標位置不正常的問題
* 修復 Word 中小狼毫工作不正常的問題
* 若干開發環境配置問題修復

#### 已知問題

* 部分應用仍存在輸入法無法輸入文字的問題
* WeaselServer 仍可能發生崩潰
* 部分防病毒軟件可能誤報病毒



<a name="0.14.3"></a>
## 0.14.3 (2019-06-22)


#### 主要更新

* 升級核心算法庫 [librime 1.5.3](https://github.com/rime/librime/blob/master/CHANGELOG.md#153-2019-06-22)
  * 修復 `single_char_filter` 組件
  * 完善上游項目 `librime` 的全自動發佈流程，免去手工上傳構建結果的步驟



<a name="0.14.2"></a>
## 0.14.2 (2019-06-17)


#### 主要更新

* 升級核心算法庫 [librime 1.5.2](https://github.com/rime/librime/blob/master/CHANGELOG.md#152-2019-06-17)
  * 修復用戶詞的權重，穩定造句質量、平衡翻譯器優先級 [librime#287](https://github.com/rime/librime/issues/287)
  * 建議 0.14.1 版本用家升級



<a name="0.14.1"></a>
## 0.14.1 (2019-06-16)


#### 主要更新

* 升級核心算法庫 [librime 1.5.1](https://github.com/rime/librime/blob/master/CHANGELOG.md#151-2019-06-16)
  * 修復未裝配語言模型時缺省的造句算法 ([weasel#383](https://github.com/rime/weasel/issues/383))



<a name="0.14.0"></a>
## 0.14.0 (2019-06-11)


#### 主要更新

* 升級核心算法庫 [librime 1.5.0](https://github.com/rime/librime/blob/master/CHANGELOG.md#150-2019-06-06)
  * 遷移到VS2017構建工具；建設安全可靠的全自動構建、發佈流程
  * 通過更新第三方庫，修復userdb文件夾大量佔用磁盤空間的問題
  * 將Rime插件納入自動化構建流程。本次發行包含兩款插件：
    - [librime-lua](https://github.com/hchunhui/librime-lua)
    - [librime-octagram](https://github.com/lotem/librime-octagram)
* 高清重製真彩輸入法狀態圖標


#### Features

* **ui:**  high-res status icons; display larger icons in WeaselPanel ([093fa806](https://github.com/rime/weasel/commit/093fa80678422f972e7a7285060553eeedb0e591))



<a name="0.13.0"></a>
## 0.13.0 (2019-01-28)


#### 主要更新

* 升級核心算法庫 [librime 1.4.0](https://github.com/rime/librime/blob/master/CHANGELOG.md#140-2019-01-16)
  * 新增 [拼寫糾錯](https://github.com/rime/librime/pull/228) 選項
    當前僅限 QWERTY 鍵盤佈局及使用 `script_translator` 的方案
  * 修復升級、部署數據時發生的若干錯誤
* 更換輸入法狀態圖標，適配高分辨率屏幕


#### Features

* **tsf:**  register as GUID_TFCAT_TIPCAP_UIELEMENTENABLED ([ae876916](https://github.com/rime/weasel/commit/ae8769166ea50b319aa89460b60890d598c618c5))
* **ui:**  high-res icons (#324) ([ad3e2027](https://github.com/rime/weasel/commit/ad3e2027644f80c6a384b7730da20dd239e780af))

#### Bug Fixes

* **WeaselSetup.vcxproj:**  Debug build linker options ([eb885fe0](https://github.com/rime/weasel/commit/eb885fe06ffd720d3de1101be2410a94bd3747c0))
* **output/install.nsi:**  bundle new yaml files from rime/rime-prelude ([cba35e9b](https://github.com/rime/weasel/commit/cba35e9b2c34d095b9ca1eb44e923e004cf23ddc))
* **test:**  Debug build ([c771126c](https://github.com/rime/weasel/commit/c771126c74fa1c4f91d4bfd8fb5ab8c16dcb7c4c))
* **tsf:**  set current page to 0 as page count is always 1 ([5447f63b](https://github.com/rime/weasel/commit/5447f63bc7c9d0e31d7ba8ead1e1229938be276d))



<a name="0.12.0"></a>
## 0.12.0  (2018-11-12)

#### 主要更新

* 合併小狼毫與小狼毫（TSF）兩種輸入法
* 合併32位與64位系統下的安裝程序
* 使用系統的關閉輸入法功能（默認快捷鍵 Ctrl + Space）後，輸入法圖標將顯示禁用狀態
* 修復一些情況下的崩潰問題
* 升級核心算法庫 [librime 1.3.2](https://github.com/rime/librime/blob/master/CHANGELOG.md#132-2018-11-12)
  * 允許多個翻譯器共用同一個詞典時的組詞，實現固定單字順序的形碼組詞([librime#184](https://github.com/rime/librime/issues/184))。
  * 新增 translator/always_show_comments 選項，允許始終顯示候選詞註解。

#### Bug Fixes

* **candidate:** fix COM pointer reference ([63d6d9a](https://github.com/rime/weasel/commit/63d6d9a))
* **ipc:** eliminate some trivial warnings ([dae945c](https://github.com/rime/weasel/commit/dae945c))
* fix constructor ([b25f968](https://github.com/rime/weasel/commit/b25f968))


#### Features

* **compartment:** show IME disabled on language bar ([#263](https://github.com/rime/weasel/issues/263)) ([4015d18](https://github.com/rime/weasel/commit/4015d18))
* **install:** combine IME and TSF ([#257](https://github.com/rime/weasel/issues/257)) ([91cbd2c](https://github.com/rime/weasel/commit/91cbd2c))
* **tsf:** get IME keyboard identifier by searching registry ([#272](https://github.com/rime/weasel/issues/272)) ([b60b5b1](https://github.com/rime/weasel/commit/b60b5b1))
* **WeaselSetup:** detect 64-bit on single 32-bit build ([#266](https://github.com/rime/weasel/issues/266)) ([fb3ae0f](https://github.com/rime/weasel/commit/fb3ae0f))



<a name="0.11.1"></a>
## 0.11.1 (2018-04-26)

#### 主要更新

* 修復了在 Excel 中奇怪的輸入丟失問題（[#185](https://github.com/rime/weasel/issues/185)）
* 功能鍵不再會觸發輸入焦點（[#194](https://github.com/rime/weasel/issues/194)、[#195](https://github.com/rime/weasel/issues/195)、[#204](https://github.com/rime/weasel/issues/204)）
* 「獲取更多輸入方案」功能優化（[#180](https://github.com/rime/weasel/issues/180)）
* 修復了後臺可能同時出現多個算法服務的問題（[#199](https://github.com/rime/weasel/issues/199)）
* 恢復語言欄右鍵菜單中「用戶資料同步」一項

#### Bug Fixes

* **server:**  use kernel mutex to ensure single instance (#207) ([bd0c4720](https://github.com/rime/weasel/commit/bd0c4720669c61087dd930b968640c60a526ecb2))
* **tsf:**
  *  do not reset composition on document focus set ([124fc947](https://github.com/rime/weasel/commit/124fc9475c30963a9bbbf9a097b452b52e8ab658))
  *  use `ITfContext::GetSelection` to get cursor position ([5664481c](https://github.com/rime/weasel/commit/5664481cc9ddd28db35c3155f7ddf83a55b65275))
  *  recover sync option in TSF language bar menu ([7a0a8cc2](https://github.com/rime/weasel/commit/7a0a8cc2a3dd913ce34204d6e966b263af766f3b))

#### Features

* **build.bat:**  build installer ([e18117b7](https://github.com/rime/weasel/commit/e18117b7b42d5af0fbfa807e4c858c40206b4967))
* **installer:**  bundle curl, update rime-install.bat, fixes #180 ([2f3b283d](https://github.com/rime/weasel/commit/2f3b283d6ef4aa0580d186e626dadb9e1030dfd5))
* **rime-install.bat:**  built-in ZIP package installer ([739be9bc](https://github.com/rime/weasel/commit/739be9bc9ba08e294f51e1d7232407148ded716c))



<a name="0.11.0"></a>
## 0.11.0 (2018-04-07)

#### 主要更新

* 新增 [Rime 配置管理器](https://github.com/rime/plum)，通過「輸入法設定／獲取更多輸入方案」調用
* 在輸入法語言欄顯示狀態切換按鈕（TSF 模式）
* 修復多個前端兼容性問題
* 新增配色主題「現代藍」`metroblue`、「幽能」`psionics`
* 安裝程序支持繁體中文介面
* 修復 0.10 版升級安裝後，因用戶文件夾中保留舊文件、配置不生效的問題
* 升級 0.9 版 `.kct` 格式的用戶詞典
  **注意**：僅此一個版本支持格式升級，請務必由 0.9 升級到 0.11，再安裝後續版本

#### Features

* **WeaselDeployer:**  add Get Schemata button to run plum script (#174) ([c786bb5b](https://github.com/rime/weasel/commit/c786bb5ba2f1cc7e79b66f36d0190e61cd7233ae))
* **build.bat:**  customize PLATFORM_TOOLSET settings ([c7a9a4fb](https://github.com/rime/weasel/commit/c7a9a4fb530e0274450e4296cb0db2906d2f1fb4))
* **config:**
  *  enable customization of label format ([76b08bae](https://github.com/rime/weasel/commit/76b08bae810735c5f1c8626ec39a7afd463f0269))
  *  alias `style/layout/border_width` to `style/layout/border` ([013eefeb](https://github.com/rime/weasel/commit/013eefebaa4474e7814b6cfb6c905bcc12543a7f))
* **install.nsi:**
  *  add Traditional Chinese for installer ([d1a9696a](https://github.com/rime/weasel/commit/d1a9696a57dfc9e04c51899572e156fb1676f786))
  *  upgrade to Modern UI 2 and prompt reboot (#128) ([f59006f8](https://github.com/rime/weasel/commit/f59006f8d195ca848e45cd934f44b3318fb135c1))
* **ipc:**  specify user name for named pipe ([2dfa5e1a](https://github.com/rime/weasel/commit/2dfa5e1a63ee1c26ef983d25471682f87cc60b62))
* **preset_color_schemes:**
  *  add homepage featured color scheme `psionics` ([89a0eb8b](https://github.com/rime/weasel/commit/89a0eb8b861b9b3f2abc42df65821254010b24ff))
  *  add metroblue color scheme ([f43e2af6](https://github.com/rime/weasel/commit/f43e2af608bde38a6d345ba540f4c37ec024853a))
* **submodules:**  switch from rime/brise to rime/plum ([f3ff5aa9](https://github.com/rime/weasel/commit/f3ff5aa962a7b8cce2b74a5cb583a69cb8938e55))
* **tsf:**
  *  enable language bar button (#170) ([2b660397](https://github.com/rime/weasel/commit/2b660397950f348205e6a93bf44a46e4a72bcc81))
  *  accomplish candidate UI interfaces (#156) ([1f0ae793](https://github.com/rime/weasel/commit/1f0ae7936fd495ecf4ff3ef162c0e38297d2d582))
  *  fix candidate selecting in preview preedit mode ([206efd69](https://github.com/rime/weasel/commit/206efd692124339d0e256198360c1860c72cd807))
  *  support user defined preedit display type ([f76379b0](https://github.com/rime/weasel/commit/f76379b01abe9d3971d68e2e272067e0bb855cc9))
* **weasel.yaml:**  enable ascii_mode in console applications by default ([28cdd096](https://github.com/rime/weasel/commit/28cdd09692f77e471784bf85ff7a19bc48e113f4))

#### Bug Fixes

*   fix defects according to Coverity Scan ([526a91d2](https://github.com/rime/weasel/commit/526a91d2954492cc8e23c2c4c8def2a053af7c20))
*   inline_preedit && fullscreen causing dead lock when there's no candidates. ([deb0bb24](https://github.com/rime/weasel/commit/deb0bb24b3f3aeaf73aef344968b7f15b471443f))
* **RimeWithWeasel:**  fix wild pointer ([ae2e3c4a](https://github.com/rime/weasel/commit/ae2e3c4a256fb9a2f7851c54114822d1bfbf0316))
* **ServerImpl:**  do finalization before exit process ([b1bae01e](https://github.com/rime/weasel/commit/b1bae01eb25c5e24e074807b7b3cb8a6d8401276))
* **WeaselUI:**
  *  specify default label format in constructor ([4374d244](https://github.com/rime/weasel/commit/4374d2440b99726894799861fb3bd5b93e73dec5), closes [#147](https://github.com/rime/weasel/issues/147))
  *  limit to subscript range when processing candidates ([6b686c71](https://github.com/rime/weasel/commit/6b686c717bfab141469c3d48ec1c6acbeb79921e), closes [#121](https://github.com/rime/weasel/issues/121))
* **composition:**
  *  improve compositions and edit sessions (#146) ([fbdb6679](https://github.com/rime/weasel/commit/fbdb66791da3291b740edf3c337032674e4377e8))
  *  fix crashes in notebook with inline preedit ([5e257088](https://github.com/rime/weasel/commit/5e257088be823a2569609f0b3591af3a51d47a46))
  *  fix crashes in notebook with inline preedit ([892930ce](https://github.com/rime/weasel/commit/892930cebc4235a0a1ef58803fe88c32ccc8b4e9))
* **install.bat:**  run in elevate cmd; detach WeaselServer process ([2194d9fb](https://github.com/rime/weasel/commit/2194d9fbd7d0341fef94efdbe9268af8a6237438))
* **ipc:**
  *  add version check for security descriptor initialization ([b97ccffe](https://github.com/rime/weasel/commit/b97ccffe76a6abf3e353724ce0607d5dd97de6f2), closes [#157](https://github.com/rime/weasel/issues/157))
  *  grant access to IE protected mode ([16c163a4](https://github.com/rime/weasel/commit/16c163a41d0afc9824723009ba8b9b9ba37b1c72))
  *  try to reconnect when failed ([3c286b6a](https://github.com/rime/weasel/commit/3c286b6a942769abf13188d88f9ab5e4c125807b))
* **librime:**  make rime_api.h available in librime\build\include\ ([3793e22c](https://github.com/rime/weasel/commit/3793e22c47b34c61d305ca80567dfdafe08b2302))
* **server:**  postpone tray icon updating when focusing on explorer ([45cf1120](https://github.com/rime/weasel/commit/45cf112099fa6db335cda06b1aaa0ae9c7975efe))
* **tsf:**
  *  fix candidate behavior ([9e2f9f17](https://github.com/rime/weasel/commit/9e2f9f17c059bf129c2c8b2561471670ea200dd7))
  *  fix `ITfCandidateListUIElement` implemention ([9ce1fa87](https://github.com/rime/weasel/commit/9ce1fa87e6ef788e791e68193700e2ebdd950d20))
  *  use commmit text preview to show inline preview ([b1d1ec43](https://github.com/rime/weasel/commit/b1d1ec43e132998ea8764d8dac2098a2b3d9a3e8))



<a name="0.10.0"></a>
## 小狼毫 0.10.0 (2018-03-14)


#### 主要更新

* 兼容 Windows 8 ~ Windows 10
* 支持高分辨率顯示屏
* 介面風格選項
  * 在內嵌編碼行預覽結果文字
  * 可指定候選序號的樣式
* 升級核心算法庫 [librime 1.3.0](https://github.com/rime/librime/blob/master/CHANGELOG.md#130-2018-03-09)
  * 支持 YAML 節點引用，方便模塊化配置
  * 改進部署流程，在 `build` 子目錄集中存放生成的數據文件
* 精簡安裝包預裝的輸入方案，更多方案可由 [東風破](https://github.com/rime/plum) 取得

#### Features

* **build.bat:**  customize PLATFORM_TOOLSET settings ([c7a9a4fb](https://github.com/rime/weasel/commit/c7a9a4fb530e0274450e4296cb0db2906d2f1fb4))
* **config:**
  *  enable customization of label format ([76b08bae](https://github.com/rime/weasel/commit/76b08bae810735c5f1c8626ec39a7afd463f0269))
  *  alias `style/layout/border_width` to `style/layout/border` ([013eefeb](https://github.com/rime/weasel/commit/013eefebaa4474e7814b6cfb6c905bcc12543a7f))
* **tsf:**
  *  fix candidate selecting in preview preedit mode ([206efd69](https://github.com/rime/weasel/commit/206efd692124339d0e256198360c1860c72cd807))
  *  support user defined preedit display type ([f76379b0](https://github.com/rime/weasel/commit/f76379b01abe9d3971d68e2e272067e0bb855cc9))

#### Bug Fixes

*   Support High DPI Display [#28](https://github.com/rime/weasel/issues/28)
* **WeaselUI:**  limit to subscript range when processing candidates ([6b686c71](https://github.com/rime/weasel/commit/6b686c717bfab141469c3d48ec1c6acbeb79921e), closes [#121](https://github.com/rime/weasel/issues/121))
* **install.bat:**  run in elevate cmd; detach WeaselServer process ([2194d9fb](https://github.com/rime/weasel/commit/2194d9fbd7d0341fef94efdbe9268af8a6237438))
* **librime:**  make rime_api.h available in librime\build\include\ ([3793e22c](https://github.com/rime/weasel/commit/3793e22c47b34c61d305ca80567dfdafe08b2302))
* **tsf:**
  *  Results of auto-selection cleared by subsequent manual selection [#107](https://github.com/rime/weasel/issues/107)
  *  use commmit text preview to show inline preview ([b1d1ec43](https://github.com/rime/weasel/commit/b1d1ec43e132998ea8764d8dac2098a2b3d9a3e8))



<a name="0.9.30"></a>
## 小狼毫 0.9.30 (2014-04-01)


#### Rime 算法庫變更集

* 新增：中西文切換方式 `clear`，切換時清除未完成的輸入
* 改進：長按 Shift（或 Control）鍵不觸發中西文切換
* 改進：並擊輸入，若按回車鍵則上屏按鍵對應的字符
* 改進：支持對用戶設定中的列表元素打補靪，例如 `switcher/@0/reset: 1`
* 改進：缺少詞典源文件 `*.dict.yaml` 時利用固態詞典 `*.table.bin` 完成部署
* 修復：自動組詞的詞典部署時未檢查【八股文】的變更，導致索引失效、候選字缺失
* 修復：`comment_format` 會對候選註釋重複使用多次的BUG

#### 【東風破】變更集

* 新增：快捷鍵 `Control+.` 切換中西文標點
* 更新：【八股文】【朙月拼音】【地球拼音】【五筆畫】
* 改進：【朙月拼音·語句流】`/0` ~ `/10` 輸入數字符號



<a name="0.9.29.1"></a>
## 小狼毫 0.9.29.1 (2013-12-22)


#### 【小狼毫】變更集

* 變更：不再支持 Windows XP SP2，因升級編譯器以支持 C++11
* 修復：輸入語言選爲中文（臺灣）在 Windows 8 系統上出現多餘的輸入法選項
* 修復：升級安裝後，外觀設定介面未及時顯示出新增的配色方案
* 修復：配色方案 Google+ 的預覽圖

#### Rime 算法庫變更集

* 更新：librime 升級到 1.1
* 新增：固定方案選單排列順序的選項 `default.yaml`: `switcher/fix_schema_list_order: true`
* 修復：正確匹配嵌套的“‘彎引號’”
* 改進：碼表輸入法自動上屏及頂字上屏（[示例](https://gist.github.com/lotem/f879a020d56ef9b3b792)）<br/>
    若有 `speller/auto_select: true`，則選項 `speller/max_code_length:` 限定第N碼無重碼自動上屏
* 優化：爲詞組自動編碼時，限制因多音字而產生的組合數目，避免窮舉消耗過量資源

#### 【東風破】變更集

* 更新：【粵拼】匯入衆多粵語詞彙
* 優化：調整部分異體字的字頻



<a name="0.9.28"></a>
## 小狼毫 0.9.28 <2013-12-01>


#### 【小狼毫】變更集

* 新增：一組配色方案，作者：P1461、Patricivs、skoj、五磅兔
* 修復：[Issue 528](https://code.google.com/p/rimeime/issues/detail?id=528) Windows 7 IE11 文字無法上屏
* 修復：[Issue 531](https://code.google.com/p/rimeime/issues/detail?id=531) Windows 8 卸載輸入法後在輸入法列表中有殘留項
* 變更：註冊輸入法時同時啓用 IME、TSF 模式

#### Rime 算法庫變更集

* 更新：librime 升級到 1.0
* 改進：`affix_segmentor` 支持向匹配到的代碼段添加標籤 `extra_tags`
* 修復：`table_translator` 按字符集過濾候選字，修正對 CJK-D 漢字的判斷

#### 【東風破】變更集

* 優化：【粵拼】兼容[教育學院拼音方案](http://zh.wikipedia.org/wiki/%E6%95%99%E8%82%B2%E5%AD%B8%E9%99%A2%E6%8B%BC%E9%9F%B3%E6%96%B9%E6%A1%88)
* 更新：`symbols.yaml` 由 Patricivs 重新整理符號表
* 更新：Emoji 提供更加豐富的繪文字（需要字體支持）
* 更新：【八股文】【朙月拼音】【地球拼音】【中古全拼】修正錯別字、註音錯誤



<a name="0.9.27"></a>
## 小狼毫 0.9.27 (2013-11-06)


#### 【小狼毫】變更集

* 變更：動態鏈接 `rime.dll`，減小程序文件的體積
* 修復：嘗試解決 Issue 487 避免服務進程以 SYSTEM 帳號執行
* 新增：開始菜單項「安裝選項」，Vista 以降提示以管理員權限啓動
* 優化：更換圖標，解決 Windows 8 TSF 圖標不清楚的問題

#### Rime 算法庫變更集

* 優化：同步用戶資料時自動備份自定義短語等 .txt 文件
* 修復：【地球拼音】反查拼音失效的問題
* 變更：編碼提示不再添加括弧（，）及逗號，可自行設定樣式

#### 輸入方案設計支持

* 新增：`affix_segmentor` 分隔編碼的前綴、後綴
* 改進：`translator` 支持匹配段落標籤
* 改進：`simplifier` 支持多個實例，匹配段落標籤
* 新增：`switches:` 輸入方案選項支持多選一
* 新增：`reverse_lookup_filter` 爲候選字標註指定種類的輸入碼

#### 【東風破】變更集

* 更新：【粵拼】補充大量單字的註音
* 更新：【朙月拼音】【地球拼音】導入 Unihan 讀音資料
* 改進：【地球拼音】【注音】啓用自定義短語
* 新增：【注音·臺灣正體】
* 修復：【朙月拼音·簡化字】通過快捷鍵 `Control+Shift+4` 簡繁切換
* 改進：【倉頡五代】開啓繁簡轉換時，提示簡化字對應的傳統漢字
* 變更：間隔號採用「·」`U+00B7`



<a name="0.9.26.1"></a>
## 小狼毫 0.9.26.1 (2013-10-09)

* 修復：從上一個版本升級【倉頡】輸入方案不會自動更新的問題



<a name="0.9.26"></a>
## 小狼毫 0.9.26 (2013-10-08)

* 新增：【倉頡】開啓自動造詞<br/>
  連續上屏的5字（依設定）以內的組合，或以連打方式上屏的短語，
  按構詞規則記憶爲新詞組；再次輸入該詞組的編碼時，顯示「☯」標記
* 變更：【五筆】開啓自動造詞；從碼表中刪除與一級簡碼重碼的鍵名字
* 變更：【地球拼音】當以簡拼輸入時，爲5字以內候選標註完整帶調拼音
* 新增：【五筆畫】輸入方案（`stroke`），取代 `stroke_simp`
* 新增：支持在輸入方案中設置介面樣式（`style:`）<br/>
  如字體、字號、橫排／直排等；配色方案除外
* 修復：多次按「.」鍵翻頁後繼續輸入，不應視爲網址而在編碼中插入「.」
* 修復：開啓候選字的字符集過濾，導致有時不出現連打候選詞的 BUG
* 修復：`table_translator` 連打組詞時產生的內存泄漏（0.9.25.2）
* 修復：爲所有用戶創建開始菜單項
* 更新：修訂【八股文】詞典、【朙月拼音】【地球拼音】【粵拼】【吳語】
* 更新：2013款 Rime 輸入法圖標



<a name="0.9.25.2"></a>
## 小狼毫 0.9.25.2 (2013-07-26)

* 改進：碼表輸入法連打，Shift+BackSpace 以字、詞爲單位回退
* 修復：演示模式下開啓內嵌編碼行、查無候選字時程序卡死



<a name="0.9.25.1"></a>
## 小狼毫 0.9.25.1 (2013-07-25)

* 新增：開始菜單項「檢查新版本」，手動升級到最新測試版
* 新增：【地球拼音】5 字內候選標註完整帶調拼音



<a name="0.9.25"></a>
## 小狼毫 0.9.25 (2013-07-24)

* 新增：演示模式（全屏的輸入窗口）`style/fullscreen: true`
* 新增：【倉頡】按快趣取碼規則生成常用詞組
* 修復：【地球拼音】「-」鍵輸入第一聲失效的BUG
* 更新：拼音、粵拼等輸入方案
* 更新：`symbols.yaml` 增加一批特殊字符



<a name="0.9.24"></a>
## 小狼毫 0.9.24 (2013-07-04)

* 新增：支持全角模式
* 更新：中古漢語【全拼】【三拼】輸入方案；三拼亦採用全拼詞典
* 修復：大陸與臺灣異讀的字「微」「檔」「蝸」「垃圾」等
* 修復：繁簡轉換錯詞「么么哒」
* 新增：（輸入方案設計用）可設定對特定類型的候選詞不做繁簡轉換<br/>
  如不轉換反查字使用選項 `simplifier/excluded_types: [ reverse_lookup ]`
* 新增：（輸入方案設計用）干預多個 translator 之間的結果排序<br/>
  選項 `translator/initial_quality: 0`
* 修復：用戶詞典未能完整支持 `derive` 拼寫運算產生的歧義切分



<a name="0.9.23"></a>
## 小狼毫 0.9.23 (2013-06-09)

* 改進：方案選單按選用輸入方案的時間排列
* 新增：快捷鍵 Control+Shift+1 切換至下一個輸入方案
* 新增：快捷鍵 Control+Shift+2~5 切換輸入模式
* 新增：初次安裝時由用戶指定輸入語言：中文（中國／臺灣）
* 新增：可屏蔽符合 fuzz 拼寫規則的單字候選，僅以其輸入詞組<br/>
  選項 `translator/strict_spelling: true`
* 改進：綜合候選詞的詞頻和詞條質量比較不同 translator 的結果
* 修復：自定義短語不應參與組詞
* 修復：八股文錯詞及「鏈」字無法以簡化字組詞的 BUG



<a name="0.9.22.1"></a>
## 小狼毫 0.9.22.1 (2013-04-24)

* 修復：禁止自定義短語參與造句
* 修復：GVim 裏進入命令模式或在插入模式換行錯使輸入法重置爲初始狀態



<a name="0.9.22"></a>
## 小狼毫 0.9.22 (2013-04-23)

* 新增：配色方案【曬經石】／Solarized Rock
* 新增：Control+BackSpace 或 Shift+BackSpace 回退一個音節
* 新增：固態詞典可引用多份碼表文件以實現分類詞庫
* 新增：在輸入方案中加載翻譯器的多個具名實例
* 新增：以選項 `translator/user_dict:` 指定用戶詞典的名稱
* 新增：支持從用戶文件夾加載文本碼表作爲自定義短語詞典<br/>
  【朙月拼音】系列自動加載名爲 `custom_phrase.txt` 的碼表
* 修復：繁簡轉換使無重碼自動上屏失效的 BUG
* 修復：若非以 Caps Lock 鍵進入西文模式，<br/>
  按 Caps Lock 只切換大小寫，不返回中文模式
* 變更：`r10n_translator` 更名爲 `script_translator`，舊名稱仍可使用
* 變更：用戶詞典快照改爲文本格式
* 改進：【八股文】導入《萌典》詞彙，並修正了不少錯詞
* 改進：【倉頡五代】打單字時，以拉丁字母和倉頡字母並列顯示輸入碼
* 改進：使自動生成的 YAML 文檔更合理地縮排、方便閱讀
* 改進：碼表中 `# no comments` 行之後不再識別註釋，以支持 `#` 作文字內容
* 改進：檢測到因斷電造成用戶詞典損壞時，自動在後臺線程恢復數據文件



<a name="0.9.20"></a>
## 小狼毫 0.9.20 (2013-02-01)

* 變更：Caps Lock 燈亮時默認輸出大寫字母 [Gist](https://gist.github.com/2981316)
  升級安裝後若 Caps Lock 的表現不正確，請註銷並重新登錄
* 新增：無重碼自動上屏 `speller/auto_select:`<br/>
  輸入方案【倉頡·快打模式】
* 改進：允許以空格做輸入碼，或作爲符號頂字上屏<br/>
  `speller/use_space:`, `punctuator/use_space:`
* 改進：【注音】輸入方案以空格輸入第一聲（陰平）
* 新增：特殊符號表 `symbols.yaml` 用法見↙
* 改進：【朙月拼音·簡化字】以 `/ts` 等形式輸入特殊符號
* 改進：標點符號註明〔全角〕〔半角〕
* 優化：同步用戶資料時更聰明地備份用戶自定義的 YAML 文件
* 修復：避免創建、使用不完整的詞典文件
* 修復：糾正用戶詞典中無法調頻的受損詞條
* 修復：用戶詞典管理／輸出詞典快照後定位文件出錯
* 修復：TSF 內嵌輸入碼沒有反選效果、候選窗位置頻繁變化



<a name="0.9.19.1"></a>
## 小狼毫 0.9.19.1 (2013-01-16)

* 新增：Caps Lock 點亮時，切換到西文模式，輸出小寫字母<br/>
  選項 `ascii_composer/switch_key/Caps_Lock:`
* 修復：Control + 字母编辑键在临时西文模式下无效
* 修復：用戶詞典有可能因讀取時 I/O 錯誤導致部份詞序無法調整
* 改進：用戶詞典同步／合入快照的字頻合併算法



<a name="0.9.18.6"></a>
## 小狼毫 0.9.18.6 (2013-01-09)

* 修復：從 0.9.16 及以下版本升級用戶詞典出錯



<a name="0.9.18.5"></a>
## 小狼毫 0.9.18.5 (2013-01-07)

* 修復：含簡化字的候選詞不能以音節爲單位移動光標
* 改進：同步用戶資料時也備份用戶修改的YAML文件



<a name="0.9.18"></a>
## 小狼毫 0.9.18 (2013-01-05)

* 新增：同步用戶詞典，詳見 [Wiki » UserGuide](https://code.google.com/p/rimeime/wiki/UserGuide)
* 新增：上屏錯誤的詞組後立即按回退鍵（BackSpace）撤銷組詞
* 改進：拼音輸入法中，按左方向鍵以音節爲單位移動光標
* 修復：【地球拼音】不能以 - 鍵輸入第一聲



<a name="0.9.17.1"></a>
## 小狼毫 0.9.17.1 (2012-12-25)

* 修復：設置爲默認輸入語言後再安裝，IME 註冊失敗
* 修復：啓用托盤圖標的選項無效
* 新增：從開始菜單訪問用戶文件夾的快捷方式
* 修復：【小鶴雙拼】拼音 an 顯示錯誤



<a name="0.9.17"></a>
## 小狼毫 0.9.17 (2012-12-23)

* 新增：切換模式、輸入方案時，短暫顯示狀態圖標
* 新增：隱藏托盤圖標，設定、部署、詞典管理請用開始菜單。<br/>
  配置項 `style/display_tray_icon:`
* 修復BUG：TSF 前端在 MS Office 裏不能正常上屏中文
* 刪除：默認不啓用 TSF 前端，如有需要可在「文本服務與輸入語言」設置對話框添加。
* 新增：分別以 `` ` ' `` 標誌編碼反查的開始結束，例如 `` `wbb'yuepinyin ``
* 改進：形碼與拼音混打的設定下，降低簡拼候選的優先級，以降低對逐鍵提示的干擾
* 優化：控制用戶詞典文件大小，提高大容量（詞條數>100,000）時的查詢速度
* 刪除：因有用家向用戶詞典導入巨量詞條，故取消自動備份的功能，後續代之以用戶詞典同步
* 修復：【小鶴雙拼】diao, tiao 等拼音回顯錯誤
* 更新：【朙月拼音】【地球拼音】【粵拼】修正用戶反饋的註音錯誤



<a name="0.9.16"></a>
## 小狼毫 0.9.16 (2012-10-20)

* 新增：TSF 輸入法框架（測試階段）及嵌入式編碼行
* 新增：支持 IE 8 ~ 10 的「保護模式」
* 新增：識別 gVim 模式切換
* 新增：開關碼表輸入法連打功能的設定項 `translator/enable_sentence: `
* 修復：「語句流」模式直接回車上屏不能記憶用戶詞組的BUG
* 改進：部署時自動編譯輸入方案的自訂依賴項，如自選的反查碼
* 改進：更精細的排版，修正註釋文字寬度、調整間距
* 改進：未曾翻頁時按減號鍵，不上屏候選字及符號「-」以免誤操作
* 變更：《注音》以逗號或句號（<> 鍵）上屏句子，書名號改用 [] 鍵
* 更新：《朙月拼音》《地球拼音》《粵拼》，修正多音字
* 更新：《上海吳語》《上海新派》，修正註音
* 新增：寒寒豆作《蘇州吳語》輸入方案，方案標識爲 `soutzoe`
* 新增：配色方案【谷歌／Google】，skoj 作品



<a name="0.9.15"></a>
## 小狼毫 0.9.15 (2012-09-12)

* 新增：橫排候選欄——歡迎 wishstudio 同學加入開發！
* 新增：綠色安裝工具 WeaselSetup，註冊輸入語言、自訂用戶目錄
* 新增：碼表輸入法啓用用戶詞典、字頻調整
* 優化：自動編譯輸入方案依賴項，如五筆·拼音的反查詞典
* 修改：日誌系統改用glog，輸出到 `%TEMP%\rime.weasel.*`
* 修復：托盤圖標在重新登錄後不可見的BUG
* 更新：【明月拼音】【粵拼】【吳語】修正註音錯誤、缺字



<a name="0.9.14.2"></a>
## 小狼毫 0.9.14.2 (2012-07-13)

* 重新編譯了 `opencc.dll` 安全軟件不吭氣了



<a name="0.9.14.1"></a>
## 小狼毫 0.9.14.1 (2012-07-07)

* 解決【中古全拼】不可用的問題



<a name="0.9.14"></a>
## 小狼毫 0.9.14 (2012-07-05)

* 介面採用新的 Rime logo，狀態圖示用較柔和的顏色
* 新特性：碼表方案支持與反查碼混合輸入，無需切換或引導鍵
* 新特性：碼表方案可在選單中使用字符集過濾開關
* 新方案：【五筆86】衍生的【五筆·拼音】混合輸入
* 新方案：《廣韻》音系的中古漢語全拼、三拼輸入法
* 新方案：X-SAMPA 國際音標輸入法
* 更新：【吳語】碼表，審定一些字詞的讀音，統一字形
* 更新：【朙月拼音】碼表，修正多音字
* 改進：當前設定的字體缺字時，使用系統後備字體顯示文字
* 解決與MacType同時使用，Ext-B/C/D區文字排版不正確的問題



<a name="0.9.13"></a>
## 小狼毫 0.9.13 (2012-06-10)

* 編碼提示用淡墨來寫，亦可在配色方案中設定顏色
* 新增多鍵並擊組件及輸入方案【宮保拼音】
* 未經轉換的輸入如網址等不再顯示爲候選項
* `default.custom.yaml`: `menu/page_size:` 設定全局頁候選數
* 新增選項：導入【八股文】詞庫時限制詞語的長度、詞頻
* 【倉頡】支持連續輸入多個字的編碼（階段成果，不會記憶詞組）
* 【注音】改爲語句輸入風格，更接近臺灣用戶的習慣
* 較少用的【筆順五碼】、【速記打字法】不再隨鼠鬚管發行
* 修復「用戶詞典管理」導入文本碼表不生效的BUG；<br/>
  部署時檢查並修復已存在於用戶詞典中的無效條目
* 檢測到用戶詞典文件損壞時，重建詞典並從備份中恢復資料
* 修改BUG：簡拼 zhzh 因切分歧義使部分用戶詞失效



<a name="0.9.12"></a>
## 小狼毫 0.9.12 (2012-05-05)

* 用 Shift+Del 刪除已記入用戶詞典的詞條，詳見 Issue 117
* 可選用Shift或Control爲中西文切換鍵，詳見 Issue 133
* 數字後的句號鍵識別爲小數點、冒號鍵識別爲時分秒分隔符
* 解決在QQ等應用程序中的定位問題
* 支持設置爲系統默認輸入法
* 支持多個Windows用戶（新用戶執行一次佈署後方可使用）



<a name="0.9.11"></a>
## 小狼毫 0.9.11 (2012-04-14)

* 使用 `express_editor` 的輸入方案中，數字、符號鍵直接上屏
* 優化「方案選單」快捷鍵操作，連續按鍵選中下一個輸入方案
* 輸入簡拼、模糊音時提示正音，【粵拼】【吳語】中默認開啓
* 拼音反查支持預設的多音節詞、形碼反查可開啓編碼補全
* 修復整句模式運用定長編碼頂字功能導致崩潰的問題
* 修復碼表輸入法候選排序問題
* 修復【朙月拼音】lo、yo 等音節的候選錯誤
* 修復【地球拼音】聲調顯示不正確、部分字的註音缺失問題
* 【五笔86】反查引導鍵改爲 z、反查詞典換用簡化字拼音
* 更新【粵拼】詞典，調整常用粵字的排序、增補粵語常用詞
* 新增輸入方案【小鶴雙拼】、【筆順五碼】



<a name="0.9.10"></a>
## 小狼毫 0.9.10 (2012-03-26)

* 記憶繁簡轉換、全／半角符號開關狀態
* 支持定長編碼頂字上屏
* 新增「用戶詞典管理」介面
* 延遲加載繁簡轉換、編碼反查詞典，降低資源佔用
* 純單字構詞時不調頻
* 新增輸入方案【速成】，速成、倉頡詞句連打
* 新增【智能ABC雙拼】、【速記打字法】



<a name="0.9.9"></a>
## 小狼毫 0.9.9

* 新增「介面風格設定」，快速選擇預設的六款配色方案
* 優化長句中字詞的動態調頻
* 新增【注音】與【地球拼音】輸入方案
* 支持自訂選詞按鍵
* 修復編碼反查失效的BUG
* 修改標點符號「間隔號」及「浪紋」



<a name="0.9.8"></a>
## 小狼毫 0.9.8

* 新增「輸入方案選單」設定介面
* 優化包含簡拼的音節切分
* 修復部分用戶組詞無效的BUG
* 新增預設輸入方案「MSPY雙拼」



<a name="0.9.7"></a>
## 小狼毫 0.9.7

* 逐鍵提示、反查提示碼支持拼寫運算（如顯示倉頡字母等）
* 重構部署工具；以 `*.custom.yaml` 文件持久保存自定義設置
* 製作【粵拼】、【吳語】輸入方案「預發行版」



<a name="0.9.6"></a>
## 小狼毫 0.9.6

* 關機時妥善保存數據，降低用戶詞庫損壞機率；執行定期備份
* 新增基於【朙月拼音】的衍生方案：
  * 【語句流】，整句輸入，空格分詞，回車上屏
  * 【雙拼】，兼容自然碼雙拼方案，演示拼寫運算常用技巧
* 修復BUG：簡拼「z h, c h, s h」的詞候選先於單字簡拼
* 修復BUG：「拼寫運算」無法替換爲空串
* 完善拼寫運算的錯誤日誌；清理調試日誌



<a name="0.9.5"></a>
## 小狼毫 0.9.5

* Rime 獨門絕活之「拼寫運算」
* 升級【朙月拼音】，支持簡拼、糾錯；增設【簡化字】方案
* 升級【倉頡五代】，以倉頡字母顯示編碼
* 重修配色方案【碧水／Aqua】、【青天／Azure】



<a name="0.9.4"></a>
## 小狼毫 0.9.4

* 增設編碼反查功能，預設方案以「`」爲反查的引導鍵
* 修復Windows XP中西文狀態變更時的通知氣球



<a name="0.9.3"></a>
## 小狼毫 0.9.3

* 新增預設輸入方案【五笔86】、【臺灣正體】拼音
* 以托盤圖標表現輸入法狀態變更
* 新增輸入法維護模式，更安全地進行部署作業
* 優化中西文切換、自動識別小數、百分數、網址、郵箱



<a name="0.9.2"></a>
## 小狼毫 0.9.2

* 增設半角標點符號
* 增設Shift鍵切換中／西文模式
* 繁簡轉換、左Shift切換中西文對當前輸入即時生效
* 可自定義OpenCC異體字轉換字典
* 提升碼表查詢效率，更新倉頡七萬字碼表
* 增設托盤圖標，快速訪問配置管理工具
* 改進安裝程序



<a name="0.9"></a>
## 小狼毫 0.9

* 用C++重寫核心算法（階段成果）
* 將輸入法介面從前端遷移到後臺服務進程
* 兼容64位系統



## 小狼毫 0.1 ~ 0.3

* 以Python開發的實驗版本
* 獨創「拼寫運算」技術
* 預裝標調拼音、註音、粵拼、吳語等多種輸入方案
