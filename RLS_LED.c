/*
 * RLS_LED.c
 *
 *  Created on: Aug 31, 2022
 *      Author: Christopher Andrew
 *      Purpose: Manages the driving of the lighting system LED
 */

//####################################################################################################################################################################
//#Define
//####################################################################################################################################################################
#define PWM_PULSE_LENGTH 1000

//PF2 Blue
//PF1 Red
//####################################################################################################################################################################
//	include statements
//####################################################################################################################################################################
/* Standard libs */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Constants */
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>
#include <inc/hw_pwm.h>

/* XDC */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Log.h>                //needed for any Log_info() call
#include <xdc/cfg/global.h>

/* DriverLib */
#include <driverlib/sysctl.h>
#include <driverlib/pin_map.h>
#include <driverlib/gpio.h>
#include <driverlib/uart.h>
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h"

/* TI-RTOS BIOS  */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Peripherals */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
//####################################################################################################################################################################
// define needed varribles
//####################################################################################################################################################################


struct state {
	unsigned int blueLightPercent;
	unsigned int redLightPercent;
	unsigned int brightness;
	int LedSetupComplete;
	int Led_On;
};

struct state LED_State;


int RLS_LED_Setup()
{
	//setup base state of the LED's
	LED_State.blueLightPercent = 0;
	LED_State.redLightPercent = 100;
	LED_State.brightness = 100;

	//configure GPIO pins
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);	// Configures PB6 for typical PWM settings
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
	GPIOPinConfigure(GPIO_PF1_M1PWM5);	// Set PB6 Alt function - find options in includes-> Tivaware C Series -> driverlib -> pin_map.h (under TM4C123GH...)
	GPIOPinConfigure(GPIO_PF2_M1PWM6);

	//configure PWM basline
	//enable PWM module 0 and wait for it to come online
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM1)){}

	//PF1 Red Gen 2 setup
	PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, PWM_PULSE_LENGTH); 	// Set the period 1000 ticks, may need to be much higher depending on transistors switch speed
	PWMGenEnable(PWM1_BASE, PWM_GEN_2);
	PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);	// enables the outputs

	//PF2 Blue Gen 3 setup
	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, PWM_PULSE_LENGTH); 	// Set the period 1000 ticks, may need to be much higher depending on transistors switch speed
	PWMGenEnable(PWM1_BASE, PWM_GEN_3);
	PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);	// enables the outputs

	//initial pulse width set
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 1); 		// Set duty cycle to 0/ticks_in_period ~= 0%
	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, 1);		//setting to 0 causes PWM pin to be fully on

	LED_State.LedSetupComplete = 1; //signals the task it can run and stops the system from running if setup is not completed
	LED_State.Led_On = 1;

	//BUTTON SETUP FOR QUICK TESTING
	//MOVE TO ANOTHER FILE LATTER MAYBE? ALONG WITH DEBOUNCING?

	//magical unlocking thing?
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
	HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);

	return 0;
}

void RLS_LED_Update_Task()
{
	if(LED_State.LedSetupComplete == 1)
	{
		//TODO MAKE Brightness/color cycling clearner and have full cycles, maybe seperate out into functions
		//LED Button Checks
		int OTHERTEST = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0)
		{
			//increment brightness slowly then reset to min at max brightness
			LED_State.brightness = LED_State.brightness + 5;
			if(LED_State.brightness  > 100)
			{
				LED_State.brightness = 0;
			}
		}

		int TESTPLSREMOVE = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
		//adjust red/blue light
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0)
		{
			LED_State.redLightPercent = LED_State.redLightPercent - 5;
			LED_State.blueLightPercent = LED_State.blueLightPercent + 5;

			if(LED_State.redLightPercent == 0 || LED_State.blueLightPercent == 100)
			{
				LED_State.redLightPercent = 100;
				LED_State.blueLightPercent = 0;
			}
		}

		//LED Update Cycle
		if(LED_State.Led_On == 1)
		{
			PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);	// enables the outputs
			PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);

			//PWM Pulse update
			unsigned int blue = (PWM_PULSE_LENGTH * LED_State.blueLightPercent * LED_State.brightness)/(100*100);
			unsigned int red  = (PWM_PULSE_LENGTH * LED_State.redLightPercent * LED_State.brightness)/(100*100);

			if(red > 0)
				PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, red);
			else
				PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 1);

			if(blue > 0)
				PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, blue);
			else
				PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, 1);
		}
		else
		{
			//check latter
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 1);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 1);//might be better to set to minium pulse width instead, unsure if what default state of the pin is
		}
	}
	else
	{
		RLS_LED_Setup();
	}
}

/**
 * allows external accesses to the LED red/blue balance
 */
int RLS_LED_Color_Update(unsigned int redLight, unsigned int blueLight)
{
	LED_State.blueLightPercent = blueLight;
	LED_State.redLightPercent = redLight;
	return 0;
}

/**
 * allows external settings of the led brightness
 *
 */
int RLS_Brightness_Update(unsigned int brightness)
{
	LED_State.brightness = brightness;
	return 0;
}

/**
 * allows the turning on or off of the LED's
 * 1 = on
 * 0 = off
 */
int RLS_LED_Control(unsigned int onOff)
{
	LED_State.Led_On = onOff;
	return 0;
}
