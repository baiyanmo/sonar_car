#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>

// 全局变量声明（接收缓冲区）
extern uint8_t Serial_RxPacket[];
extern uint8_t Serial_RxPacket_RxFlag;

// 串口初始化（支持JDY-31蓝牙模块）
void Serial_Init(void);

// 发送相关函数
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);

// 接收相关函数
uint8_t Serial_GetRxFlag(void);
uint8_t Serial_GetRxData(void);
uint8_t Serial_GetRxPacket_RxFlag(void);

#endif
