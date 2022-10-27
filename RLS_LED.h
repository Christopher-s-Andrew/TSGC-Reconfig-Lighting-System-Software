/*
 * RLS_LED.h
 *
 *  Created on: Aug 31, 2022
 *      Author: Chris
 */

#ifndef RLS_LED_H_
#define RLS_LED_H_

/**
 * main LED manging function
 *
 */
void RLS_LED_Update_Task();

int RLS_LED_Setup();
int RLS_LED_Color_Update(unsigned int redLight, unsigned int blueLight);
int RLS_Brightness_Update(unsigned int brightness);
int RLS_LED_Control(unsigned int onOff);


#endif /* RLS_LED_H_ */
