/**
 * Creator: Christopher Andrew
 * Creation Date: 7/4/2022
 * Purpose: to provide CDC and DFU functionality to the device and allow external systems via a driver to send commands over a virtual serial port
 * based on the USB CDC example provided by texas instraments
 */
//##################################################################################################################################################
//include statements
//##################################################################################################################################################
//XDC tools
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

//HWI
#include <inc/hw_ints.h>
#include <inc/hw_types.h>

//USB libraries
#include <usblib/usb-ids.h>
#include <usblib/usblib.h>
#include <usblib/usbcdc.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdcdc.h>

//std
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

//bios Include
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/gates/GateMutex.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/hal/Hwi.h>
//header includes
#include "RLS_USB.h"

//#####################################################################################################
//Other Helpful things
//#####################################################################################################
//needed types
#if defined(TIVAWARE)
typedef uint32_t            USBCDCDEventType;
#else
#define eUSBModeForceDevice USB_MODE_FORCE_DEVICE
typedef unsigned long       USBCDCDEventType;
#endif

//states to help manage usb
typedef volatile enum{
	USB_UNCONFIG=0,
	USB_READY = 1,
} USBCDCD_USBState;
static volatile USBCDCD_USBState state;

//semaphores for data transmission/recive/conection control
//done in rtos
static Semaphore_Handle RX_Ready;
static Semaphore_Handle TX_Ready;
static Semaphore_Handle USB_Connected;

//gates for sole access to tx/rx
//may be able to not need these as I plan to only have one thing using the USB, but better play it safe
static GateMutex_Handle gate_Tx;
static GateMutex_Handle gate_Rx;
static GateMutex_Handle gate_Wait;

//######################################################################################################
//Internal Function declerations
//######################################################################################################
static USBCDCDEventType cdc_RX_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
        void *eventMsgPtr);
static USBCDCDEventType cdc_TX_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
        void *eventMsgPtr);
static USBCDCDEventType cdc_Event_Control_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
        void *eventMsgPtr);
//##################################################################################################################################################
//generic USB configurations
//##################################################################################################################################################

//MAKE SURE THESE VALUES MATCH THE .inf for driver assosiation
#define USB_VID 0x6666 //change to real own VID when publishing
#define USB_PID 0xC6C4 //product ID replace with real owned when publishing
#define USB_MAX_PACKET 64 //max number of bytes that can be Tx/RX in a single packet
						//leave at 64 unless USB buffer is implemented
#define USB_MAX_WAIT_FOR_CHECK 10
//##################################################################################################################################################
//DEFINE USB CDC PORT HERE
//##################################################################################################################################################

//USB DType strings are specific length with hull seperators?

//language supported
const uint8_t g_pui8LangDescriptor[] =
{
		4,
		USB_DTYPE_STRING,
		USBShort(USB_LANG_EN_US)
};

//manurfactureer name
//	Team #6 Cohort C4
const uint8_t g_pui8ManufacturerString[] =
{
		(17+1)*2,
		USB_DTYPE_STRING,
		'T', 0, 'e', 0, 'a', 0, 'm', 0, ' ', 0, '#', 0, '6', 0, ' ',
		'C', 0, 'o', 0, 'h', 0, 'o', 0, 'r', 0, 't', 0, ' ', 0, 'C', 0, '4', 0
};

//product string
//	RLS COM Port
const uint8_t g_pui8ProductString[] =
{
		(12+1)*2,
		USB_DTYPE_STRING,
		'R', 0, 'L', 0, 'S', 0, ' ', 0,
		'C', 0, 'O', 0, 'M', 0, ' ', 0, 'P', 0, 'o', 0, 'r', 0, 't', 0

};

