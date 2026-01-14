#include "stm32f10x.h"                  // Device header

/**
  * 函    数：PWM初始化
  * 参    数：无
  * 返 回 值：无
  */
void PWM_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);			//开启GPIOA的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);			//开启AFIO的时钟，复用功能必需
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;			//PA2(TIM2_CH3)和PA3(TIM2_CH4)
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);							//将PA2和PA3引脚初始化为复用推挽输出	
																	//受外设控制的引脚，均需要配置为复用模式
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;                 //计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;               //预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            //重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);             //将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元
	
	/*输出比较初始化*/ 
	TIM_OCInitTypeDef TIM_OCInitStructure;							//定义结构体变量
	TIM_OCStructInit(&TIM_OCInitStructure);                         //结构体初始化，若结构体没有完整赋值                                             //避免结构体初值不确定的问题
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //输出比较模式，选择PWM模式1
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //输出极性，选择为高，若选择极性为低，则输出高低电平取反
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;								//初始的CCR值
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);                        //将结构体变量交给TIM_OC3Init，配置TIM2的输出比较通道3
	TIM_OC4Init(TIM2, &TIM_OCInitStructure); 
	
	/*TIM使能*/
	TIM_Cmd(TIM2, ENABLE);			//使能TIM2，定时器开始运行
}

/**
  * 函    数：PWM设置CCR
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
  *           占空比Duty = CCR / (ARR + 1)
  */
void PWM_SetCompare3(uint16_t Compare)
{
	TIM_SetCompare3(TIM2, Compare);		//设置CCR3的值
}

void PWM_SetCompare4(uint16_t Compare)
{
	TIM_SetCompare4(TIM2, Compare);		//设置CCR3的值
}


// ========== 软件PWM实现（用于舵机）==========
// 软件PWM变量
static uint16_t servo1_pulse = 1500;  // 舵机1脉冲宽度（微秒），默认1500us（中位）
static uint16_t servo2_pulse = 1500;  // 舵机2脉冲宽度（微秒）
static uint16_t pwm_counter = 0;      // PWM计数器（单位：100us）

/**
  * 函    数：软件PWM初始化（舵机用）
  * 参    数：无
  * 返 回 值：无
  */
void PWM3_Init(void)
{
    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);         // GPIOB 时钟
    
    /* GPIO 初始化为普通推挽输出 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;              // 推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;          // PB8和PB9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 初始化为低电平
    GPIO_ResetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9);
}

/**
  * 函    数：软件PWM更新函数（在100us定时器中断中调用）
  * 参    数：无
  * 返 回 值：无
  * 说    明：50Hz PWM，周期20ms = 200 * 100us
  */
void PWM_SoftwareUpdate(void)
{
    pwm_counter++;
    
    // 20ms周期（200个100us）
    if (pwm_counter >= 200)
    {
        pwm_counter = 0;
        // 周期开始，拉高所有舵机引脚
        GPIO_SetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9);
    }
    
    // 根据脉冲宽度拉低引脚
    // servo1_pulse和servo2_pulse单位是微秒，pwm_counter单位是100us
    if (pwm_counter == (servo1_pulse / 100))
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_8);
    }
    
    if (pwm_counter == (servo2_pulse / 100))
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_9);
    }
}

void PWM_SetCompare5(uint16_t Compare)
{
    // Compare范围0-2500，对应500-2500us的脉冲宽度
    servo1_pulse = Compare;
    if (servo1_pulse < 500) servo1_pulse = 500;
    if (servo1_pulse > 2500) servo1_pulse = 2500;
}

void PWM_SetCompare6(uint16_t Compare)
{
    // Compare范围0-2500，对应500-2500us的脉冲宽度
    servo2_pulse = Compare;
    if (servo2_pulse < 500) servo2_pulse = 500;
    if (servo2_pulse > 2500) servo2_pulse = 2500;
}
