#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Motor.h"
#include "Serial.h"
#include "encoder.h"
#include "sys.h"
#include "sonar.h"
#include "Servo.h"
#include "Timer.h"
#include "PWM.h"


//定义速度变量
int8_t Speed_L = 10;
int8_t Speed_R = 10;

// 舵机初始位置（直行）
float servo_angle = 90.0f;        // 舵机中间位置（直行）

// 避障相关参数
uint8_t distance_MAX = 25;			  // 前方最大障碍距离
uint16_t sonar_distance_front = 0;    // 前方超声波测距结果
uint16_t sonar_distance_right = 0;    // 右方超声波测距结果
uint16_t sonar_distance_left = 0;     // 左方超声波测距结果
uint8_t avoid_state = 0;              // 避障状态 (0:直行, 1:检测右, 2:检测左, 3:左转, 4:停止, 5:恢复)
uint16_t avoid_counter = 0;           // 避障计数器
uint8_t detection_step = 0;           // 多方向检测步骤

// 避障决策参数
uint8_t can_go_left = 0;              // 左边是否可通过
uint8_t can_go_right = 0;             // 右边是否可通过

// 用于定时控制
uint16_t pid_control_flag = 0;  // PID控制标志

/**
  * 函    数：定时器初始化（10ms中断用于PID计算）
  * 参    数：无
  * 返 回 值：无
  */
void Timer3_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 启用TIM3时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    // TIM3配置：10ms中断（100Hz）
    TIM_TimeBaseStructure.TIM_Period = 9;
    TIM_TimeBaseStructure.TIM_Prescaler = 35999;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * 函    数：TIM3中断处理函数（10ms）
  * 参    数：无
  * 返 回 值：无
  */
void TIM3_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        Motor_ClosedLoop_Control();
				
			if(avoid_state == 3)
		{
			avoid_counter++;
				
			// 转向100ms后进入恢复状态
			if (avoid_counter > 160)  // 10 * 100ms = 1000ms
			{
				avoid_state = 5;
				avoid_counter = 0;
			}
		}
    } }

