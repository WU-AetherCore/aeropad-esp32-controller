# 资料索引
首页概览在 [仓库 README](../README.md)。初次使用按“构建 → 操作 → 接收端 → 协议”阅读。

|资料|用途|
|---|---|
|[项目与源码导览](PROJECT_GUIDE.md)|菜单、源码职责、内部名称|
|[构建与烧录](BUILD_AND_FLASH.md)|遥控器与两种接收端分别编译、正确选择串口|
|[硬件与操作](HARDWARE_AND_CONTROLS.md)|GPIO、校准、进入和返回按键|
|[接收端接入](RECEIVER_INTEGRATION.md)|从BLE/UART到解析、控制映射、失联停止|
|[控制协议](CONTROL_PROTOCOL.md)|权威字节布局、四格式、CRC、按钮bit、测试向量|
|[排查指南](TROUBLESHOOTING.md)|乱码、未动作、波特率、断连、重复包|
|[WiFi管理](WIFI_MANAGEMENT.md)|热点配网、开机设置、数据查看|
|[设备监测](DEVICE_MONITOR.md)|百分比含义与刷新|
|[游戏玩法](LOCAL_GAMES.md)|六款本机游戏|
|[验证状态](VALIDATION.md)|当前与历史的测试边界|
|[推箱子专项验证](SOKOBAN_VALIDATION.md)|十关、庆祝、自动切关|
|[协议测试向量](protocol_vectors.json)|发送端实际编码的可机读样例|
|[界面截图](images/README.md)|最新截图来源|
|[更新记录](../CHANGELOG.md)|按日期整理的改动|

`BLE_CONTROL_PROTOCOL.md` 和 `LATEST_UPDATE.md` 保留为兼容旧链接的入口；历史验证独立存放于 `history/`，不代表当前全部功能已实测。

- [NRF 调试](NRF_DEBUG.md)：本机检查、射频参数、专用测试包与对端联调。

- [NRF 无人机与四驱车控制](NRF_CONTROL.md)：遥控操作、通用接收端、SBUS、差速混控与32字节协议。

- [NRF设置](NRF_SETTINGS.md)：可保存的射频参数、地址、原始数据与江协参考程序预设。

- [无人机与四驱车自定义协议和预设](NRF_CUSTOM_PROFILES.md)：逐字节映射、5 个独立槽位、保存/加载/删除。

- [蓝牙协议遥控与C30D](BLUETOOTH_PROTOCOL_CONTROL.md)：内置APP/ROS、五个预设、自定义包、停止帧、BLE兼容范围。
