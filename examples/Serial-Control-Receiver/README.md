# UART控制帧解析示例（独立接收板）

另一块ESP32-S3接收v1二进制UART帧，校验CRC/范围、过滤重复seq、计算差速控制意图。当前只打印，不驱动电机。

UART1：RX GPIO18、TX GPIO17、115200/8N1；接模块TX到板RX、共地。按住A允许输出，LY假设向前为负、RX假设右转为正，必须实测确认。输出限到30%；neutral、A松开、启动及300ms有效帧超时均停止。

把 `src/main.cpp` 的 `applyOutputs()` 替换为你的驱动接口。遥控器选择二进制/50ms；详见 [接收端教程](../../docs/RECEIVER_INTEGRATION.md)。

```powershell
pio run -d examples/Serial-Control-Receiver
pio run -d examples/Serial-Control-Receiver -t upload --upload-port COM6
```

COM6是占位，改成接收板实际端口，不能刷入遥控器COM11。这个例子尚未做真实接收板、电机联调。
