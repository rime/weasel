【小狼毫】輸入法
================

基於 中州韻輸入法引擎／Rime Input Method Engine 等開源技術

式恕堂 版權所無

[![Download](https://api.bintray.com/packages/rime/weasel/release/images/download.svg)](https://bintray.com/rime/weasel/release/_latestVersion)
[![Build status](https://ci.appveyor.com/api/projects/status/github/rime/weasel?svg=true)](https://ci.appveyor.com/project/rime/weasel)
[![GitHub Tag](https://img.shields.io/github/tag/rime/weasel.svg)](https://github.com/rime/weasel)

授權條款：GPLv3

項目主葉：https://rime.im

您可能還需要 RIME 用於其他操作系統的發行版：

  * 【中州韻】用於 Linux + IBus 輸入法框架
  * 【鼠鬚管】用於 macOS （64位）

安裝輸入法
----------

本品適用於 Windows XP SP3, Windows 7 ~ Windows 10

初次安裝時，安裝程序將顯示「安裝選項」對話框。

若要將【小狼毫】註冊到繁體中文（臺灣）鍵盤佈局，請在「輸入語言」欄選擇「中文（臺灣）」，再點擊「安裝」按鈕。

安裝完成後，仍可由開始菜單打開「安裝選項」更改輸入語言。

使用輸入法
----------

選取輸入法指示器菜單裏的【中】字樣圖標，開始用小狼毫寫字。

可通過快捷鍵 Ctrl+` 或 F4 呼出方案選單、切換輸入方式。

定製輸入法
----------

通過 開始菜單 » 小狼毫輸入法 訪問設定工具及常用位置。

用戶詞庫、配置文件位於 `%AppData%\Rime`，可通過菜單中的「用戶文件夾」打開。高水平玩家調教 Rime 輸入法常會用到。

修改詞庫、配置文件後，須「重新部署」方可生效。

定製 Rime 的方法，請參考 Wiki [《定製指南》](https://github.com/rime/home/wiki/CustomizationGuide)

致謝
----

### 輸入方案設計：

  * 【朙月拼音】系列及【八股文】詞典
    - 部分數據來源於 CC-CEDICT、Android 拼音、新酷音、opencc 等開源項目
    - 維護者：佛振、瑾昀
  * 【注音／地球拼音】
    - 維護者：佛振、瑾昀
  * 【倉頡五代】
    - 發明人：朱邦復先生
    - 碼表源自 www.chinesecj.com
    - 構詞碼表作者：惜緣

  【五笔】【粵拼】【上海／蘇州吳語】【中古漢語拼音】【國際音標】等衆多方案
  不再以安裝包預裝形式提供。可由 <https://github.com/rime/plum> 下載安裝。

### 程序設計：

  * [佛振](https://github.com/lotem)
  * [鄒旭](https://github.com/zouxu09)
  * [Xiangyan Sun](https://github.com/wishstudio)
  * [Prcuvu](https://github.com/Prcuvu)
  * [nameoverflow](https://github.com/nameoverflow)

  查看更多 [代碼貢獻者](https://github.com/rime/weasel/graphs/contributors)

### 美術：

  * 圖標設計／[Patricivs](https://github.com/Patricivs)
  * 配色方案／Aben、P1461、Patricivs、skoj、佛振、五磅兔

### 本品引用了以下開源軟件：

  * [Boost C++ Libraries](http://www.boost.org/) (Boost Software License)
  * [curl](https://curl.haxx.se/) (MIT/X derivate license)
  * [google-glog](https://github.com/google/glog) (BSD 3-Clause License)
  * [Google Test](https://github.com/google/googletest) (BSD 3-Clause License)
  * [LevelDB](https://github.com/google/leveldb) (BSD 3-Clause License)
  * [librime](https://github.com/rime/librime) (BSD 3-Clause License)
  * [marisa-trie](https://github.com/s-yata/marisa-trie) (BSD 2-Clause License, LGPL 2.1)
  * [OpenCC / 開放中文轉換](https://github.com/BYVoid/OpenCC) (Apache License 2.0)
  * [plum](https://github.com/rime/plum) (GNU Lesser General Public License v3.0)
  * [WinSparkle](https://github.com/vslavik/winsparkle) (MIT License)
  * [yaml-cpp](https://github.com/jbeder/yaml-cpp) (MIT License)
  * [7-Zip](https://www.7-zip.org) (GNU LGPLv2.1+ with unRAR restriction)

問題與反饋
----------

發現程序有 bug，請到 GitHub 反饋
<https://github.com/rime/weasel/issues>

歡迎提交 pull request
<https://github.com/rime/weasel/pulls>

Rime 輸入法（不限於 Windows 平臺）功能、使用方法與配置相關的問題，請反饋到
<https://github.com/rime/home/issues>

聯繫方式
--------

加入聊天室，提出疑問，發表建議或感想
[![Join the chat at https://gitter.im/rime/home](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/rime/home?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

其他技術交流請寄 Rime 開發者 <rimeime@gmail.com>

謝謝！
