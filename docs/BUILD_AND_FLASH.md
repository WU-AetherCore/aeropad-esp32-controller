# 构建与烧录

## 遥控器主固件

硬件目标为ESP32-S3 / 16MB Flash / 8MB OPI PSRAM / RM67162。官方PlatformIO板型名字中的N8不是本工程的最终Flash配置，实际覆盖项在 `platformio.ini`。换硬件先核对引脚、Flash和PSRAM模式。

```powershell
python -m pip install platformio==6.1.18
pio run -e aeropad
pio run -e aeropad -t upload --upload-port COM11
pio device monitor -p COM11 -b 115200
```

COM11只是本地遥控器端口。运行 `pio device list` 找自己的设备；烧录时关闭占用该端口的串口助手/监视器。下载模式若不能自动进入，按板子的BOOT/RESET操作要求进入，不把“找不到可执行文件”当作硬件连接失败。

Windows脚本查找 `%USERPROFILE%\.platformio\penv\Scripts\pio.exe`。该路径不存在时使用已安装的 `pio` 命令，或先安装PlatformIO的标准环境：
```powershell
.\build.cmd
.\flash.cmd
.\monitor.cmd
```

## CLion

打开仓库根目录，首次先完成PlatformIO构建，安装ESP32交叉工具链。选择 **AeroPad** CMake preset，等待加载完成：
```powershell
cmake --preset AeroPad
cmake --build --preset firmware
cmake --build --preset upload
```

- 编译：构建 `firmware`；烧录：构建 `upload`。
- `upload` 是构建目标，不是Windows可执行程序；不要把 `cmake-build-*/upload` 或 `firmware` 填入“可执行文件”栏。
- 内置共享运行配置使用 `cmd.exe` 调用脚本；源码索引目标 `code_index` 不用于烧录。
- CMake配置目前面向Windows CLion；Linux/macOS直接使用PlatformIO命令。
- 确认成功输出包括 `SUCCESS` 和烧录的 `Hash of data verified`。

## 独立接收端

先读 [接收端教程](RECEIVER_INTEGRATION.md)。接收板不是遥控器，应单独选择端口。COM6仅为以下示例占位，按实际设备修改：
```powershell
pio run -d examples/AeroPad-BLE-Receiver
pio run -d examples/AeroPad-BLE-Receiver -t upload --upload-port COM6
pio run -d examples/Serial-Control-Receiver
pio run -d examples/Serial-Control-Receiver -t upload --upload-port COM6
```

前者为BLE-UART桥接板，后者为UART解析板；不需要都刷到同一块板，也不能用它们覆盖遥控器。串口解析板默认只打印控制意图，适配电机驱动后才会执行动作。

## 验证与字形生成

主机C++测试见 `tools/test_*.cpp`；四格式接收器测试：
```powershell
python -m unittest discover -s tools -p test_host_receiver.py
```
协议向量来源为 `tools/export_protocol_vectors.cpp`，不是手工猜CRC。

正常构建不用Pillow。仅重新生成中文位图时：
```powershell
python -m pip install pillow
python tools/rebuild_ble_ui.py
```
