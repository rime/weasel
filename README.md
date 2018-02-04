【小狼毫】輸入法
================

基於 中州韻輸入法引擎／Rime Input Method Engine 等開源技術

式恕堂 版權所無

[![Build status](https://ci.appveyor.com/api/projects/status/github/rime/weasel?svg=true)](https://ci.appveyor.com/project/lotem/weasel)

授權條款：GPLv3

項目主葉：http://rime.im/

您可能還需要 RIME 用於其他操作系統的發行版：
  * 【中州韻】用於 Linux + IBus 輸入法框架
  * 【鼠鬚管】用於 Mac OS X 10.7+（64位）

安裝輸入法
----------

本品適用於 Windows XP SP3, 32/64位 Windows 7

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
    - 數據來源於 CC-CEDICT、Android 拼音、新酷音、opencc 等開源項目
    - 維護者 佛振 瑾昀
  * 【注音／地球拼音】
    - 維護者 佛振 瑾昀
  * 【倉頡五代】
    - 發明人 朱邦復先生
    - 碼表源自 www.chinesecj.com
    - 構詞碼表作者 惜緣
    - 輸入方案作者 佛振
  * 【五笔86】
    - 發明人 王永民先生
    - 碼表源自 ibus-table
  * 【粵拼】
    - 採用《香港語言學學會粵語拼音方案》
    - http://www.lshk.org/
    - 碼表源自 ibus-table
  * 【上海吳語】【蘇州吳語】
    - 採用《吳語拉丁式注音法》
    - http://input.foruto.com/wu/method.html
    - 作者 上海閒話abc、吳語越音、寒寒豆
  * 【中古全拼／三拼】
    - 採用《廣韻》音系的中古漢語拼音，亦稱「古韻羅馬字」。
    - 韻典網·廣韻 http://ytenx.org/kyonh/
    - https://zh.wikipedia.org/wiki/User:Polyhedron/中古漢語拼音
    - 作者 Polyhedron
  * 【X-SAMPA】
    - 國際音標輸入法
    - https://zh.wikipedia.org/wiki/X-SAMPA
    - 作者 Patrick Tschang、佛振

### 程序設計：
  * 佛振 <chen.sst@gmail.com>
  * 鄒旭 <zouivex@gmail.com>
  * BYVoid <byvoid.kcp@gmail.com>
  * nameoverflow <i@hcyue.me>
  * wishstudio <wishstudio@gmail.com>

### 美術：
  * 圖標設計 Patricivs
  * 配色方案 Aben、P1461、Patricivs、skoj、佛振、五磅兔

### 本品引用了以下開源軟件：
  * [Boost C++ Libraries] (http://www.boost.org/) (Boost Software License)
  * [google-glog] (https://github.com/google/glog) (BSD 3-Clause License)
  * [Google Test] (https://github.com/google/googletest) (BSD 3-Clause License)
  * [LevelDB] (https://github.com/google/leveldb) (BSD 3-Clause License)
  * [marisa-trie] (https://github.com/s-yata/marisa-trie) (BSD 2-Clause License, LGPL 2.1)
  * [OpenCC / 開放中文轉換] (https://github.com/BYVoid/OpenCC) (Apache License 2.0)
  * [WinSparkle] (https://github.com/vslavik/winsparkle) (MIT License)
  * [yaml-cpp] (https://github.com/jbeder/yaml-cpp) (MIT License)

問題與反饋
----------

發現程序有BUG，或建議，或感想，請到Rime項目網站 [反饋] (https://github.com/rime/weasel/issues) 。

已知問題
--------

  * 不支持 Windows 命令行和全屏遊戲

聯繫方式
--------

技術交流請寄 Rime 開發者 <rimeime@gmail.com>

謝謝！