int main(void)
{
	/*模块初始化*/
	Motor_Init();		//直流电机初始化
	Serial_Init();	//串口初始化
	Serial_SendString("\r\nClosed-Loop Motor Control System Started\r\n");
	MY_NVIC_PriorityGroupConfig(2);	//设置中断分组
	Servo_Init();
	Encoder_Init_TIM2();            //初始化编码器2
	Encoder_Init_TIM4();            //初始化编码器4
	
	Timer_Init();
	Sonar_Init();
	// 设置电机目标速度
	Motor_SetTargetSpeed(10,10);  // 左右电机都设置为50的速度值
	Servo_SetAngle1(servo_angle);	
	Timer3_Init();  	//初始化定时器3（PID控制）
	  //设置舵机初始角度
	Delay_ms(500);
	
	while (1)
	{
		// 软件PWM已在TIM1中断中更新，此处不再需要
		
		// 根据避障状态执行不同的动作
		switch (avoid_state)
		{
			case 0:  // 直行 - 检测前方
				servo_angle = 90.0f;   // 舵机保持中间位置（前方）
			
				// 设置舵机角度
				Servo_SetAngle1(servo_angle);
			
				Motor_SetTargetSpeed(Speed_L, Speed_R);  // 直行速度
//				Motor_SetLspeed(Speed_L);
//				Motor_SetRspeed(Speed_R);
				
				// 读取超声波距离
				sonar_distance_front = Sonar_GetValue();  // 获取前方距离值
				
				// 前方有障碍物则进入多方向检测
				if (sonar_distance_front > 0 && sonar_distance_front < distance_MAX)
				{
					avoid_state = 1;    // 转向右侧检测
					detection_step = 0;
					can_go_left = 0;
					can_go_right = 0;
				}
				break;
				
			case 1:  // 检测右方		
				Motor_SetTargetSpeed(0, 0);  // 停止移动
				
				servo_angle = 0.0f;   // 舵机向右转，检测右方
				Servo_SetAngle1(servo_angle);
				
				detection_step++;
				if (detection_step == 1)  // 第一次进入，等待舵机转动
				{
					Delay_ms(200);  // 等待舵机转动到位
				}
				else if (detection_step >= 3)  // 第三次循环再测距
				{
					sonar_distance_right = Sonar_GetValue();
					avoid_state = 2;
					detection_step = 0;
				}
				break;
				
			case 2:  // 检测左方
				Motor_SetTargetSpeed(0, 0);  // 停止移动
				
				servo_angle = 180.0f;  // 舵机向左转，检测左方
				Servo_SetAngle1(servo_angle);
				
				detection_step++;
				if (detection_step == 1)  // 第一次进入，等待舵机转动
				{
					Delay_ms(200);  // 等待舵机转动到位
				}
				else if (detection_step >= 3)  // 第三次循环再测距
				{
					sonar_distance_left = Sonar_GetValue();
					
					
					// 做出避障决策
					if (sonar_distance_left > distance_MAX)
					{
						avoid_state = 3;  // 左转避障
					}
					else if (sonar_distance_right > distance_MAX)
					{
						avoid_state = 3;  // 右转避障
					}
					detection_step = 0;
					avoid_counter = 0;
				}
				break;
				
			case 3:  // 转向避障
				// 如果左边可通过则左转，否则右转
				if (sonar_distance_left > distance_MAX)
				{
					servo_angle = 135.0f;  // 舵机向左转
					// 设置舵机角度
					Servo_SetAngle1(servo_angle);
					Motor_SetTargetSpeed(0, Speed_R);  // 右速保持，左速降低，实现左转
//					Motor_SetLspeed(-Speed_L);
//					Motor_SetRspeed(Speed_R);
				}
				else
				{
					servo_angle = 45.0f;   // 舵机向右转
					// 设置舵机角度
					Servo_SetAngle1(servo_angle);
					Motor_SetTargetSpeed(Speed_L, 0);  // 左速保持，右速降低，实现右转
//					Motor_SetLspeed(Speed_L);
//					Motor_SetRspeed(-Speed_R);
				}
				
//				avoid_counter++;
//				
//				// 转向600ms后进入恢复状态
//				if (avoid_counter > 4)  // 12 * 50ms = 600ms
//				{
//					avoid_state = 5;
//					avoid_counter = 0;
//				}
				break;
				
			case 4:  // 停车 - 前左右都有障碍
				servo_angle = 90.0f;   // 舵机归正
				Motor_SetTargetSpeed(0, 0);  // 停车
//				Motor_SetLspeed(0);
//				Motor_SetRspeed(0);
				
//				avoid_counter++;
				
//				// 停车2秒后尝试继续检测
//				if (avoid_counter > 40)  // 40 * 50ms = 2000ms
//				{
//					avoid_state = 0;
//					avoid_counter = 0;
//					obstacle_detected = 0;
//				}
				break;
				
			case 5:  // 恢复直行
				servo_angle = 90.0f;   // 舵机归正
				Motor_SetTargetSpeed(Speed_L, Speed_R);  // 恢复直行
//				Motor_SetLspeed(Speed_L);
//				Motor_SetRspeed(Speed_R);
				
				avoid_counter++;
				
				// 恢复200ms后回到直行状态
				if (avoid_counter > 4)  // 4 * 50ms = 200ms
				{
					avoid_state = 0;
					avoid_counter = 0;
				}
				break;
		}
		
		
//		// 读取编码器数据并显示（用于调试）
		int encoder_value_left = Read_Encoder(2);   // 左轮编码器
		int encoder_value_right = Read_Encoder(4);   // 左轮编码器
		
		// 显示调试信息
		
	Serial_Printf("LeftSpeed: %d\r\n", encoder_value_left);
	Serial_Printf("RightSpeed: %d\r\n", encoder_value_right);
	Serial_Printf("distance: %d\r\n",sonar_distance_front);
	
		
		Delay_ms(50);  // 50ms循环周期 
	}
}