//serial number string
//	12345678
const uint8_t g_pui8SerialNumberString[] =
{
	(8+1)*2,
	USB_DTYPE_STRING,
	'1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};
//control interface desecription
const uint8_t g_pui8ControlInterfaceString[] =
{
		(18+1)*2,
		USB_DTYPE_STRING,
		'R', 0, 'L', 0, 'S', 0, ' ', 0,
		'C', 0, 't', 0, 'r', 0, 'l', 0, ' ', 0,
		'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0
};
//configuration description string
//	External Charging, control
const uint8_t g_pui8ConfigString[] =
{
		(25+1)*2,
		USB_DTYPE_STRING,
		'E', 0, 'x', 0, 't', 0, 'e', 0, 'r', 0, 'n', 0, 'a', 0, 'l', 0,
		' ', 0, 'C', 0, 'h', 0, 'a', 0, 'r', 0, 'g', 0, 'i', 0, 'n', 0, 'g', 0,
		' ', 0, 'c', 0, 'o', 0, 'n', 0, 't', 0, 'r', 0, 'o', 0, 'l', 0
};

//usb description table
const uint8_t * const g_ppui8StringDescriptors[] =
{
		g_pui8LangDescriptor,
		g_pui8ManufacturerString,
		g_pui8ProductString,
		g_pui8SerialNumberString,
		g_pui8ControlInterfaceString,
		g_pui8ConfigString
};

//size of description table
#define NUM_STRING_DESCRIPTORS (sizeof(g_ppui8StringDescriptors) / sizeof(uint8_t *))

//defing cdc interface
static tUSBDCDCDevice g_sCDCDevice =
{
		// The Vendor ID
		USB_VID,
		//product ID
		USB_PID,
		//estimated power consumption in mA
		//update once we know the max draw of the battery while charging
		0,
		//power type
		USB_CONF_ATTR_SELF_PWR,
		//
		// A pointer to your control callback event handler.
		//
		cdc_Event_Control_Handler,
		//
		// A value that you want passed to the control callback alongside every
		// event.
		//
		0, //IDK WHAT DATA I WANT
		//
		// A pointer to your receive callback event handler.
		//
		cdc_RX_Handler,
		//
		// A value that you want passed to the receive callback alongside every
		// event.
		//
		0, //IDK WHAT DATA I WANT buffer if usb buffer is used
		//
		// A pointer to your transmit callback event handler.
		//
		cdc_TX_Handler,
		//
		// A value that you want passed to the transmit callback alongside every
		// event.
		//
		0, //IDK WHAT DATA I WANT buffer if used
		//
		// A pointer to your string table describing the cdc port
		//
		g_ppui8StringDescriptors,
		//
		// The number of entries in your string table size define
		//
		NUM_STRING_DESCRIPTORS
};

static tLineCoding g_sLineCoding = {
    115200,                     /* 115200 baud rate. */
    1,                          /* 1 Stop Bit. */
    0,                          /* No Parity. */
    8                           /* 8 Bits of data. */
};

//############################################################################################################################
//usb CDC event handler functions start here
//############################################################################################################################

/*
 * Handles recieve events that fire when the device recives information over USB
 *
 */
static USBCDCDEventType cdc_RX_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
        void *eventMsgPtr)
{
	printf("RX_EVENT_%d\n", event);
	switch(event)
		{
		case USB_EVENT_RX_AVAILABLE: //data availible
		{
			printf("USB_EVENT_RX_AVAILABLE\n");
			Semaphore_post(RX_Ready);
			break;
		}
		case USB_EVENT_DATA_REMAINING: //data still present?
		{
			printf("USB_EVENT_DATA_REMAINING\n");
			int i = USBDCDCRxPacketAvailable(&g_sCDCDevice);
			return i;
			break;
		}
		case USB_EVENT_ERROR: //error of some kind
		{
			printf("USB_EVENT_ERROR\n");
			break;
		}
		default:
		{
			printf("RX_UNHANDELED_EVENT\n");
			break;
		}
		}
	return 0;
}

static USBCDCDEventType cdc_TX_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
        void *eventMsgPtr)
{
	printf("TX_EVENT_%d\n", event);
	switch(event)
	{
		case USB_EVENT_TX_COMPLETE: //data sent, can have more data
		{
			printf("USB_EVENT_TX_COMPLETE\n");
			Semaphore_post(TX_Ready);
			break;
		}
		default:
		{
			printf("TX_UNHANDELED_EVENT\n");
			break;
		}
	}
	return 0;
}

