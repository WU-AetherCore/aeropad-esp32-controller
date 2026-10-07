#pragma once
// Match the transmitter's settings. Defaults are shared by both vehicle pages.
constexpr int NRF_CE=43,NRF_CSN=42,NRF_SCK=12,NRF_MISO=13,NRF_MOSI=11;
constexpr int NRF_CHANNEL=76,NRF_POWER=0;
constexpr int SBUS_TX=17; // ESP32-S3 inverted UART -> compatible flight-controller SBUS input.
// TB6612 or equivalent two-channel direction/PWM interface. Verify wiring first.
constexpr bool MOTOR_OUTPUT_ENABLED=false;
constexpr int LEFT_IN1=5,LEFT_IN2=6,RIGHT_IN1=7,RIGHT_IN2=8,LEFT_PWM=9,RIGHT_PWM=10,MOTOR_STBY=4;
constexpr bool LEFT_REVERSE=false,RIGHT_REVERSE=false;
