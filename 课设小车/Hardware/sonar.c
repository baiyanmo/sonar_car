#include "stm32f10x.h"
#include "Delay.h"
#include "Timer.h"

uint16_t sonar_time_counter = 0;	//超声波用时间（供TIM1中断使用）
int distance = 0;					//路程距离


void Sonar_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	//Trig初始化，用于产生超声波
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						
	
	//Echo初始化，用于接收反射回波
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	//让Trig引脚初始为低电平
	GPIO_ResetBits(GPIOA, GPIO_Pin_8);				
}

void Sonar_Start(void)
{
	//给Trig引脚，输出脉冲需要保持10us
	GPIO_SetBits(GPIOA,GPIO_Pin_8);
	Delay_us(45);
	GPIO_ResetBits(GPIOA, GPIO_Pin_8);
	
	sonar_time_counter = 0;
}

//读取超声波距离
uint16_t Sonar_GetValue(void)
{
	Sonar_Start();
	Delay_ms(100);  // 等待超声波测量完成
	
	// 限制最大值，防止溢出
	if(sonar_time_counter > 235)
	{
		sonar_time_counter = 235;
	}
	
	// 计算距离(cm)：时间(100us单位) * 0.0001s * 34000cm/s / 2
	return ((sonar_time_counter * 0.0001f) * 34000) / 2;
}