/**
 * handles control events of the sub device
 *
 */
static USBCDCDEventType cdc_Event_Control_Handler(void *cbData, USBCDCDEventType event, USBCDCDEventType eventMsg,
                                        void *eventMsgPtr)
{
	printf("CDC_CONTROL_HANDLE_%d\n", event);
	tLineCoding *lineConfig;

	switch(event)
	{
		case USB_EVENT_CONNECTED: //we are connected
		{
			state = USB_READY;
			printf("USB_EVENT_CONNECTED\n");
			Semaphore_post(USB_Connected);
			break;
		}
		case USB_EVENT_DISCONNECTED: //we are no longer connected
		{
			printf("USB_EVENT_DISCONNECTED\n");
			 state = USB_UNCONFIG;
			break;
		}
		case USB_EVENT_SUSPEND:
		{
			printf("USB_EVENT_SUSPEND\n");
			break;
		}
		case USB_EVENT_RESUME:
		{
			printf("USB_EVENT_RESUME\n");
			break;
		}
		case USBD_CDC_EVENT_SEND_BREAK:
		{
			printf("USBD_CDC_EVENT_SEND_BREAK\n");
			break;
		}
		case USBD_CDC_EVENT_CLEAR_BREAK:
		{
			printf("USBD_CDC_EVENT_CLEAR_BREAK\n");
			break;
		}
		case USBD_CDC_EVENT_SET_LINE_CODING: //transmission speed/style setting
		{
			printf("USBD_CDC_EVENT_SET_LINE_CODING\n");
			lineConfig = (tLineCoding *)eventMsgPtr;
			 g_sLineCoding = *(lineConfig);
			break;
		}
		case USBD_CDC_EVENT_GET_LINE_CODING:
		{
			printf("USBD_CDC_EVENT_GET_LINE_CODING\n");
			lineConfig = (tLineCoding *)eventMsgPtr; //pointer to line coding
			*(lineConfig) = g_sLineCoding; //save line coding
			break;
		}
		case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
		{
			printf("USBD_CDC_EVENT_SET_CONTROL_LINE_STATE\n");
			break;
		}
		default:
		{
			printf("USB_CONTROL_UNHANDELED_EVENT\n");
			break;
		}
	}
	return 0;
}

/**
 * forwards the usb interrupts to the usb lib because we can't easily point the rtos to the proper
 * 	interrupt handler in tivaware because reasons
 *
 */
void USBCDCD_hwiHandler(UArg arg0)
{
    USB0DeviceIntHandler();
}


//############################################################################################################################
//usb TX/RX
//############################################################################################################################

/**
 * transmits provided data
 * fails and provides error data if required
 *
 */
unsigned int USB_serialTX(unsigned char* packet, int size)
{
	printf("WAIT_FOR_TX_DATA\n");
	unsigned int gateKey; //aperently this can't be declared inside a switch
	unsigned int returnData = 0;
	//enter gate for sole control of transmission access
	gateKey = GateMutex_enter(gate_Tx);
	switch(state)
	{
	case USB_UNCONFIG:
		printf("NO_CONNECTION_TX\n");
		USB_WaitConnect(BIOS_WAIT_FOREVER); //wait for usb host to be connected
		break;
	case USB_READY:
		printf("TX_START\n");
		//check if packet is large enough to get
		if(size <= USBDCDCTxPacketAvailable(&g_sCDCDevice))
		{
			printf("DATA_TRANSMITED\n");
			//send data
			int a = USBDCDCPacketWrite(&g_sCDCDevice, packet, size, 0);
			a = a;
		}
		else
		{
			printf("NO_ROOM_TX\n");
			returnData = 1; //returns 1 if transmission fails
		}
		printf("TX_END\n");
		break;
	}

	printf("WAIT_FOR_TRANSMISION\n");
	//pend until it is safe to leave and allow another transmission
	Semaphore_pend(TX_Ready, BIOS_WAIT_FOREVER);

	GateMutex_leave(gate_Tx, gateKey);
	printf("TRANSMISION_COMPLETE\n");
	return returnData;
}

