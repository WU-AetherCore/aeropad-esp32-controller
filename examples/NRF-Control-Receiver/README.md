# 通用 NRF 控制接收端（ESP32-S3）

同一套接收程序有无人机和四驱车两个构建环境。只烧录到另一块接收板，不是遥控器 COM11。

```powershell
# 在本目录，使用已安装 PlatformIO 的 Python
python -m platformio run -e car
python -m platformio run -e drone
# 实际端口按接收板修改，例如 COM6
python -m platformio run -e car -t upload --upload-port COM6
```

NRF默认频道76、1Mbps、最低功率，CE43/CSN42/SCK12/MISO13/MOSI11。具体接线、SBUS通道、差速混控、协议及失联行为见[完整接入说明](../../docs/NRF_CONTROL.md)。

`src/ReceiverConfig.h` 集中管理接线。四驱车电机输出默认关闭，核对接线后设置 `MOTOR_OUTPUT_ENABLED=true`。无人机环境通过GPIO17输出反相SBUS给飞控；需要在飞控中配置接收协议和通道。

主机安全状态机和协议测试、两个环境编译验证不等于真实飞控或电机联调。
