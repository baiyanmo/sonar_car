#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>

// 接收数据缓冲区
uint8_t Serial_RxData;			// 接收数据变量
uint8_t Serial_RxFlag;			// 接收标志位

uint8_t Serial_RxPacket[100];	// 接收数据包数组，留出100个位置
uint8_t Serial_RxPacket_RxFlag;	// 接收数据包标志位

/**
  * 函    数：串口初始化（支持JDY-31蓝牙模块）
  * 参    数：无
  * 返 回 值：无
  * 说    明：配置为9600波特率，支持收发双向通信
  *           连接方式：JDY-31 TXD -> STM32 PB11(RX)
  *                    JDY-31 RXD -> STM32 PB10(TX)
  *                    JDY-31 VCC -> 3.3V/5V
  *                    JDY-31 GND -> GND
  */
void Serial_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//开启USART3的时钟（注意是APB1）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//开启GPIOB的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	// 配置TX引脚 PB10（USART3_TX）
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将PB10引脚初始化为复用推挽输出
	
	// 配置RX引脚 PB11（USART3_RX，接收JDY-31数据）
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;			// 上拉输入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					//将PB11引脚初始化为上拉输入
	
	/*USART初始化*/
	USART_InitTypeDef USART_InitStructure;					//定义结构体变量
	USART_InitStructure.USART_BaudRate = 9600;				//波特率（JDY-31默认9600）
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制，不需要
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	//模式，选择为发送和接收模式
	USART_InitStructure.USART_Parity = USART_Parity_No;		//奇偶校验，不需要
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//停止位，选择1位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长，选择8位
	USART_Init(USART3, &USART_InitStructure);				//将结构体变量交给USART_Init，配置USART3
	
	/*中断配置*/
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);			//开启串口接收数据的中断
	
	/*NVIC配置*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);			//配置NVIC为分组2
	
	NVIC_InitTypeDef NVIC_InitStructure;					//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;		//选择配置NVIC的USART3线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);							//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*USART使能*/
	USART_Cmd(USART3, ENABLE);								//使能USART3，串口开始运行
}

/**
  * 函    数：串口发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART3, Byte);		//将字节数据写入数据寄存器，写入后USART自动生成时序波形
	while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);	//等待发送完成
	/*下次写入数据寄存器会自动清除发送完成标志位，故此循环后，无需清除标志位*/
}

/**
  * 函    数：串口发送一个数组
  * 参    数：Array 要发送数组的首地址
  * 参    数：Length 要发送数组的长度
  * 返 回 值：无
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)		//遍历数组
	{
		Serial_SendByte(Array[i]);		//依次调用Serial_SendByte发送每个字节数据
	}
}

/**
  * 函    数：获取接收标志位
  * 参    数：无
  * 返 回 值：接收标志位，范围：0~1，接收到数据后，标志位置1，读取后自动清零
  */
uint8_t Serial_GetRxFlag(void)
{
	if (Serial_RxFlag == 1)			//如果标志位为1
	{
		Serial_RxFlag = 0;			//自动清零
		return 1;					//返回1
	}
	return 0;						//如果标志位为0，返回0
}

/**
  * 函    数：获取接收到的数据
  * 参    数：无
  * 返 回 值：接收到的数据，范围：0~255
  */
uint8_t Serial_GetRxData(void)
{
	return Serial_RxData;			//返回接收到的数据变量
}

/**
  * 函    数：获取接收数据包标志位
  * 参    数：无
  * 返 回 值：接收数据包标志位，范围：0~1，接收到数据包后，标志位置1
  */
uint8_t Serial_GetRxPacket_RxFlag(void)
{
	if (Serial_RxPacket_RxFlag == 1)	//如果标志位为1
	{
		Serial_RxPacket_RxFlag = 0;		//自动清零
		return 1;						//返回1
	}
	return 0;							//如果标志位为0，返回0
}

/**
  * 函    数：串口发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Serial_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)//遍历字符数组（字符串），遇到字符串结束标志位后停止
	{
		Serial_SendByte(String[i]);		//依次调用Serial_SendByte发送每个字节数据
	}
}

/**
  * 函    数：次方函数（内部使用）
  * 返 回 值：返回值等于X的Y次方
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//设置结果初值为1
	while (Y --)			//执行Y次
	{
		Result *= X;		//将X累乘到结果
	}
	return Result;
}

/**
  * 函    数：串口发送数字
  * 参    数：Number 要发送的数字，范围：0~4294967295
  * 参    数：Length 要发送数字的长度，范围：0~10
  * 返 回 值：无
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i ++)		//根据数字长度遍历数字的每一位
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');	//依次调用Serial_SendByte发送每位数字
	}
}

/**
  * 函    数：使用printf需要重定向的底层函数
  * 参    数：保持原始格式即可，无需变动
  * 返 回 值：保持原始格式即可，无需变动
  */
int fputc(int ch, FILE *f)
{
	Serial_SendByte(ch);			//将printf的底层重定向到自己的发送字节函数
	return ch;
}

/**
  * 函    数：自己封装的prinf函数
  * 参    数：format 格式化字符串
  * 参    数：... 可变的参数列表
  * 返 回 值：无
  */
void Serial_Printf(char *format, ...)
{
	char String[100];				//定义字符数组
	va_list arg;					//定义可变参数列表数据类型的变量arg
	va_start(arg, format);			//从format开始，接收参数列表到arg变量
	vsprintf(String, format, arg);	//使用vsprintf打印格式化字符串和参数列表到字符数组中
	va_end(arg);					//结束变量arg
	Serial_SendString(String);		//串口发送字符数组（字符串）
}

/**
  * 函    数：USART3中断服务函数
  * 参    数：无
  * 返 回 值：无
  * 说    明：接收JDY-31蓝牙模块发送来的数据
  */
void USART3_IRQHandler(void)
{
	static uint8_t RxState = 0;		//定义接收状态变量，0为接收包头，1为接收数据
	static uint8_t pRxPacket = 0;	//定义接收数据包的位置变量
	
	if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)	//判断是否为USART3接收中断
	{
		uint8_t RxData = USART_ReceiveData(USART3);			//读取数据寄存器，存放到接收数据变量
		
		if (RxState == 0)		//如果当前状态为0（接收包头）
		{
			if (RxData == '@' && Serial_RxPacket_RxFlag == 0)	//如果收到包头'@'，并且上一个数据包已经被处理
			{
				RxState = 1;			//进入下一个状态
				pRxPacket = 0;			//数据包位置归零
			}
		}
		else if (RxState == 1)	//如果当前状态为1（接收数据）
		{
			if (RxData == '\r')		//如果收到包尾'\r'
			{
				RxState = 0;				//返回状态0
				Serial_RxPacket[pRxPacket] = '\0';		//添加字符串结束标志
				Serial_RxPacket_RxFlag = 1;				//接收数据包完成，置标志位为1
			}
			else					//如果收到的是数据
			{
				Serial_RxPacket[pRxPacket] = RxData;	//将数据存入数据包数组
				pRxPacket ++;							//数据包位置自增
			}
		}
		
		// 单字节接收模式（用于简单应用）
		Serial_RxData = RxData;		//将接收到的数据保存到全局变量
		Serial_RxFlag = 1;			//置接收标志位为1
		
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);		//清除USART3的RXNE中断标志位
	}
}
