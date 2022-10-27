/*
 * RLS_USB.h
 *
 *  Created on: Oct 19, 2022
 *      Author: Chris
 */

#ifndef RLS_USB_H_
#define RLS_USB_H_

#include <inc/hw_ints.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ti/sysbios/BIOS.h>

unsigned int USB_serialTX(unsigned char* packet, int size);
unsigned int USB_serialRX(unsigned char* packet, int size);

unsigned int USB_Setup(void);
void USBCDCD_hwiHandler(UArg arg0);
unsigned int USB_WaitConnect(unsigned int timeout);




#endif /* RLS_USB_H_ */