/**
 * attempts to retrive a packet from the usb buffer if there is no packet the function will wait for one
 * varius integers will be returned if this fails
 *
 */
unsigned int USB_serialRX(uint8_t* packet, int size)
{
	printf("WAIT_FOR_RX_DATA\n");
	unsigned int gateKey;
	unsigned int returnData =0;
	gateKey = GateMutex_enter(gate_Rx);
	int returnValue = 0;
	//wait for semaphore, check every 10 ms? maybe want longer for less power usage
	while(!Semaphore_pend(RX_Ready, BIOS_WAIT_FOREVER))
	{
		Task_sleep(USB_MAX_WAIT_FOR_CHECK);
	}

	switch(state)
	{
	case USB_UNCONFIG:
		printf("NO_CONNECT_WAIT_RX\n");
		USB_WaitConnect(BIOS_WAIT_FOREVER);
		break;
	case USB_READY:
		printf("RX_START\n");
		if(size >= USBDCDCRxPacketAvailable(&g_sCDCDevice))
		{
			returnData =  USBDCDCRxPacketAvailable(&g_sCDCDevice);
			USBDCDCPacketRead(&g_sCDCDevice, packet, returnData, 0);
		}
		else
		{
			returnData = 1;
		}
		printf("RX_END\n");
		break;
	default:
		printf("UNHANDLED_RX_STATE");
	}
	GateMutex_leave(gate_Rx, gateKey);

	return returnData;
}

//############################################################################################################################
//USB setup and utility functions
//############################################################################################################################

/**
 * sets up the composite CDC DFU device
 *
 */
unsigned int USB_Setup(void)
{
	Error_Block eb;
	printf("USB_SETUP\n");

	//hwi creation, not sure rtos config is doing it right?
	Hwi_create(INT_USB0, USBCDCD_hwiHandler, NULL, &eb);

	//USB is not connected to anything initially
	state = USB_UNCONFIG;

	//semaphores created by rtos config
	Semaphore_Params smeaDefine;

	Semaphore_Params_init(&smeaDefine);
	smeaDefine.mode = Semaphore_Mode_BINARY;
	RX_Ready = Semaphore_create(0, &smeaDefine, &eb);
	TX_Ready = Semaphore_create(0, &smeaDefine, &eb);
	USB_Connected = Semaphore_create(0, &smeaDefine, &eb);

	//gates
	gate_Tx = GateMutex_create(NULL, &eb);
	gate_Rx = GateMutex_create(NULL, &eb);
	gate_Wait = GateMutex_create(NULL, &eb);

	//force device mode
	USBStackModeSet(0, eUSBModeForceDevice, 0);

	//start USB Device
	if(!USBDCDCInit(0, &g_sCDCDevice))
	{
		System_abort("USB failed to start");
	}


	return 0;
}

unsigned int USB_WaitConnect(unsigned int timeout)
{
	printf("WAIT_FOR_CONNECT\n");
	unsigned int gateKey;
	//enter gate, wait for semaphore
	gateKey = GateMutex_enter(gate_Wait);
	if(state == USB_UNCONFIG)
	{
		while(!Semaphore_pend(USB_Connected, BIOS_WAIT_FOREVER))
		{
			Task_sleep(USB_MAX_WAIT_FOR_CHECK);
		}
	}

	GateMutex_leave(gate_Wait, gateKey);
	printf("CONNECTION_FOUND\n");
	return 0;
}

//############################################################################################################################
//USB DFU functionality, may be simplier to leave out as device is easy to disassemble to reach debug port
//############################################################################################################################
/**

uint32_t DFU_Detach_Callback(void *pvCBData, uint32_t ui32Event, uint32_t ui32MsgData, void *pvMsgData)
{
	switch(ui32Event)
	{
		case USBD_DFU_EVENT_DETACH:
			//start up clean up then BIOS_exit somewhere else once clean up is done
			break;
	}

}
*/
