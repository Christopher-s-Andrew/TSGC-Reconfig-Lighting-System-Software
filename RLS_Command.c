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
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include <ti/sysbios/knl/Task.h>

#include "RLS_LED.h"
#include "RLS_USB.h"
#include "RLS_Command.h"
//##################################################################################################
//defines
//###################################################################################################
#define COMMAND_LIST_SIZE 5
#define MAX_COMMAND_SIZE 64
#define NAX_COMMAND_TXT_SIZE 3
//###############################################################################################
//fucntion defininitions
//#######################################################################################################
//put reference to function the command here
int (*commandFuctions[COMMAND_LIST_SIZE+1])()={
		&RLS_LED_Control,
		&RLS_LED_Mode_Update,
		&RLS_Brightness_Update,
		&RLS_LED_Color_Blue_Update,
		&RLS_LED_Color_Red_Update,
		NULL
};

//put command name here
unsigned char commandName[COMMAND_LIST_SIZE+1][NAX_COMMAND_TXT_SIZE+1]={
		"LCT\0", //LED on or off
		"LMS\0", //LED mode set
		"LBS\0", //LED brightness set
		"LBP\0",	//LED blue light set
		"LRP\0",	//LED red light set
		"NUL\0"		//nothing
};

//define number of arguments
unsigned int commandArgument[COMMAND_LIST_SIZE+1] = {
		1,
		1,
		1,
		1,
		1,
		0
};

//##########################################################################################################
//Command parsing
//##########################################################################################################
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

		printf("BUILD COMMAND\n");
		i=0;
		unsigned char comBuff = 0;
		while(comBuff != '$')
		{
			USB_serialRX(&comBuff, 1);
		//	USB_serialTX((unsigned char*)(comBuff+sizeof(char)) ,1);

			printf("CHAR_RECIVED=%c\n", comBuff);
			command[i] = comBuff;
			i++;
		}
		printf("COMMAND ASSEMBLED =%s\n",command);
		//echo test to make sure usb is working right and to provided record that command is being processed
		//USB_serialTX("I GOT DATA\n", 11);

		//Parsing and searching for the base command
		int command_List_Count = 0;
		int command_Char_Count = 0;
		int command_Good = 1;
		for(command_List_Count=0; command_List_Count < COMMAND_LIST_SIZE; command_List_Count++)
		{
			command_Good = 1;
			printf("COMMAND_LIST_POS %d, %c%c%c\n", command_List_Count, commandName[command_List_Count][0], commandName[command_List_Count][1], commandName[command_List_Count][2]);
			//check command
			for(command_Char_Count=0; command_Char_Count<NAX_COMMAND_TXT_SIZE; command_Char_Count++)
			{
				printf("COM_COMPAR_%d %c %c STATUS %d\n",command_Char_Count, command[command_Char_Count],commandName[command_List_Count][command_Char_Count], command_Good);

				if(command[command_Char_Count] != commandName[command_List_Count][command_Char_Count])
				{
					printf("COMMAND_BAD %d\n", command_List_Count);
					//invalid command, check next command
					command_Good = 0;
					break;
				}
			}

			if(command_Good == 1)
			{
				printf("COMMAND_NUMBER %d\n", command_List_Count);
				printf("COMMAND_FOUND %s\n",commandName[command_List_Count]);
				break;
			}
		}


		if((command_Good == 1) && (commandFuctions != NULL))
		{
			int arg = 0;
			//argument parsing
			switch(commandArgument[command_List_Count])
			{
			case 0:
				(*commandFuctions[command_List_Count])();
				break;
			case 1: //1 argument

				arg = atoi((char *)(command + 4*sizeof(char))); //change to look for next whitespace latter

				(*commandFuctions[command_List_Count])(arg);
				break;
			default:
				printf("UNHANDELED_ARGUMENT_COUNT\n");
				break;
			}
		}

		Task_sleep(10);
	}
}

