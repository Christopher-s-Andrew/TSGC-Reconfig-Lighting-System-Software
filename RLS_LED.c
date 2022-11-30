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
#define PWM_PULSE_LENGTH 					60000 //currently at 1333 [Hz] assuming 80 [MHz] Clock
#define DEFAULT_BRIGHTNESS_CHANGE_PERCENT	5
#define DEFAULT_TINT_CHANGE_PERCENT			5
//PF2 Blue
//PF1 Red
//####################################################################################################################################################################
//	include statements
//####################################################################################################################################################################
/* Standard libs */
#include <stdint.h>
#include <stdbool.h>
//#include <stdio.h>

/* Constants */
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>
#include <inc/hw_pwm.h>


/* DriverLib */
#include <driverlib/sysctl.h>
#include <driverlib/pin_map.h>
#include <driverlib/gpio.h>
#include "driverlib/pwm.h"

#include "RLS_USB.h"
/* TI-RTOS BIOS  */
//#include <ti/sysbios/BIOS.h>
//#include <ti/sysbios/knl/Task.h>

//####################################################################################################################################################################
// define needed local varribles
//####################################################################################################################################################################


struct state {
	int blueLightPercent;
	int redLightPercent;
	int tintChange;
	int brightness;
	int brightnessChange;
	int LedSetupComplete;
	int Led_On;
	int mode; //mode 0 basic brightten darken cycles
};

static struct state LED_State;


int RLS_LED_Setup()
{
	//setup base state of the LED's
	LED_State.blueLightPercent = 0;
	LED_State.redLightPercent = 100;
	LED_State.brightness = 100;
	LED_State.brightnessChange = DEFAULT_BRIGHTNESS_CHANGE_PERCENT;
	LED_State.tintChange = DEFAULT_TINT_CHANGE_PERCENT;
	LED_State.mode = 0;

	//configure GPIO pins
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);	// Configures PB6 for typical PWM settings
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
	GPIOPinConfigure(GPIO_PF1_M1PWM5);	// Set PB6 Alt function - find options in includes-> Tivaware C Series -> driverlib -> pin_map.h (under TM4C123GH...)
	GPIOPinConfigure(GPIO_PF2_M1PWM6);

	//configure PWM basline
	//enable PWM module and wait for the module to be ready for configuration
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

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)){}

	GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_4, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
	GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_5 | GPIO_PIN_4);

	//Clock task currently setup in RTOS Config, look into setting up here instead latter
	return 0;
}

void RLS_LED_Update_Task()
{
	if(LED_State.LedSetupComplete == 1)
	{
		//TODO MAKE Brightness/color cycling clearner and have full cycles, maybe seperate out into functions?
		//LED Button Checks

		if((GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_5) == 0) && (LED_State.mode == 0))
		{
			//increment brightness slowly then reset to min at max brightness
			LED_State.brightness = LED_State.brightness + LED_State.brightnessChange;
			if(LED_State.brightness > 100) //check for brightness going over 100%
			{
				LED_State.brightness = 100;
				LED_State.brightnessChange = -1 * LED_State.brightnessChange;
			}
			else if((LED_State.brightness - LED_State.brightnessChange) < 0) //check for brightness going under 0%
			{
				LED_State.brightness = 0;
				LED_State.brightnessChange = -1 * LED_State.brightnessChange;
			}
		}

		//adjust red/blue light
		if((GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4) == 0) && (LED_State.mode == 0))
		{
			//update light percent
			LED_State.redLightPercent = LED_State.redLightPercent - LED_State.tintChange;
			LED_State.blueLightPercent = LED_State.blueLightPercent + LED_State.tintChange;
			//then check boundries
			if(LED_State.redLightPercent > 100 || LED_State.blueLightPercent < 0) //check if red goes over 100 and flips tinting and corrects boundries
			{
				LED_State.redLightPercent = 100;
				LED_State.blueLightPercent = 0;
				LED_State.tintChange = -1 * LED_State.tintChange;
			}
			else if(LED_State.blueLightPercent > 100 || LED_State.redLightPercent < 0) //checks if blue goes over 100% and flips tinting and corrects boundries
			{
				LED_State.redLightPercent = 0;
				LED_State.blueLightPercent = 100;
				LED_State.tintChange = -1 * LED_State.tintChange;
			}

		}

		//LED Update Cycle
		if(LED_State.Led_On == 1)
		{

			//PWM Pulse update
			//split to allow larger pwm periods with less issues
			int blue = PWM_PULSE_LENGTH * LED_State.blueLightPercent / 100;
			blue = blue * LED_State.brightness / 100;
			int red  =PWM_PULSE_LENGTH * LED_State.redLightPercent / 100;
			red = red * LED_State.brightness / 100;

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
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, 1);
			PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, 1);//might be better to set to minium pulse width instead, unsure if what default state of the pin is
		}
	}
	else
	{
		RLS_LED_Setup();
	}
}

/**
 * allows changing the LED mode to turn on or off button controls
 */
int RLS_LED_Mode_Update(unsigned int mode)
{
	LED_State.mode = mode;
	return 0;
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

int RLS_LED_Color_Blue_Update(int blueLight)
{
	LED_State.blueLightPercent = blueLight;
	return 0;
}

int RLS_LED_Color_Red_Update(int redlight)
{
	LED_State.redLightPercent = redlight;
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
