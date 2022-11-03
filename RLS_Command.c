/*
 * RLS_Command.c
 *
 *  Created on: Oct 19, 2022
 *      Author: Chris
 */

//################################################################################################
//Includes
//###############################################################################################
#include <string.h>

#include <ti/sysbios/knl/Task.h>

#include "RLS_USB.h"
#include "RLS_Command.h"
//##################################################################################################
//defines
//###################################################################################################
#define COMMAND_LIST_SIZE 2
#define MAX_COMMAND_SIZE 64
#define NAX_COMMAND_TXT_SIZE 3
//###############################################################################################
//fucntion defininitions
//#######################################################################################################
void (*commandFuctions[COMMAND_LIST_SIZE])()={

};

char* commandName[COMMAND_LIST_SIZE]={
		"LON\0"
		"LOF\0"
};

unsigned int commandArgument[COMMAND_LIST_SIZE] = {
		0,
		0
};
/**
 * command task
 * reads USB buffer
 * parses command
 * interpets command
 * sends command to apropriate command fuction for handling
 * swap to software interupts if more than 1 command source is required
 */
void command_Task(UArg arg0, UArg arg1)
{
	unsigned char command[MAX_COMMAND_SIZE];
	USB_Setup();

	while(1)
	{
		printf("CLEAN_COMMAND_BUFFER\n");
		//clean up command buffer
		int i = 0;
		for(i=0; i<MAX_COMMAND_SIZE; i++)
		{
			command[i] = 0;
		}

		printf("COMMAND_WAIT_FOR_COMMAND\n");
		//get command
		int returnCount = USB_serialRX(command, MAX_COMMAND_SIZE);

		printf("COMMAND_AQUIRED_COUNT %d\n", returnCount);
		//echo test to make sure usb is working right and to provided record that command is being processed
		USB_serialTX("I GOT DATA\n", 11);

		printf("RETURNING_COMMAND\n");
		USB_serialTX(command, returnCount);

		printf("COMMAND_PARSING\n");

		/**
		//parse
		char delim[] = " ";
		char* commandToken = strtok((char*)command, delim);

		//search for command
		for(i=0; i<COMMAND_LIST_SIZE; i++)
		{
			if(0==strcmp(commandToken, commandName[i]))
				break;
		}
		//continue parsing for any arguments as needed
		switch(i)
		{
			case 0://0 arguments, no additional parsing
			{
				(*commandFuctions[i])();
			}
			default:
			{
				(*commandFuctions[i])();
			}
		}

		**/
		//run command
		Task_sleep(10);
	}
}

