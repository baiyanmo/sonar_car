#include "stm32f10x.h"                  // Device header
#include "PWM.h"
#include "encoder.h"
/**
  * 函    数：直流电机初始化
  * 参    数：无
  * 返 回 值：无
  */
void Motor_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	/*先初始化PWM，避免引脚冲突*/
	PWM_Init();													//初始化直流电机的底层PWM（PA2-TIM2_CH3, PA3-TIM2_CH4）
	
	/*GPIO初始化 - 方向控制引脚（必须在PWM初始化后）*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;		//PA0、PA1(左电机)、PA4、PA5(右电机)
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA0、PA1、PA4和PA5引脚初始化为推挽输出	
	
	/*初始化所有方向控制引脚为低电平*/
	GPIO_ResetBits(GPIOA, GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_4 | GPIO_Pin_5);
}

/**
  * 函    数：右直流电机设置速度
  * 参    数：Speed 要设置的速度，范围：-100~100
  * 返 回 值：无
  */
void Motor_SetLspeed(int8_t Speed)
{
	if (Speed >= 0)							//如果设置正转的速度值
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_4);	//PA4置高电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_5);	//PA5置低电平，设置方向为正转
		PWM_SetCompare3(Speed);				//PWM设置为速度值
	}
	else									//否则，即设置反转的速度值
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_4);	//PA4置低电平
		GPIO_SetBits(GPIOA, GPIO_Pin_5);	//PA5置高电平，设置方向为反转
		PWM_SetCompare3(-Speed);			//PWM设置为负的速度值，因为此时速度值为负数，而PWM只能给正数
	}
}

/**
  * 函    数：左直流电机设置速度
  * 参    数：Speed 要设置的速度，范围：-100~100
  * 返 回 值：无
  */
void Motor_SetRspeed(int8_t Speed)
{
	if (Speed >= 0)							//如果设置正转的速度值
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_6);	//PA0置高电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_7);	//PA1置低电平，设置方向为正转
		PWM_SetCompare4(Speed);				//PWM设置为速度值
	}
	else									//否则，即设置反转的速度值
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_6);	//PA0置低电平
		GPIO_SetBits(GPIOA, GPIO_Pin_7);	//PA1置高电平，设置方向为反转
		PWM_SetCompare4(-Speed);			//PWM设置为负的速度值，因为此时速度值为负数，而PWM只能给正数
	}
}
// ========== 左电机PID参数 ==========
float speed_target_l =0;// 50.0f;      // 左电机目标速度
float l_enc_read = 0;              // 左编码器读出来的速度
float errorl = 0;                  // 左电机当前误差
float last_errorl = 0;             // 左电机上次误差
float integral_errorl = 0;         // 左电机积分误差
int pwmoutl = 0;                   // 左电机PWM输出

// 左电机PID参数（可根据实际情况调整）
float l_kp = 2.0f;     // 比例系数
float l_ki = 0.5f;     // 积分系数
float l_kd =0;// 0.2f;     // 微分系数

// ========== 右电机PID参数 ==========
float speed_target_r = 0;      // 右电机目标速度
float r_enc_read = 0;              // 右编码器读出来的速度
float errorr = 0;                  // 右电机当前误差
float last_errorr = 0;             // 右电机上次误差
float integral_errorr = 0;         // 右电机积分误差
int pwmoutr = 0;                   // 右电机PWM输出

// 右电机PID参数（可根据实际情况调整）
float r_kp = 2.0f;     // 比例系数
float r_ki = 0.5f;     // 积分系数
float r_kd = 0;//0.2f;     // 微分系数

// PID限幅参数
int pwm_max = 100;                 // PWM最大值
int pwm_min = -100;                // PWM最小值
float integral_max = 50.0f;         // 积分项最大值（防止积分饱和）

/**
  * 函    数：左电机PID控制
  * 参    数：无
  * 返 回 值：pwm输出值
  */
int PID_L(void)
{
    // 读取编码器反馈
    l_enc_read = Read_Encoder(2);
    
    // 计算误差
    errorl = speed_target_l - l_enc_read;
    
    // 积分项累加，防止积分饱和
    integral_errorl += errorl;
    if(integral_errorl > integral_max)
        integral_errorl = integral_max;
    if(integral_errorl < -integral_max)
        integral_errorl = -integral_max;
    
    // PID控制算法：Kp*e(k) + Ki*∑e(k) + Kd*[e(k)-e(k-1)]
    pwmoutl = (int)(l_kp * errorl + 
                    l_ki * integral_errorl + 
                    l_kd * (errorl - last_errorl));
    
    // 保存本次误差用于下次微分计算
    last_errorl = errorl;
    
    // PWM输出限幅
    if(pwmoutl > pwm_max)
        pwmoutl = pwm_max;
    if(pwmoutl < pwm_min)
        pwmoutl = pwm_min;
    
    return pwmoutl;
}

/**
  * 函    数：右电机PID控制
  * 参    数：无
  * 返 回 值：pwm输出值
  */
int PID_R(void)
{
    // 读取编码器反馈
    r_enc_read = Read_Encoder(4);
    
    // 计算误差
    errorr = speed_target_r - r_enc_read;
    
    // 积分项累加，防止积分饱和
    integral_errorr += errorr;
    if(integral_errorr > integral_max)
        integral_errorr = integral_max;
    if(integral_errorr < -integral_max)
        integral_errorr = -integral_max;
    
    // PID控制算法：Kp*e(k) + Ki*∑e(k) + Kd*[e(k)-e(k-1)]
    pwmoutr = (int)(r_kp * errorr + 
                    r_ki * integral_errorr + 
                    r_kd * (errorr - last_errorr));
    
    // 保存本次误差用于下次微分计算
    last_errorr = errorr;
    
    // PWM输出限幅
    if(pwmoutr > pwm_max)
        pwmoutr = pwm_max;
    if(pwmoutr < pwm_min)
        pwmoutr = pwm_min;
    
    return pwmoutr;
}

/**
  * 函    数：电机闭环速度控制输出
  * 参    数：无
  * 返 回 值：无
  */
void Motor_ClosedLoop_Control(void)
{
    // 执行PID计算并输出
    PID_L();
    PID_R();
    
    // 控制左电机
    Motor_SetLspeed(pwmoutl);
    
    // 控制右电机
    Motor_SetRspeed(pwmoutr);
}

/**
  * 函    数：设置目标速度
  * 参    数：speed_l-左电机目标速度, speed_r-右电机目标速度
  * 返 回 值：无
  */
void Motor_SetTargetSpeed(float speed_l, float speed_r)
{
    speed_target_l = speed_l;
    speed_target_r = speed_r;
    
    // 重置积分项，防止突然改变速度时积分饱和导致电机卡死
    if(speed_l == 0) {
        integral_errorl = 0;
        last_errorl = 0;
    }
    if(speed_r == 0) {
        integral_errorr = 0;
        last_errorr = 0;
    }
}
