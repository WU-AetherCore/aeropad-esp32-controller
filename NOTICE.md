# 原作者与修改说明

本项目 AeroPad ESP32 Controller 是对 **卜开元（嘉立创开源平台账号 bukaiyuan）** 的《ESP32 万能遥控器》的复刻与二次开发，原系统框架源码署名 **bilibili-黑人黑科技**。原作者身份依据本地原始源码文件头与原项目地址记录，不将其作品署名改为本项目维护者。

原项目：https://oshwhub.com/bukaiyuan/ESP32-hang-mu-yao-kong-qi

该页面标注 **GPL 3.0**。本仓库派生软件按 **GPL-3.0-only** 发布，完整协议见 [LICENSE](LICENSE)。原硬件设计、原图标、原代码及其他第三方作品的版权仍归其各自作者；本仓库不是原作者官方版本，也不代表原作者认可本次修改。

2026-10-06，WU-AetherCore 的二次开发包含：CLion/PlatformIO 工程化、ADC 校准与 NVS 保存、统一中文界面与局部刷新、蓝牙 HID 状态更新、BLE-UART 扫描/连接/回显、四种控制数据格式与协议文档、模块内串口设置、菜单名称滚动、立方体姿态及主机测试。原有文件头保留，后续维护者应保留来源、许可和修改说明。

显示驱动源于 LILYGO T-Display-S3-AMOLED 生态及原工程，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。新增静态中文字形改用 Noto Sans CJK SC（SIL OFL 1.1），不分发 Windows 黑体字体或从该字体生成的新增 UI 字形。继承的 chinese_32 与原图标作为原项目资产保留，并明确来源。
