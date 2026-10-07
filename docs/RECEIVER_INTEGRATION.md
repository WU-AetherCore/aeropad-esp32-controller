# 接收端接入：从遥控器数据到设备动作

本教程针对“蓝牙模块控制”的自定义v1协议。无人机/四驱车采用独立 [NRC1协议与通用接收端](NRF_CONTROL.md)；HID游戏手柄也不发送本教程的帧。

## 1. 选择架构

```mermaid
flowchart LR
    A[遥控器 BLE模块控制] -->|NUS或FFE0写入| B[BLE模块或ESP32桥接板]
    B -->|UART字节流| C[目标MCU]
    C --> D[分帧 校验 解码]
    D --> E[控制映射与失联判断]
    E --> F[电机驱动 舵机或目标系统]
    C -->|状态回传| B
    B -->|BLE通知| A
```

|方案|使用的程序|还需要实现什么|
|---|---|---|
|已有FFE0/NUS透传模块|目标MCU用下方解析器|配置模块UART；目标引脚、电机/舵机驱动和停止动作|
|另一块ESP32-S3做无线桥|[BLE-UART桥接示例](../examples/AeroPad-BLE-Receiver/README.md)|桥接板只转发字节，UART目标仍需解析和驱动|
|另一块ESP32-S3解析UART|[串口接收示例](../examples/Serial-Control-Receiver/README.md)|替换 `applyOutputs()`，当前只打印控制意图|
|电脑先验证|[Python四格式解析器](../examples/host_receiver.py)|选择接收模块/适配器端口，不需要执行器|
|STM32等其他MCU|[可移植C++11解析器](../examples/common/ControlReceiver.h)|UART/DMA缓存、毫秒时钟、驱动适配|

已有透传模块不用烧本仓库的桥接固件。遥控器自己的COM11用于烧录、调试、通知回显，不是无线接收板的串口。

## 2. 接线和设置

模块TX接目标RX，模块RX接目标TX，共地，核对供电和逻辑电平。示例使用3.3V逻辑UART，不是RS-232。UART双方设置相同波特率、8数据位、无校验、1停止位（8N1）。

接收板示例GPIO18=RX、GPIO17=TX、115200/8N1；按你的板子修改。遥控器显示也使用17/18，不能把接收板接线复制到遥控器上。

遥控器：蓝牙中心 → 蓝牙模块控制 → A搜索 → 选设备O连接 → 设备操作中选择开始遥控。初次联调在发送设置选择**二进制、50ms**。设置入口也可从蓝牙中心或模块设置进入。发送格式与接收回显互不影响。

`WUFUDONG` 是广播名称，不足以识别芯片或AT命令。没有NUS参数扩展特征的模块，应按模块自己的资料配置UART，不能靠BLE名称自动匹配电气波特率。

## 3. 在电脑先确认字段

接收模块UART可接3.3V USB-UART适配器。假定电脑接收口是COM6：
```powershell
python -m pip install pyserial
python examples/host_receiver.py --port COM6 --baud 115200 --format binary --timeout-ms 300
```

按发送设置改为 `--format json`、`hex` 或 `text`。解析器支持分片、粘包、限长和二进制CRC重同步。以下是解释用途的例子，不是宣称真实采样：

```json
{"frame":{"seq":12,"lx":0,"ly":-60,"rx":20,"ry":0,"kl":0,"kr":0,"buttons":1,"ax":0,"ay":0,"neutral":0},"motor_intent":[24,12]}
```

LY=-60假设向前，RX=20假设右转，buttons=1为A按住。物理方向必须实测确认。v1轴值是-100～100，完整字段与18个按钮bit见 [协议](CONTROL_PROTOCOL.md)。

## 4. 写MCU接收循环

二进制每帧20字节；解析器的 `axes[0..7]` 为LX、LY、RX、RY、KL、KR、AX、AY。不要直接把字节强转为C结构体，避免填充、类型大小和端序差异。

```cpp
#include "ControlReceiver.h"
ControlReceiver::Stream parser;
ControlReceiver::Watchdog watchdog;
ControlReceiver::Frame frame;
void receivedByte(uint8_t byte, uint32_t nowMs) {
    if (!parser.feed(byte, frame)) return;
    watchdog.accepted(nowMs);  // 仅完整有效帧可更新
    if (frame.neutral) { stopVehicle(); return; }
    int left, right;
    ControlReceiver::differential(frame, left, right);
    setMotorPercent(left, right);
}
void controlTick(uint32_t nowMs) {
    if (watchdog.expired(nowMs, 300)) stopVehicle();
}
```

这是接入骨架，`stopVehicle()` 与 `setMotorPercent()` 由你实现。完整可编译版本在 [示例源码](../examples/Serial-Control-Receiver/src/main.cpp)，额外处理启动停止、seq重复过滤与超时时清半帧。

UART中断/DMA只缓存字节，在任务/主循环逐字节喂解析器；一次DMA回调不保证是一包。不要在中断里打印大量日志、调用BLE或启动执行器。

## 5. 映射设备动作

|目标|示例映射|适配点|
|---|---|---|
|两路独立电机差速车|油门=-LY；转向=RX；左=油门+转向，右=油门-转向|限幅后缩放，方向实测；示例A按住才输出|
|转向舵机小车|LY映射车速，RX映射舵机位置|舵机中点/两端脉宽、电机停止值依驱动器决定|
|旋钮参数|KL/KR映射参数范围|不是原始ADC；释放标志优先于参数值|
|按键动作|`buttons & (1u << bit)`|A为bit0，O为bit3；可使用边沿触发而非每包重复动作|
|已有飞控无人机|将字段转为飞控接受的输入|飞控协议、解锁、控制闭环未实现；不能直接把20字节当飞控标准输入|

例：LY=-60、RX=20 → 油门60、转向20 → 左80/右40 → 限制至30%后左24%/右12%。修改 [differential()](../examples/common/ControlReceiver.h) 可更换轴、反转符号或调整限幅；修改 `applyOutputs()` 接入真实驱动。本示例不是驱动板通用固件。

按钮需边沿动作时保存上一帧：`newPress = currentButtons & ~previousButtons`，再按bit判断。失联时清除上一帧状态，重新建立明确的允许输出条件。

## 6. 释放、失联和反馈

- 启动、neutral=1、A松开、300ms无新的有效控制帧时，参考程序停止。
- 只有完整、范围正确、CRC正确的帧才能刷新有效时间；重复seq不能维持旧运动。seq在255后回0，正常回绕不是错误。
- 推荐20/50ms发送配合300ms参考看门狗。选择500/1000/2000ms会在帧间超时；按用途改周期/超时策略，不要删除失联判断。
- 桥接示例的5000ms只是“透传层未收到BLE写入”检测，任意片段可更新，不替代目标MCU的有效包看门狗。
- 退出/断连时遥控器尝试发送释放，但不保证无线丢包时能收到，所以目标必须独立判断失联。
- 对方回传UART字节 → BLE通知 → 遥控器回显。可自定义 `ACK seq=12 left=24 right=12\n`，但v1没有通用执行ACK；TX不是执行确认。

先以日志和低速控制意图确认方向、按键和失联，再适配真实驱动。接收板/电机硬件联调未验证，主机测试和编译不等于设备已动作。
