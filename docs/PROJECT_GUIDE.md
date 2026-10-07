# 项目与源码导览

本工程是原《ESP32 万能遥控器》的二次开发；“AeroPad”仅为派生工程的技术标识。账号和署名见 [NOTICE](../NOTICE.md)，不更改已有构建标识、蓝牙名称、NVS键或协议线格式来美化标题。

## 目录职责

|路径|内容|
|---|---|
|`src/main.cpp`|启动、主菜单、子页面、本机游戏及界面调用|
|`src/Keys.*` / `controller_keys.h`|采样、校准、控件引脚|
|`src/Bluetooth.*`|标准BLE HID，六轴、14按钮、方向帽|
|`src/BleModule.*`|遥控器作为BLE客户端，扫描、连接、写数据、接收通知|
|`src/ControlPacket.h`|自定义控制协议发送端，四种编码的事实依据|
|`src/NetworkPortal.*`|热点、配网网页、路由器连接与NVS配置|
|`src/Screen.*` / `rm67162.*`|显示驱动；原源码署名保留|
|`src/generated_ui_text.h`|由仓库OFL字体生成的中文位图|
|`src/LocalGames.h` / `PuzzleGames.h` / `Sokoban*.h`|本机游戏逻辑和固定关卡|
|`examples/`|独立接收端与电脑协议参考程序，不是遥控器主固件|
|`tools/`|字形生成、主机测试、实机调试工具|
|`docs/`|面向用户的说明、协议、验证记录和截图|
|`assets/fonts/` / `LICENSES/`|字体和第三方许可|
|`.github/workflows/`|无硬件的构建和协议测试|

## 当前菜单

- NRF遥控：无人机与四驱车控制（通用接收端）；NRF 调试（模块检测与收发诊断）；NRF 设置（射频、地址、原始数据互通）。
- 本机游戏：贪吃蛇、打砖块、飞机大战、2048、俄罗斯方块、推箱子。
- 网络信息：WiFi管理（热点配网、连接设置与网页数据监测）。
- 蓝牙中心：蓝牙游戏手柄、蓝牙模块控制、发送设置。串口设置在模块控制的设置/设备操作内。
- 系统设置：按键测试、陀螺仪立方体、控件校准、设备监测。

源码遗留注释的分类编号可能跳过删除的“双人对战”；应以实际菜单列表为准。新功能不通过菜单存在与否判断完成度。

## 维护约定

协议变更需更新协议版本、解析器、测试向量和接入说明。引脚变更需同步硬件说明。页面名以生成字形的实际字符串为准。历史记录不覆盖新版本的验证边界。内部标识变更可能影响配对、NVS和CLion配置，本次资料整理不修改这些标识。

NRF 调试界面在 `src/NrfDebugPage.inc`，紧凑中文在 `src/NrfUiText.h`，测试协议在 `src/NrfDebugPacket.h`。菜单入口在 `NRFControl()`。

NRF 设置独立实现于 `src/NrfGenericPage.inc`，NVS配置校验在 `src/NrfGenericConfig.h`。见 [NRF设置说明](NRF_SETTINGS.md)。
