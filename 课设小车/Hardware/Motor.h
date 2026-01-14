#ifndef __MOTOR_H
#define __MOTOR_H

void Motor_Init(void);
void Motor_SetRspeed(int8_t Speed);
void Motor_SetLspeed(int8_t Speed);
void Motor_ClosedLoop_Control(void);
void Motor_SetTargetSpeed(float speed_l, float speed_r);
int PID_L(void);
int PID_R(void);
void motorout(void);
extern int pwmoutl , pwmoutr;
#endif
