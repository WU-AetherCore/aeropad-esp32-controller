# 原作者、来源与二次开发说明

|角色|按证据保留的名称|依据|
|---|---|---|
|原项目发布账号|bukaiyuan|[立创开源硬件平台原项目](https://oshwhub.com/bukaiyuan/ESP32-hang-mu-yao-kong-qi) 的地址账号|
|原系统源码署名|bilibili-黑人黑科技|原始 `1.ControllerSystem.ino`、`2.receiver_tank.ino` 文件头；原项目说明亦提及“黑人黑科技”|
|原项目名称|ESP32 万能遥控器|原项目公开页面标题|
|本派生工程维护账号|WU-AetherCore|本 GitHub 仓库所属账号|
|本派生工程内部标识|AeroPad|二次开发目录、CMake preset、BLE 名称中的标识；不是原作者身份|

此前资料将 `bukaiyuan` 写成“卜开元”，没有可核对的姓名依据，现已删除该写法。账号、源码署名和真实姓名不能擅自等同，不推断两种署名的身份对应关系。

本工程是对原遥控器系统的复刻与二次开发，不是原作者官方版本，不表示原作者认可这些修改。原作者代码、引脚定义、图标、原字库及硬件设计的权利仍归原作者或相应权利人；原始文件头保留，维护者不得用自己的署名覆盖。

原项目页面标注 GPL 3.0。本派生软件按 GPL-3.0-only 发布，全文见 [LICENSE](LICENSE)。字体、显示驱动与依赖的各自来源/许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。仓库没有打包原硬件设计文件。

WU-AetherCore 维护的改动包括 CLion/PlatformIO 工程化、校准与掉电保存、中文界面与刷新、BLE HID、BLE-UART 控制和回显、自定义 v1 控制协议、WiFi 管理、设备监测、本机游戏、接收端示例及测试。具体修改见 [CHANGELOG.md](CHANGELOG.md)。

新增中文位图使用 Noto Sans CJK SC（SIL OFL 1.1）；继承的 `chinese_32` 和原图标明确作为原项目资源保留。推箱子参考开源 Sokoban 的规则与设计说明，本项目固定关卡为自生成布局，未复制该项目的地图、图形或声音。

WHEELTEC/轮趣科技 C30D协议兼容部分依据用户持有的厂商接收程序资料独立实现，厂商程序及协议资料的原作者为WHEELTEC；本仓库不复制或重新许可该厂商固件，也不是厂商官方发布。协议来源与适用接口见[蓝牙协议遥控说明](docs/BLUETOOTH_PROTOCOL_CONTROL.md)。
