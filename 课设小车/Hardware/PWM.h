#ifndef __PWM_H
#define __PWM_H

void PWM_Init(void);
void PWM3_Init(void);
void PWM_SetCompare3(uint16_t Compare);
void PWM_SetCompare4(uint16_t Compare);
void PWM_SetCompare5(uint16_t Compare);
void PWM_SetCompare6(uint16_t Compare);
void PWM_SoftwareUpdate(void);  // 软件PWM更新函数（在定时器中断中调用）

#endif
