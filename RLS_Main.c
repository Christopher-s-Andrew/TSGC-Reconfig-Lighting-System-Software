/*
 * RLS_Main.c
 *
 *  Created on: Jul 3, 2022
 *      Author: Chris
 */

#define PART_TM4C123GH6PM
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

int main()
{
	//hardware setup

	//other software setup

	//start RTOS
}
