# 第三方来源与许可

|组件|来源|许可与说明|
|---|---|---|
|遥控器原系统、引脚与图标/原字库|原项目发布账号 bukaiyuan；源码文件头署名 bilibili-黑人黑科技，[原项目](https://oshwhub.com/bukaiyuan/ESP32-hang-mu-yao-kong-qi)|原项目标注 GPL 3.0；保留原署名，不推断账号对应的真实姓名|
|RM67162 / AMOLED 驱动生态|[Xinyuan-LilyGO/T-Display-S3-AMOLED](https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED)，包含 nikthefix 注释|MIT，Copyright (c) 2023 Xinyuan-LilyGO；原许可副本 LICENSES/LILYGO-MIT.txt|
|NotoSansCJKsc-Regular.otf 与新增中文位图|[notofonts/noto-cjk](https://github.com/notofonts/noto-cjk)|SIL Open Font License 1.1，完整原许可见 assets/fonts/OFL.txt；字体及字形不改称GPL字体|
|TFT_eSPI 2.5.43|Bodmer|包含 MIT / FreeBSD 等原作者声明，保留库自身许可|
|RF24 1.4.8|nRF24|GPL-2.0-only（library.json及RF24.h明确版本2），不改许可|
|Bounce2 2.72.0|Thomas Fredericks|MIT，保留库自身许可|
|MPU6050_tockn 1.5.2|tockn|MIT，保留库自身许可|
|Adafruit MCP23017 2.3.2|Adafruit|BSD，保留库自身许可|
|Adafruit BusIO 1.17.4|Adafruit|MIT，保留库自身许可|
|ESP32-BLE-Gamepad 0.7.3|lemmingDev|MIT，保留库自身许可|
|NimBLE-Arduino 2.5.1|h2zero / Apache NimBLE|Apache-2.0，保留库自身许可|
|Arduino-ESP32 2.0.11、ESP-IDF/toolchain|Espressif 与其上游|各自 LGPL/Apache/GPL 等许可；由 PlatformIO 安装，不将这些依赖改许可|

依赖没有复制到本仓库，构建时由 PlatformIO 下载。库许可的完整文本及嵌入的其他作者声明，以各组件安装目录中的 LICENSE/COPYING/NOTICE 和文件头为准。软件派生工程 GPL 不替代这些组件的原版权声明。没有将硬件设计文件打包进本仓库。

当前RF24是GPL-2.0-only，而原项目标注GPL-3.0；组合二进制再分发存在许可兼容性问题。本次只发布本派生工程源代码、文档及构建测试，不发布链接RF24的固件二进制；组合固件再分发前须取得适当授权、兼容例外或改用兼容实现，不将其伪称为已解决。
