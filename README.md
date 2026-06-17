# Sonar Car (Ultrasonic Obstacle-Avoidance Smart Car)

STM32-based ultrasonic obstacle-avoidance and line-tracking smart car with C# Winform upper computer, PCB design files included.

[![Stars](https://img.shields.io/github/stars/baiyanmo/sonar_car?style=flat)](https://github.com/baiyanmo/sonar_car/stargazers)
[![STM32](https://img.shields.io/badge/STM32-F103-blue)](https://www.st.com)
[![Keil](https://img.shields.io/badge/Keil-uVision5-green)](https://www.keil.com)

## Hardware
- MCU: STM32F103 series
- Motor Driver: L298N
- Ultrasonic: HC-SR04 (obstacle detection and distance measurement)
- Infrared: Multi-channel reflective sensors (line tracking)
- Chassis: DC geared motors with encoders

## PCB Design
- PCB design file included: XiaoCheZhuBan.eprj2 (LCEDA format, ready for fabrication)
- Integrated STM32 minimum system, motor interface, sensor headers, power management
- 7.4V Li-ion battery support, onboard 5V/3.3V regulation

## Software
- Sensor drivers: ultrasonic timing control, IR sensor reading
- Motor control: PWM speed control, direction control
- Obstacle avoidance: real-time distance-based path planning
- Line tracking: IR array deviation calculation with differential steering

## Upper Computer (Winform)
C# Winform application in winform/ directory for:
- Real-time sensor data monitoring (distance, IR status)
- Remote control commands
- Debugging and parameter tuning

## Development
- IDE: Keil uVision5 (MDK-ARM)
- Library: STM32 Standard Peripheral Library

## Quick Start
1. Open Project.uvprojx in Keil (in KeSheXiaoChe/ folder)
2. Compile and flash to STM32 via ST-Link/DAP-Link
3. Power on and observe line-tracking and obstacle-avoidance behavior
4. (Optional) Run Winform upper computer with serial/Bluetooth module

## License
MIT