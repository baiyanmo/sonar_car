#ifndef __SONAR_H
#define __SONAR_H

extern int time;		//超声波用时间
extern int distance;	//路程距离

void Sonar_Init(void);
void Sonar_Start(void);
uint16_t Sonar_GetValue(void);

#endif
