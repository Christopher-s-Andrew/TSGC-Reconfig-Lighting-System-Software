/*
 * RLS_Main.c
 *
 *  Created on: Jul 3, 2022
 *      Author: Chris
 */

//#define PART_TM4C123GH6PM
#define TARGET_IS_TM4C123_RB1

/* Standard libs */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Constants */
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>

/* XDC */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Log.h>                //needed for any Log_info() call
#include <xdc/runtime/Timestamp.h>
#include <xdc/cfg/global.h>

/* DriverLib */
#include <driverlib/sysctl.h>
#include <driverlib/pin_map.h>
#include <driverlib/gpio.h>
#include <driverlib/uart.h>
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"

//RTOS
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

//RLS includes
#include "EK_TM4C123GXL.h"
#include "Board.h"
#include "RLS_LED.h"

Task_Struct task0Struct;
#define TASKSTACKSIZE   512
Char task0Stack[TASKSTACKSIZE];

Void heartBeatFxn(UArg arg0, UArg arg1);

int main()
{
	Task_Params taskParams;

	/* Call board init functions */
	Board_initGeneral();
	Board_initGPIO();
	// Board_initI2C();
	// Board_initSDSPI();
	// Board_initSPI();
	// Board_initUART();
	// Board_initUSB(Board_USBDEVICE);
	// Board_initWatchdog();
	// Board_initWiFi();

	Task_Params_init(&taskParams);
	taskParams.arg0 = 1000;
	taskParams.stackSize = TASKSTACKSIZE;
	taskParams.stack = &task0Stack;
	Task_construct(&task0Struct, (Task_FuncPtr)heartBeatFxn, &taskParams, NULL);

	RLS_LED_Setup();
	/* Start BIOS */
	BIOS_start();

	return (0);
}

Void heartBeatFxn(UArg arg0, UArg arg1)
{
	int a = 0;
    while (1) {
        Task_sleep((UInt)arg0);
        a = a + 1;
    }
}

